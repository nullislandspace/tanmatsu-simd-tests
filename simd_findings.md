# ESP32-P4 PIE SIMD Optimization Findings for H.264 Decoding

## Executive Summary

Two rounds of H.264 function optimization have been performed. The first round
(documented in the original version of this file) tested functions with the old
i32 block-order residual layout and found only `interpolate_ver_half` benefited.
The second round (April 2025) retests after the h264bsd rewrite to i16 row-order
residuals, and applies new optimization techniques discovered along the way.

### Round 2 Results (i16 row-order residual layout)

| Function | Decode share | Best SIMD result | Status |
|---|---|---|---|
| memcpy (bulk copy) | utility | **4.3-4.5x** | SIMD optimized (round 1) |
| write_output_blocks | 9% (9ms) | **4.52x** | SIMD optimized |
| deblock_hor_luma | 24% (24ms) | **3.04x** | SIMD threshold + scalar filter |
| convert_image_to_ppa | 11% (11ms) | **1.06x** | Scalar word-pack (PIE N/A) |
| interpolate_hor_half | 7% (part of 20ms) | pending | Not yet optimized |
| interpolate_chroma_hor | 7% (7ms) | pending | Not yet optimized |

### Round 1 Results (i32 block-order residual layout, historical)

| Function | Best SIMD result | Status |
|---|---|---|
| interpolate_ver_half | **1.50x** | SIMD optimized |
| idct_4x4 | 0.91x (slower) | Abandoned |
| write_output_blocks | 0.79x (slower) | Abandoned (layout changed in round 2) |
| deblock_ver_luma | not attempted | Skipped |

---

## Critical Performance Lessons

### 1. Never use memcpy/memset for SIMD data staging

The single biggest performance lesson. Using `memcpy` to copy data to aligned
buffers before SIMD loading is extremely expensive at small sizes (16-64 bytes).

| Approach | DeblockHorLuma result |
|---|---|
| 4x memcpy(16 bytes) + 2x memset(16 bytes) | **0.50x** (2x slower than scalar!) |
| usar direct loads (no memcpy) | **1.16x** |

The `usar` (unaligned shift-register) pattern loads directly from unaligned
source addresses with just 3 PIE instructions:
```asm
esp.ld.128.usar.ip q0, ptr, 16    /* load + set alignment offset */
esp.ld.128.usar.ip q1, ptr, 0     /* load next aligned block */
esp.src.q q2, q0, q1              /* extract correct 16 bytes */
```

**Rule**: Always use usar loads. Never copy to aligned temporaries first.

### 2. Minimize asm block count — merge into single blocks

Each `asm volatile` block is a compiler optimization barrier. Having 6 separate
asm blocks with pointer reassignment between them prevents the compiler from
optimizing the scalar code and causes unnecessary register spills.

| Approach | DeblockHorLuma result |
|---|---|
| 6 separate asm blocks | **1.16x** |
| 1 main asm block + 1 store | **2.44x** |

**Rule**: Merge all PIE instructions into the fewest possible asm blocks.
Pre-compute all pointer operands in C, pass them as register operands (a0-a5).

### 3. Pre-load constant vectors once, reuse across loop iterations

Loading the same constant vector (e.g., zeros for `vmax`, 255 vector for `vmin`)
inside every loop iteration wastes a load instruction per iteration. Loading
once before the loop and keeping the register alive across iterations is free.

| Approach | WriteOutputBlocks result |
|---|---|
| Load vec_255 + zero per row (32 loads total) | **1.63x** |
| Pre-load q6=0, q7=255 once, reuse all 32 rows | **4.74x** |

**Rule**: Identify constants used inside loops. Load them into dedicated Q
registers before the loop. Use q6/q7 for constants, q0-q5 for per-iteration data.

### 4. Use scalar word stores for unaligned output (not memcpy)

Writing SIMD results to unaligned output addresses via `memcpy(imgRow, tmp, 16)`
is slow. Storing to an aligned temp buffer then copying 4 u32 words is faster:

