# ESP32-P4 PIE SIMD Optimization Findings for H.264 Decoding

## Executive Summary

Of the four hottest H.264 decoder functions tested, only **one** benefited from
SIMD optimization on the ESP32-P4 PIE instruction set. A SIMD memcpy also showed
significant gains. The remaining three functions were faster with scalar C code.

| Function | Decode share | Best SIMD result | Status |
|---|---|---|---|
| memcpy (bulk copy) | utility | **4.3-4.5x** | SIMD optimized |
| interpolate_ver_half | 35-40% | **1.50x** | SIMD optimized |
| idct_4x4 | 15-20% | 0.91x (slower) | C reference kept |
| write_output_blocks | 10-15% | 0.79x (slower) | C reference kept |
| deblock_ver_luma | 15-20% | not attempted | C reference kept |

**Estimated overall decode improvement: 12-16%** (1.50x on 35-40% of decode time).

---

## What Worked

### SIMD memcpy — 4.3-4.5x speedup

An 8-register unrolled SIMD memcpy using all q0-q7 registers to copy 128 bytes per
loop iteration. Based on the approach from Espressif's PIE blog post.

- Uses `esp.vld.128.ip` + `esp.vst.128.ip` with post-increment
- 8 loads followed by 8 stores per iteration (128 bytes per loop)
- Requires 16-byte aligned buffers and sizes that are multiples of 128
- Benchmarked against both a byte-loop and libc `memcpy` — both were ~4.4x slower
- Notably, the platform's libc `memcpy` is not optimized (same speed as byte loop)

### interpolate_ver_half — 1.50x speedup

The 6-tap vertical FIR filter for half-pixel motion compensation. This was the
only H.264 function where SIMD provided a measurable speedup.

**Why it worked:**
- Rows are 16 pixels (bytes) wide — fills a full 128-bit vector register
- The filter applies the same operation to every pixel (uniform compute)
- Many rows to process (partHeight + 5 input rows), amortizing setup cost
- The 6-tap filter is compute-heavy (~13 scalar ops per pixel)

**Implementation approach (5 iterations to get here):**

| Version | Speed | Problem |
|---|---|---|
| v1 | 0.23x | Scalar u8→i16 widening per function call |
| v2 | 0.32x | memcpy to aligned buffer per call, redundant re-widening |
| v3 | 0.44x | Widen once upfront, but constant arrays reloaded per chunk |
| v4 | 1.20x | `esp.ld.128.usar.ip` unaligned load eliminated all memcpy |
| v5 | 1.50x | Removed QACC >>5 path, use clp[] table for narrowing |

**Key techniques:**
1. **Widen once**: Convert all input rows u8→i16 upfront using `esp.ld.128.usar.ip`
   (unaligned load) + `esp.vzip.8` (zero-extend). Each pixel converted exactly once.
2. **Filter in i16**: Compute `(A+F) - 5*(B+E) + 20*(C+D)` using only `vadd.s16`
   and `vsub.s16`. Multiply by 5 and 20 done with add chains (5x = 4x+x, 20x = 16x+4x).
3. **Narrow via clp[] table**: Scalar `clp[val >> 5]` for the final >>5 shift + clamp
   to [0,255] + u8 narrowing. The clipping table is faster than any SIMD alternative.

---

## What Didn't Work

### idct_4x4 — 0.91x (slower)

**Attempted**: SIMD vertical butterfly using `vadd.s32`, `vsub.s32`, `vsr.s32`
processing all 4 columns in parallel.

**Why it failed**:
- The 4×4 block is only 16 i32 values (64 bytes) — too small for SIMD setup to pay off
- The zig-zag reorder is a random permutation — inherently scalar
- Dequantization uses 3 different multipliers per coefficient position — can't vectorize
- Horizontal butterfly needs cross-lane operations (elements [0]+[2], [1]+[3]) within
  each 4-element row — PIE has no shuffle/permute instruction
- The vertical butterfly (the only SIMD-friendly part) is ~1/3 of total work
- Total function is ~50-60 scalar ops — not enough to amortize SIMD overhead

### write_output_blocks — 0.79x best (slower)

**Attempted 4 versions:**
- v1 (0.91x): `vadd.s32` + `vsat.s32` on 4×i32 per 4×4 block row
- v2 (0.71x): Narrow i32→i16, `vzip` u8→i16, `vadd.s16` + `vsat.s16`
- v3 (0.65x): Inline `if/else` clamp replacing table lookup
- v4 (0.79x): Row-by-row processing of full 16-pixel rows with usar loads

**Why it failed**:
- Residuals are stored as i32 (4 bytes each) in block order, not row order
- Assembling 16 residual values for one pixel row requires gathering from 4
  non-contiguous blocks and narrowing 16 i32→i16 — all scalar
- The C code's `clp[pred + residual]` is a single indexed memory load per pixel
  with zero conversion overhead — essentially unbeatable
- The inline clamp (v3) proved that branch prediction on random pixel data is
  terrible — table lookup is always faster

### deblock_ver_luma — not attempted

**Why it was skipped** (analysis only, no implementation):
- Only 6-8 bytes per row accessed (p3..q3), strided by `imageWidth`
- 4 rows with heavy data-dependent branching: 3+ threshold comparisons per row,
  each independently deciding whether and how to filter