```c
u8 tmp[16] __attribute__((aligned(16)));
/* ... SIMD stores to tmp ... */
u32 *src = (u32 *)tmp;
u32 *dst = (u32 *)imgRow;  /* unaligned but u32-granularity is fine */
dst[0] = src[0]; dst[1] = src[1];
dst[2] = src[2]; dst[3] = src[3];
```

### 5. Use vldbc or vsadds for vector broadcasts — never memset

Broadcasting a byte value to all 16 lanes of a vector has three approaches,
in order of preference:

**Best: `esp.vldbc.8.ip` (broadcast load from memory, 1 instruction)**
Store the value as a single byte, then broadcast-load it. Only reads 1 byte:
```c
u8 __attribute__((aligned(16))) val_buf[16];
val_buf[0] = threshold;
/* In asm: */
"esp.vldbc.8.ip q4, %[ptr], 0\n"   /* q4 = [threshold x 16] */
```

**Good: `esp.vsadds.u16` (broadcast from scalar register, 2 instructions)**
For constants known at compile time, build entirely in-register with no memory:
```asm
esp.zero.q q7
esp.vsadds.u16 q7, q7, %[val]   /* q7 = [val x 8] as u16 */
```
Note: scalar is **truncated to lane width** (low 16 bits for .u16, low 8 for .u8).

**Avoid: memset or scalar word-fill**
`memset(buf, val, 16)` has function call overhead. Scalar word-fill (4 u32 stores)
is better but still 8 stores + 1 load = 9 memory ops vs 1 for vldbc.

| Approach | DeblockHorLuma result |
|---|---|
| memset broadcast | **1.16x** |
| Scalar word-fill + vld.128 | **1.69x** |
| vldbc.8 broadcast | **3.04x** |

### 6. Keep lookup tables in SRAM, not flash

`const` arrays are placed in `.rodata` (flash) by default. Flash access goes
through the cache hierarchy and is significantly slower than SRAM on cache misses.

The H.264 clipping table `h264bsdClip[1280]` (1.25 KB) is accessed hundreds of
times per macroblock. Copying it to a 16-byte-aligned SRAM buffer at startup
gives a measurable speedup on all functions that use it:

```c
u8 *clip_sram = aligned_alloc(16, 1280);
memcpy(clip_sram, h264bsdClip_flash, 1280);
```

| Function | Plain C with flash table | Plain C with SRAM table |
|---|---|---|
| WriteOutputBlocks | 13925 ns | 13226 ns (**5% faster**) |

**Rule**: Any fixed lookup table accessed in a hot loop should be copied to
SRAM at initialization time. The 1280-byte cost is negligible.

### 7. Use SIMD for threshold masks, scalar for conditional logic

For functions with per-element conditional logic (like the deblocking filter),
a hybrid approach works well:

- **Phase 1 (SIMD)**: Compute boolean masks vectorially (absolute differences,
  comparisons, AND-combine). This replaces N×3 scalar comparisons with ~12 PIE
  instructions regardless of N.
- **Early-out**: If no elements pass the threshold (common for smooth regions),
  skip all per-element work. Check via `(mw[0] | mw[1] | mw[2] | mw[3]) == 0`.
- **Phase 2 (scalar)**: Only iterate elements that passed the threshold. Check
  `mask_bytes[i]` directly (no bitmask conversion needed).

### 8. Avoid bitmask conversion — use byte masks directly

Converting a 16-byte SIMD mask (0xFF/0x00 per lane) to a scalar bitmask requires
~16 shift-and-OR operations. Instead, check `mask_bytes[i]` directly in the
scalar phase — a simple byte load + branch-if-zero per element:

```c
/* Instead of this: */
u32 mask = 0;
for (int i = 0; i < 16; i++)
    if (mask_bytes[i]) mask |= (1u << i);
/* ... then: if (!(mask & (1u << i))) continue; */

/* Do this: */
if (!mask_bytes[i]) continue;
```

### 9. SIMD vmax/vmin clamping is faster than clp[] table for vectorized paths

When the entire operation is SIMD (no scalar fallback), using `vmax.s16` with
zero and `vmin.s16` with 255 for [0,255] clamping is faster than storing to
memory and doing per-element `clp[]` table lookups:

```asm
esp.vmax.s16 q0, q0, q6     /* q6 = 0 vector (pre-loaded) */
esp.vmin.s16 q0, q0, q7     /* q7 = 255 vector (pre-loaded) */
```

This replaced the `clp[]` table in WriteOutputBlocks SIMD, contributing to the
4.52x speedup. However, for scalar-only paths, `clp[val]` remains the fastest
single-element clamp (one indexed load, no branches).

---

## Round 2 Function Details

### write_output_blocks — 4.52x speedup

**Operation**: `output[i] = clamp(pred_u8[i] + residual_i16[i], 0, 255)`

The i16 row-order residual layout (from the h264bsd rewrite) makes this function
ideal for SIMD. Each luma row is 16 contiguous u8 predictions + 16 contiguous
i16 residuals (16-byte aligned).

**SIMD pipeline per luma row (16 pixels)**:
1. `usar` load 16 u8 predictions (unaligned)
2. `vzip.8` with zero register → widen to 2x 8×i16
3. `vld.128` 2x aligned i16 residual loads
4. `vadd.s16` — saturating add (both halves)
5. `vmax.s16` / `vmin.s16` — clamp to [0,255] (using pre-loaded constants)
6. `vunzip.8` — narrow i16→u8 (high bytes are 0 after clamp)
7. `vst.128` to aligned temp, scalar u32 stores to output

**Key optimizations that made the difference**:
- Pre-load q6=0, q7=255 once before all 32 rows (was: load per row = 32 extra loads)
- usar load (was: memcpy + memset for chroma = eliminated ~50 function calls)
- u32 stores for output (was: memcpy = eliminated 32 function calls)
- Inline loop (was: per-row function call = eliminated 32 call/return pairs)

### deblock_hor_luma — 2.58x speedup

**Operation**: H.264 horizontal edge filter, 16 pixels, conditional per-pixel.

**Phase 1 (SIMD — ~25 PIE instructions)**:
- usar load p1, p0, q0, q1 rows (4x 3 instructions = 12 insns)
- Aligned load alpha/beta broadcasts (2 insns)
- Compute 3 absolute differences + 3 compares + 2 ANDs (11 insns)
- Store 16-byte mask (1 insn)
- u32 OR early-out check

**Phase 2 (scalar)**: Iterate only masked pixels, apply filter with clp[] table.

**Optimization progression**:

| Version | Speed | Key change |
|---|---|---|
| v1: memcpy + 6 asm blocks + bitmask loop | 0.50x | Baseline SIMD attempt |
| v2: usar loads (no memcpy) | 1.16x | Eliminated 64 bytes of copies |
| v3: scalar word-fill broadcast | 1.69x | Eliminated memset overhead |
| v4: single asm + direct mask_bytes check | 2.44x | Eliminated compiler barriers + bitmask conversion |
| v5: + SRAM clip table | 2.58x | Faster scalar phase table lookups |
| v6: vldbc.8 broadcast | 3.04x | 1-byte load vs 32-byte word-fill |

### convert_image_to_ppa — 1.06x (scalar word-pack)

**Operation**: I420 planar → PPA-packed YUV420 byte interleaving.

PIE SIMD is **not applicable** to this function. The PPA format uses a 3:1
interleave pattern (1 chroma byte per 2 luma bytes) that is fundamentally
incompatible with PIE's power-of-2 interleave instructions. PIE lacks a
general byte shuffle/permute instruction.

The optimization uses scalar word-packing instead: load Y and chroma as u32
words, construct output words using shifts/masks/ORs, write as u32 stores.
This reduces memory operations from 36 byte loads+stores to 12 word loads+stores
per 16 Y pixels.

---

## Round 1 Function Details (Historical)

### interpolate_ver_half — 1.50x speedup