- Within each row, additional conditional branches for p2/q2 filtering
- `CLIP3` macro and `clp[]` table lookups — inherently scalar
- Cross-row vectorization (processing 4 rows at once) is incorrect because
  each row's filter decision depends on that row's specific pixel values
- Total working set: ~32 bytes
- Every factor that made SIMD lose on the other functions is present, plus
  the worst one: per-pixel conditional branching

---

## ESP32-P4 PIE Architecture Findings

### Confirmed Behaviors (tested on device)

**Register constraints:**
- PIE load/store and scalar operand instructions only accept registers x8-x15
  (RISC-V: s0, s1, a0-a5)
- Use `register ... asm("aX")` in inline C asm to force register allocation
- Using registers outside x8-x15 causes assembler errors

**All arithmetic is saturating:**
- `vadd.s32`, `vsub.s32` — saturate to INT32_MAX / INT32_MIN
- `vadd.s16`, `vsub.s16` — saturate to 32767 / -32768
- `vadd.u8`, `vsub.u8` — saturate to 255 / 0
- This is uniform across all arithmetic types — no wrapping variants exist

**vmul.s16 has a built-in >>4 shift:**
- `esp.vmul.s16` computes `sat_s16((a * b) >> 4)`, NOT a standard multiply
- The >>4 shift is baked into the instruction and cannot be disabled
- For full-precision multiply, use `esp.vmulas.s16.qacc` + `esp.srcmb.s16.qacc`

**Unaligned loads work via usar + src.q pattern:**
- `esp.ld.128.usar.ip q0, ptr, 16` — loads from aligned-down address, sets SAR
- `esp.ld.128.usar.ip q1, ptr+16, 0` — loads next aligned block
- `esp.src.q q2, q0, q1` — extracts 16 unaligned bytes using SAR offset
- Tested at offsets 0, 1, 3, 7, 13 — all correct

**vzip.8 / vunzip.8 match S3 TRM:**
- `vzip.8 qd, qs`: interleaves lower 8 elements of each into qd, upper 8 into qs
- `vunzip.8 qd, qs`: exact inverse — separates even/odd indexed elements

**vsat instructions:**
- `vsat.u8`, `vsat.s16`, `vsat.s32`: clamp to arbitrary [lo, hi] range specified
  by two scalar registers
- Boundary-exact: values at lo or hi are unchanged

### Architecture Limitations Discovered

1. **Only 8 vector registers (q0-q7)**: Severe register pressure. Can hold 6 data
   rows + 2 scratch, but no room for constants. Constants must be loaded from memory
   every time they're needed.

2. **No lane shuffle/permute**: Can't rearrange elements within a register. This
   prevents vectorizing operations that need cross-lane data (e.g., horizontal
   butterfly: needs elements [0]+[2] and [1]+[3]).

3. **No i16 shift instructions**: `vsr.s32` and `vsl.32` exist for 32-bit lanes only.
   For i16 shifts, must use the QACC path (vmulas×1 + srcmb with shift) or scalar.

4. **16-byte alignment required**: All loads/stores silently mask the bottom 4 address
   bits. Unaligned access requires the 3-instruction usar+src.q pattern.

5. **Scalar register constraint**: Only x8-x15 (6 usable: a0-a5) for PIE instruction
   operands. Limits how many pointers/constants can be passed into an asm block.

---

## When to Use SIMD on ESP32-P4

**SIMD wins when ALL of these are true:**
- Row width ≥ 16 bytes (fills a full 128-bit register)
- Contiguous or easily addressable data (no scattered gather/scatter)
- Uniform compute: same operation applied to every element
- Enough rows/iterations to amortize widening/narrowing and setup cost
- No per-element branching or data-dependent control flow

**SIMD loses when ANY of these are true:**
- Working set < 64 bytes (overhead exceeds computation)
- Data requires format conversion (i32↔i16↔u8) at boundaries
- Per-element different operations (different multipliers, permutations)
- Data-dependent branching per element or per row
- Strided access with stride ≠ element width (gather/scatter needed)
- The scalar code uses table lookups (indexed loads are very efficient)

**The clipping table (`clp[val]`) is faster than SIMD clamping** for final u8
output. A single indexed memory load with no branches beats `vsat` + store +
reload + scalar extract. Use SIMD for the bulk compute, then scalar clp[] for
the final narrowing.

---

## Reference Files

| File | Purpose |
|---|---|
| `main/pie_tests.c` | PIE instruction reference & verification (35 tests, all passing) |
| `main/h264_simd.c` | SIMD implementations with detailed failure analysis in comments |
| `main/h264_ref.c` | C reference implementations (verbatim from h264bsd) |
| `main/test_runner.c` | Correctness + performance benchmark harness |

## Documentation References

| Source | Location |
|---|---|
| xesppie.S instruction list | `tanmatsu-launcher/esp-idf/.../rv_decode/xesppie.S` |
| ESP32-S3 TRM Chapter 8 | Best PIE behavioral reference (translate `ee.*` → `esp.*`) |
| Espressif PIE blog | https://developer.espressif.com/blog/2024/12/pie-introduction/ |
| PIE memcpy benchmark | https://gist.github.com/BitsForPeople/b030f9b6417e4091b955c00fd55ddcc0 |