The 6-tap vertical FIR filter for half-pixel motion compensation. See the
"Implementation approach (5 iterations)" table above for the optimization
progression. Key insight: widen all input rows u8→i16 upfront using usar loads,
compute filter entirely in i16 using add chains for multiply-by-5 and
multiply-by-20, then use scalar `clp[val >> 5]` for final narrowing.

### idct_4x4 — 0.91x (abandoned)

4×4 working set (64 bytes) too small for SIMD setup to pay off. Zig-zag reorder
and per-position dequantization multipliers are inherently scalar. No lane
shuffle/permute instruction makes cross-lane butterfly operations impossible.

### write_output_blocks round 1 — 0.79x (abandoned, then revisited)

Failed in round 1 because residuals were i32 in block-order — gathering 16
residual values for one pixel row required accessing 4 non-contiguous blocks.
Succeeded in round 2 (4.52x) after the h264bsd rewrite provided i16 row-order
residuals with 16-byte alignment.

---

## ESP32-P4 PIE Architecture Findings

### Confirmed Behaviors (tested on device)

**Register constraints:**
- PIE load/store and scalar operand instructions only accept registers x8-x15
  (RISC-V: s0, s1, a0-a5)
- Use `register ... asm("aX")` in inline C asm to force register allocation
- Using registers outside x8-x15 causes assembler errors (e.g., a6 = x16 fails)

**All arithmetic is saturating:**
- `vadd.s32`, `vsub.s32` — saturate to INT32_MAX / INT32_MIN
- `vadd.s16`, `vsub.s16` — saturate to 32767 / -32768
- `vadd.u8`, `vsub.u8` — saturate to 255 / 0
- This is uniform across all arithmetic types — no wrapping variants exist

**vmul.s16 has a built-in >>4 shift:**
- `esp.vmul.s16` computes `sat_s16((a * b) >> 4)`, NOT a standard multiply
- The >>4 shift is baked into the instruction and cannot be disabled
- For full-precision multiply, use `esp.vmulas.s16.qacc` + `esp.srcmb.s16.qacc`

**vsadds/vssubs scalar operand is TRUNCATED to lane width:**
- `esp.vsadds.u16`: scalar register is masked to low 16 bits before add
- `esp.vsadds.s16`: scalar register is masked to low 16 bits (sign-extended)
- `esp.vsadds.u8`: scalar register is masked to low 8 bits
- `esp.vsadds.s8`: scalar register is masked to low 8 bits (sign-extended)
- Passing 0x10000 to `.u16` adds 0 (not 65536). Passing 256 to `.u8` adds 0.
- Passing -1 (0xFFFFFFFF) to `.u16` adds 0xFFFF = 65535 (unsigned truncation)
- `vsadds.u16 q_zero, 255` correctly broadcasts 255 to all u16 lanes.
  (Originally suspected broken, but actual bug was a truncated clipping table.)
- Useful for building constant vectors purely in-register (no memory access):
  `esp.zero.q q7` + `esp.vsadds.u16 q7, q7, 255` = [255 x 8] broadcast
- Verified with 13 boundary-condition tests on hardware (229/229 pass).

**vldbc.8/16/32 broadcast load — most efficient for runtime variables:**
- `esp.vldbc.8.ip qd, addr, 0` reads 1 byte, broadcasts to all 16 u8 lanes
- `esp.vldbc.16.ip qd, addr, 0` reads 1 halfword, broadcasts to all 8 u16 lanes
- Far cheaper than building a 16-byte broadcast buffer: 1 memory op vs 8+ stores
- Measured 18% improvement (2.58x → 3.04x) on deblocking filter thresholds

**Unaligned loads work via usar + src.q pattern:**
- `esp.ld.128.usar.ip q0, ptr, 16` — loads from aligned-down address, sets SAR
- `esp.ld.128.usar.ip q1, ptr+16, 0` — loads next aligned block
- `esp.src.q q2, q0, q1` — extracts 16 unaligned bytes using SAR offset
- Tested at offsets 0, 1, 3, 7, 13 — all correct

**vzip.8 / vunzip.8 match S3 TRM:**
- `vzip.8 qd, qs`: interleaves lower 8 elements of each into qd, upper 8 into qs
- `vunzip.8 qd, qs`: exact inverse — separates even/odd indexed elements
- Used for u8↔i16 widening/narrowing: `vzip.8(data, zeros)` widens,
  `vunzip.8(data_lo, data_hi)` narrows

**vcmp + andq for vectorized threshold masks:**
- `esp.vcmp.lt.u8 q_dst, q_a, q_b` — sets each byte to 0xFF if a<b, else 0x00
- `esp.andq q_dst, q_a, q_b` — bitwise AND of two Q registers
- Combined with `vsub.u8` + `vmax.u8` for absolute difference:
  `|a-b| = max(a -sat b, b -sat a)`

### Architecture Limitations Discovered

1. **Only 8 vector registers (q0-q7)**: Severe register pressure. Recommend
   reserving q6-q7 for constants (zero, 255) and using q0-q5 for data.

2. **No lane shuffle/permute**: Can't rearrange elements within a register. This
   prevents vectorizing operations that need cross-lane data or non-power-of-2
   interleave patterns (like PPA's 3:1 chroma:luma ratio).

3. **No i16 shift instructions**: `vsr.s32` and `vsl.32` exist for 32-bit lanes
   only. For i16 shifts, must use the QACC path or scalar.

4. **16-byte alignment required**: All loads/stores silently mask the bottom 4
   address bits. Unaligned access requires the 3-instruction usar+src.q pattern.

5. **Scalar register constraint**: Only x8-x15 (6 usable: a0-a5) for PIE
   instruction operands. Plan register allocation carefully for single asm blocks.

---

## When to Use SIMD on ESP32-P4

**SIMD wins when ALL of these are true:**
- Row width ≥ 16 bytes (fills a full 128-bit vector register)
- Contiguous or easily addressable data (no scattered gather/scatter)
- Uniform compute: same operation applied to every element
- Enough rows/iterations to amortize widening/narrowing and setup cost
- No per-element branching, OR branching can be hoisted to a SIMD mask phase

**SIMD loses when ANY of these are true:**
- Working set < 64 bytes (overhead exceeds computation)
- Data requires non-power-of-2 rearrangement (no shuffle/permute)
- Per-element different operations (different multipliers, permutations)
- Strided access with stride ≠ element width (gather/scatter needed)

**Hybrid SIMD+scalar works when:**
- Threshold/comparison logic is uniform but filter logic is conditional
- SIMD computes the mask, scalar applies the filter only where needed
- The common case is "most elements don't need processing" (early-out wins)

---

## Reference Files

| File | Purpose |
|---|---|
| `main/pie_tests.c` | PIE instruction reference & verification (35 tests, all passing) |
| `main/h264_tests.c` | H264 function correctness + performance benchmark harness |
| `main/h264_test_helpers.h` | Shared types, macros, test structures |
| `main/write_output_blocks_simd_p4.c` | 4.52x SIMD: widen/add/clamp/narrow pipeline |
| `main/deblock_filter_simd_p4.c` | 2.58x hybrid: SIMD threshold mask + scalar filter |
| `main/convert_ppa_simd_p4.c` | 1.06x scalar word-pack (PIE not applicable) |
| `main/test_runner.c` | Top-level test orchestration + SIMD memcpy/memset benchmarks |

## Documentation References

| Source | Location |
|---|---|
| xesppie.S instruction list | `tanmatsu-launcher/esp-idf/.../rv_decode/xesppie.S` |
| ESP32-S3 TRM Chapter 8 | Best PIE behavioral reference (translate `ee.*` → `esp.*`) |
| Espressif PIE blog | https://developer.espressif.com/blog/2024/12/pie-introduction/ |
| PIE memcpy benchmark | https://gist.github.com/BitsForPeople/b030f9b6417e4091b955c00fd55ddcc0 |
