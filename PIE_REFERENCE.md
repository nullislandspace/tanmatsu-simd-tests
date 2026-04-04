# ESP32-P4 PIE SIMD Instruction Reference

Verified on hardware, 229/229 tests passing. This is the only known comprehensive
reference for the P4 PIE instruction set — Espressif has not published documentation.

## Architecture

- **8 x 128-bit vector registers**: q0-q7
- **Element types**: 16 x u8, 8 x s16, 4 x s32 per register
- **256-bit accumulator**: QACC (8 x 32-bit lanes for multiply-accumulate)
- **40-bit scalar accumulator**: XACC (32-bit low + 8-bit high)
- **Shift Amount Register**: SAR (controls shift distances)
- **Memory alignment**: all load/store addresses MUST be 16-byte aligned
- **Register constraint**: inline asm operands must use x8-x15 only (a0-a5, s0, s1)
- **Enabling**: PIE auto-enabled by FreeRTOS lazy coprocessor switching

## Notation

```
qd, qs, qt    = destination/source Q registers (q0-q7)
rs, rd        = scalar RISC-V registers (must be a0-a5, s0, s1)
addr          = base address register (must be a0-a5)
imm           = immediate value
SAR           = Shift Amount Register (set via esp.movx.w.sar)
QACC          = 256-bit accumulator (8 x 32-bit lanes)
XACC          = 40-bit cross-lane accumulator
.ip           = immediate post-increment (addr += imm after operation)
.xp           = register post-increment (addr += rs after operation)
```

---

## Load / Store

### esp.vld.128.ip qd, addr, imm
Load 128 bits (16 bytes) from aligned memory into qd. addr += imm.

### esp.vld.128.xp qd, addr, rs
Load 128 bits from aligned memory into qd. addr += rs.

### esp.vst.128.ip qs, addr, imm
Store 128 bits from qs to aligned memory. addr += imm.

### esp.vst.128.xp qs, addr, rs
Store 128 bits from qs to aligned memory. addr += rs.

### esp.vld.l.64.ip qd, addr, imm
Load 64 bits into LOWER half of qd (bytes 0-7). Upper half unchanged. addr += imm.

### esp.vld.l.64.xp qd, addr, rs
Load 64 bits into LOWER half of qd. addr += rs.

### esp.vld.h.64.ip qd, addr, imm
Load 64 bits into UPPER half of qd (bytes 8-15). Lower half unchanged. addr += imm.

### esp.vld.h.64.xp qd, addr, rs
Load 64 bits into UPPER half of qd. addr += rs.

### esp.vst.l.64.ip qs, addr, imm
Store LOWER 64 bits of qs to memory. addr += imm.

### esp.vst.l.64.xp qs, addr, rs
Store LOWER 64 bits of qs to memory. addr += rs.

### esp.vst.h.64.ip qs, addr, imm
Store UPPER 64 bits of qs to memory. addr += imm.

### esp.vst.h.64.xp qs, addr, rs
Store UPPER 64 bits of qs to memory. addr += rs.

### esp.vldbc.8.ip qd, addr, imm
Broadcast-load: read 1 byte, replicate to all 16 bytes of qd. addr += imm.
**Optimization use**: excellent for broadcasting runtime variables (e.g., threshold
values) to all lanes. Only reads 1 byte from memory — far cheaper than storing a
16-byte broadcast buffer then loading it with vld.128. Measured 18% speedup
(2.58x → 3.04x) on deblocking filter when replacing word-fill broadcast with vldbc.8.

### esp.vldbc.8.xp qd, addr, rs
Broadcast-load 1 byte. addr += rs.

### esp.vldbc.16.ip qd, addr, imm
Broadcast-load: read 1 half-word (16-bit), replicate to all 8 lanes of qd. addr += imm.

### esp.vldbc.16.xp qd, addr, rs
Broadcast-load 1 half-word. addr += rs.

### esp.vldbc.32.ip qd, addr, imm
Broadcast-load: read 1 word (32-bit), replicate to all 4 lanes of qd. addr += imm.

### esp.vldbc.32.xp qd, addr, rs
Broadcast-load 1 word. addr += rs.

### esp.vldext.u8.ip qd0, qd1, addr, imm
Widening load: read 8 bytes, zero-extend each to u16. qd0 gets 8 x u16. addr += imm.

### esp.vldext.s8.ip qd0, qd1, addr, imm
Widening load: read 8 bytes, sign-extend each to s16. qd0 gets 8 x s16. addr += imm.

### esp.vldext.u16.ip qd0, qd1, addr, imm
Widening load: read 4 half-words, zero-extend each to u32. qd0 gets 4 x u32. addr += imm.

### esp.vldext.s16.ip qd0, qd1, addr, imm
Widening load: read 4 half-words, sign-extend each to s32. qd0 gets 4 x s32. addr += imm.

### esp.vldhbc.16.incp qd, qt, addr
FFT twiddle broadcast: loads two consecutive 16-bit values from addr.
Broadcasts hw1 to qd lanes 0-1, hw2 to qd lanes 2-3. Upper 8 bytes of qd are zero.
qt purpose unknown (outputs zero). addr incremented by 4.

### esp.ld.128.usar.ip qd, addr, imm
Unaligned load: loads 16 bytes from potentially unaligned address.
Sets internal USAR state (alignment offset). Use with esp.src.q to extract aligned result.
addr += imm.

### esp.ld.128.usar.xp qd, addr, rs
Unaligned load with register post-increment. addr += rs.

### esp.ldqa.s16.128.ip addr, imm
Load 8 x s16 directly into QACC (sign-extended to 32-bit lanes). addr += imm.
Round-trips perfectly: ldqa → srcmb.s16.qacc returns original values.

### esp.ldqa.u16.128.ip addr, imm
Load 8 x u16 directly into QACC. addr += imm.

### esp.ldqa.u8.128.ip addr, imm
Load 16 x u8 into QACC. Exact lane mapping needs further investigation. addr += imm.

### esp.ldqa.s8.128.ip addr, imm
Load 16 x s8 into QACC. addr += imm.

### esp.ld.qacc.{l.l|l.h|h.l|h.h}.128.ip addr, imm
Load 128 bits into one quadrant of QACC (256-bit). Round-trips with matching st.qacc.

### esp.st.qacc.{l.l|l.h|h.l|h.h}.128.ip addr, imm
Store one quadrant of QACC to memory.

### esp.ld.xacc.ip addr, imm
Load XACC (40-bit accumulator) from memory. Only lower 5 bytes are meaningful. addr += imm.

### esp.st.s.xacc.ip addr, imm
Store XACC as signed to memory (5 bytes). addr += imm.

### esp.st.u.xacc.ip addr, imm
Store XACC as unsigned to memory (5 bytes). addr += imm.

### esp.ldxq.32 qd, qs, addr, imm1, imm2
Load with lane extraction from Q register. Exact semantics unclear — produces non-zero output related to instruction encoding.

### esp.srcq.128.st.incp qd, qs, addr
Unaligned source + store with post-increment. Combines two usar-loaded registers into aligned output and stores. addr incremented by 16.

### esp.ld.ua.state.ip addr, imm
Load unaligned-access state from memory. addr += imm.

### esp.st.ua.state.ip addr, imm
Store unaligned-access state to memory. addr += imm.

---

## Zero

### esp.zero.q qd
Zero all 128 bits of qd.

### esp.zero.qacc
Zero all 256 bits of QACC.

### esp.zero.xacc
Zero all 40 bits of XACC.

---

## Arithmetic

**All arithmetic is SATURATING on P4. No wrapping variants exist.**

### esp.vadd.{s32|s16|s8|u32|u16|u8} qd, qs, qt
Vector add: qd[i] = sat(qs[i] + qt[i])

### esp.vsub.{s32|s16|s8|u32|u16|u8} qd, qs, qt
Vector subtract: qd[i] = sat(qs[i] - qt[i])

### esp.vsadds.{s16|s8|u16|u8} qd, qs, rs
Scalar add to all lanes: qd[i] = sat(qs[i] + truncate(rs))

**P4 finding — scalar truncation**: The scalar register value is **truncated to the
lane width** before the addition. The full 32-bit register value is NOT used.
- `.u16` / `.s16`: uses low 16 bits of rs (rs & 0xFFFF)
- `.u8` / `.s8`: uses low 8 bits of rs (rs & 0xFF)

Consequences:
- Passing 0x10000 to `.u16` adds 0 (not 65536) — identity operation
- Passing 256 to `.u8` adds 0 (not 256) — identity operation
- Passing -1 (0xFFFFFFFF) to `.u16` adds 0xFFFF = 65535 (unsigned truncation)

**Broadcast pattern**: To broadcast a scalar to all lanes of a Q register:
```asm
esp.zero.q qd
esp.vsadds.u16 qd, qd, rs   /* qd[i] = 0 + truncate_u16(rs) for all i */
```
Verified: `vsadds.u16(zeros, 255)` correctly produces [255, 255, 255, 255, 255, 255, 255, 255].

**Alternative broadcast** using vldbc (loads from memory, but only 1 byte/halfword):
```asm
esp.vldbc.8.ip qd, addr, 0   /* broadcast byte at addr to all 16 u8 lanes */
esp.vldbc.16.ip qd, addr, 0  /* broadcast halfword at addr to all 8 u16 lanes */
```
vldbc is preferred when the value is already in memory; vsadds when it's in a register.

### esp.vssubs.{s16|s8|u16|u8} qd, qs, rs
Scalar subtract from all lanes: qd[i] = sat(qs[i] - truncate(rs))

Same scalar truncation rules as vsadds: the scalar operand is truncated to lane
width before subtraction.

### esp.addx2 rd, rs1, rs2
Scalar: rd = rs1 + (rs2 << 1). Verified: addx2(100, 25) = 150.

### esp.addx4 rd, rs1, rs2
Scalar: rd = rs1 + (rs2 << 2). Verified: addx4(100, 25) = 200.

### esp.subx2 rd, rs1, rs2
Scalar: rd = rs1 - (rs2 << 1). Verified: subx2(100, 25) = 50.

### esp.subx4 rd, rs1, rs2
Scalar: rd = rs1 - (rs2 << 2). Verified: subx4(200, 25) = 100.

### esp.sat rd, rs1, rs2
Scalar saturation. Exact formula unclear. Verified: sat(300, 8) = 34.

---

## Multiply

### esp.vmul.{s16|s8|u16|u8} qd, qs, qt
Lane-wise saturating multiply: qd[i] = sat(qs[i] * qt[i]).
**P4 does NOT apply >>4 shift** (differs from ESP32-S3).
Saturation to type range (e.g., s16: -32768..32767).

### esp.vmul.s16.s8xs8 qd, qs, qt, qx
Widening multiply: s8 * s8 → s16 products.
**P4 finding**: zeros qs (source register) as side effect. Products likely accumulate into QACC.
qd and qx outputs are zero. Use vmulas.s8.qacc + srcmb for widening multiply instead.

### esp.vmul.s32.s16xs16 qd, qs, qt, qx
Widening multiply: s16 * s16 → s32 products.
**P4 finding**: same as s8xs8 — zeros qs, products go to QACC. Use vmulas.s16.qacc + srcmb.

### esp.vmulas.s16.qacc qs, qt
Multiply-accumulate into QACC: QACC[i] += qs[i] * qt[i] (lane-wise, 8 lanes).
Extract results via esp.srcmb.s16.qacc. Low 16 bits extracted (truncation, not saturation).

### esp.vmulas.s8.qacc qs, qt
Multiply-accumulate s8 into QACC. Uses EVEN-indexed products only:
QACC[i] = qs[2i] * qt[2i] for i=0..7.

### esp.vmulas.u16.qacc qs, qt
Multiply-accumulate u16 into QACC. Lane-wise. srcmb extraction truncates to low 16 bits.

### esp.vmulas.u8.qacc qs, qt
Multiply-accumulate u8 into QACC. Even-indexed products only.

### esp.vmulas.{s16|s8|u16|u8}.xacc qs, qt
Cross-lane dot product: XACC += sum(qs[i] * qt[i]) for all lanes.
Result is a single 40-bit scalar. Extract via esp.srs.s.xacc or esp.srs.u.xacc.

### esp.vcmulas.s16.qacc.l qs, qt
Complex multiply-accumulate (lower pairs). Treats s16 pairs as complex: (qs[2i] + qs[2i+1]*j).
Processes lower complex pairs into QACC.
Verified: (3+4i)*(2+0i) → QACC lanes 0,1 = 6, 8.

### esp.vcmulas.s16.qacc.h qs, qt
Complex multiply-accumulate (upper pairs). Same as .l but processes upper half.
Verified: product appears at QACC lanes 4,5.

### esp.vcmulas.s8.qacc.l qs, qt
Complex multiply-accumulate s8, lower pairs. Only real part in lane 0 observed.

### esp.vcmulas.s8.qacc.h qs, qt
Complex multiply-accumulate s8, upper pairs. Product appears at QACC lane 4.

### esp.vsmulas.{s16|s8|u16|u8}.qacc qs, qt, imm
Scalar-vector multiply-accumulate: multiplies each qs[i] by qt[imm] (the immediate selects which element of qt to use as the scalar multiplier), accumulates into QACC.
Verified with imm=4: vsmulas.s16 with a={16,32,...}, b={1,2,4,8,16,...} → out[0]=256=16*16=a[0]*b[4].

### esp.cmul.{s16|s8|u16|u8} qd, qs, qt, mode
Complex multiply: treats element pairs as (real, imag).
**Processes first 2 complex pairs only** (4 elements, 8 bytes). Upper bytes of qd not written.
mode=0: standard complex multiply. No >>4 shift on P4.
Formula: out_re = a_re*b_re - a_im*b_im, out_im = a_re*b_im + a_im*b_re.
Saturates to type range.

---

## Compare

### esp.vcmp.eq.{s32|s16|s8|u32|u16|u8} qd, qs, qt
Lane-wise compare equal: qd[i] = (qs[i] == qt[i]) ? ALL_ONES : 0

### esp.vcmp.gt.{s32|s16|s8|u32|u16|u8} qd, qs, qt
Lane-wise compare greater: qd[i] = (qs[i] > qt[i]) ? ALL_ONES : 0

### esp.vcmp.lt.{s32|s16|s8|u32|u16|u8} qd, qs, qt
Lane-wise compare less: qd[i] = (qs[i] < qt[i]) ? ALL_ONES : 0

### esp.vmax.{s32|s16|s8|u32|u16|u8} qd, qs, qt
Lane-wise maximum: qd[i] = max(qs[i], qt[i])

### esp.vmin.{s32|s16|s8|u32|u16|u8} qd, qs, qt
Lane-wise minimum: qd[i] = min(qs[i], qt[i])

### esp.max.{s32|s16|s8|u32|u16|u8}.a qs, rs
Reduce-max: rs = max(rs, max(qs[0..N])). Initialize rs to type minimum before use.

### esp.min.{s32|s16|s8|u32|u16|u8}.a qs, rs
Reduce-min: rs = min(rs, min(qs[0..N])). Initialize rs to type maximum before use.

---

## Shift / Slide

### esp.vsr.s32 qd, qs
Arithmetic right shift each s32 element by SAR bits. Set SAR first via esp.movx.w.sar.

### esp.vsr.u32 qd, qs
Logical right shift each u32 element by SAR bits.

### esp.vsl.32 qd, qs
Left shift each 32-bit element by SAR bits.

### esp.vsld.{8|16|32} qd, qs, qt
Vector slide left: SAR-controlled bit shift across concatenated qs:qt.
**P4 finding**: SAR is in bits. Use SAR=8 for .8 and .16 widths, SAR=32 for .32.
SAR=16 is a no-op for .16 (full rotation). SAR=4 is also no-op for .16.

### esp.vsrd.{8|16|32} qd, qs, qt
Vector slide right: SAR-controlled bit shift across concatenated qs:qt.
Same SAR requirements as vsld.

### esp.srci.2q qd, qs, imm
Shift qd right by (imm+1) bytes, zero-fill. **qs is completely ignored** — tested
with various qs contents, output is always qd shifted right with zeros, regardless of qs.
Result stored in qd (modified in-place).

Verified: q0=[16..31], q1=[0..15], `srci.2q q0, q1, 0` → [17,18,...,31, 0] (zero, not q1 data).

### esp.slci.2q qd, qs, imm
Shift qd left by (imm+1) bytes. Fills vacated low bytes from the **tail** (high bytes)
of qs. Result stored in qd.

Verified: q0=[0..15], q1=[16..31], `slci.2q q0, q1, 2` → [29,30,31, 0,1,...,12].
Takes last 3 bytes of q1 (29,30,31) and prepends them, shifts q0 data left.

**Note**: fills from qs tail, not head. Not useful as a right-shift-with-carry.

### esp.srcxxp.2q qd, qs, rs1, rs2
Register-controlled version of srci.2q. Shift qd right by (rs1+1) bytes, zero-fill.
Same behavior as srci.2q — qs is ignored. Verified on hardware.

### esp.slcxxp.2q qd, qs, rs1, rs2
Register-controlled version of slci.2q. Shift left, fill from qs tail.

### esp.src.q qd, qs, qt
Unaligned source combine: extracts aligned 128-bit result from two usar-loaded registers.
Uses internal USAR alignment state set by esp.ld.128.usar.ip.
Verified at offsets 0, 1, 3, 7, 13.

**CRITICAL P4 finding**: `src.q` does NOT use the SAR register set by `movx.w.sar`.
It uses a **separate internal alignment state** that is ONLY set by `usar` loads.
Setting SAR via `movx.w.sar` has no effect on `src.q` — all extractions produce the
same result regardless of SAR value. Tested with SAR=0,1,2,4,6,8,10,15: all produced
identical output determined by the last `usar` load's alignment offset.

Additionally, `usar` loads **clobber the SAR register** — after an usar load, SAR
contains garbage (e.g., 1378). Do not rely on SAR being preserved across usar loads.

Consequence: there is no way to extract arbitrary byte-shifted views from Q registers
using only register operations. Each shifted view requires a separate `usar` load from
a different memory address.

### esp.src.q.qup qd, qs, qt
Variant of src.q with queue-up semantics. Same unaligned extraction.

### esp.srcmb.s16.qacc qd, rs, imm
Extract 8 x s16 from QACC. SAR (set via rs or movx.w.sar) controls extraction point.
With SAR=0: extracts low 16 bits of each 32-bit QACC lane (truncation, not saturation).

### esp.srcmb.s8.qacc qd, rs, imm
Extract from QACC as s8 values. Output: products in even bytes, zero in odd bytes.

### esp.srcmb.u16.qacc qd, rs, imm
Extract from QACC as u16. Same as s16 variant.

### esp.srcmb.u8.qacc qd, rs, imm
Extract from QACC as u8. Same layout as s8 variant.

### esp.srcmb.{s16|s8|u16|u8}.q.qacc qd, qs, imm
Extract from QACC with Q register input. The Q register modifies extraction.
Produces non-zero output but exact semantics differ from plain srcmb.

### esp.srs.s.xacc rd, rs
Shift-and-round signed: extract XACC value with right-shift by rs bits. Result in rd.

### esp.srs.u.xacc rd, rs
Shift-and-round unsigned: extract XACC value with right-shift by rs bits.

---

## Logical

### esp.andq qd, qs, qt
Bitwise AND: qd = qs & qt (128-bit)

### esp.orq qd, qs, qt
Bitwise OR: qd = qs | qt

### esp.xorq qd, qs, qt
Bitwise XOR: qd = qs ^ qt

### esp.notq qd, qs
Bitwise NOT: qd = ~qs

---

## Data Reformat

### esp.vzip.{8|16|32} qd, qs
Interleave: splits qd and qs elements into interleaved pairs.
For .8: qd gets low-half interleave, qs gets high-half interleave.
Modifies BOTH qd and qs in-place. Verified exact byte patterns.

### esp.vunzip.{8|16|32} qd, qs
Deinterleave: inverse of vzip. Modifies BOTH qd and qs in-place.

### esp.vzipt.{8|16} qd, qs, qt
Transposed zip: writes interleaved result to qd (third register), leaving qs/qt unchanged.
Different interleave pattern from vzip — includes zero-padding between elements.

### esp.vunzipt.{8|16} qd, qs, qt
Transposed unzip: writes deinterleaved result to qd.

### esp.vext.{u8|s8|u16|s16} qd, qs, qt
Widening extension: zero/sign-extends elements from qs.
**P4 finding**: result appears in qt (third register), NOT qd. qd output is mostly zero.

### esp.vabs.{8|16|32} qd, qs
Absolute value: qd[i] = |qs[i]|. Saturating (e.g., |-128| = 127 for s8).

### esp.vsat.{s32|s16|s8|u32|u16|u8} qd, qs, rs_lo, rs_hi
Clamp to range: qd[i] = clamp(qs[i], rs_lo, rs_hi).

### esp.vclamp.s16 qd, qs, imm
Clamp s16 to symmetric range: qd[i] = clamp(qs[i], -(1<<imm), (1<<imm)-1).
Verified: imm=7 clamps to [-128, 127].

### esp.vrelu.{s16|s8} qd, rs_threshold, rs_max
ReLU activation: if element <= threshold, set to 0; else clamp to max.
**P4 finding**: for s16, max parameter may not clamp as expected at small values.
Use rs_max=32767 for s16 to avoid issues.

### esp.vprelu.{s16|s8} qd, qs, qt, rs_shift
Parametric ReLU: if element >= 0, pass through; else multiply by slope and shift.
qd = output, qs = input, qt = slope vector, rs_shift = right-shift amount.

---

## Move / Config

### esp.mov.{s16|s8|u16|u8}.qacc qs
Move Q register contents into QACC with type interpretation.
Verified: mov.s16.qacc round-trips (load → mov → srcmb → store = original values).

### esp.movi.32.a rd, qs, imm
Extract 32-bit lane from Q register: rd = qs[imm]. imm selects lane 0-3.

### esp.movi.32.q qd, rs, imm
Insert 32-bit value into Q register lane: qd[imm] = rs.

### esp.movi.16.a rd, qs, imm
Extract 16-bit lane: rd = qs[imm] (zero-extended to 32 bits).

### esp.movi.16.q qd, rs, imm
Insert 16-bit value into lane: qd[imm] = rs (low 16 bits).

### esp.movi.8.a rd, qs, imm
Extract byte: rd = qs[imm] (zero-extended).

### esp.movi.8.q qd, rs, imm
Insert byte: qd[imm] = rs (low 8 bits).

### esp.movx.w.sar rs
Write SAR (Shift Amount Register): SAR = rs.

### esp.movx.r.sar rd
Read SAR: rd = SAR.

### esp.movx.w.sar.bytes rs
Write SAR in byte units: SAR = rs * 8.

### esp.movx.r.sar.bytes rd
Read SAR in byte units: rd = SAR / 8.

### esp.movx.w.cfg rs
Write PIE configuration register.

### esp.movx.r.cfg rd
Read PIE configuration register.

### esp.movx.w.fft.bit.width rs
Write FFT bit-width register (controls fft.bitrev).

### esp.movx.r.fft.bit.width rd
Read FFT bit-width register.

### esp.movx.w.perf rs
Write performance counter selector.

### esp.movx.r.perf rd, rs
Read performance counter: rd = perf_counter[rs].

### esp.movx.w.xacc.h rs
Write XACC high byte: XACC[39:32] = rs[7:0]. Only low 8 bits used (XACC is 40-bit).

### esp.movx.r.xacc.h rd
Read XACC high byte: rd = XACC[39:32] (zero-extended).

### esp.movx.w.xacc.l rs
Write XACC low 32 bits: XACC[31:0] = rs.

### esp.movx.r.xacc.l rd
Read XACC low 32 bits: rd = XACC[31:0].

---

## FFT

### esp.fft.bitrev qd, addr
Bit-reversal permutation for FFT index generation.
**P4 finding**: outputs bit-reversed INDICES (not reordered data).
Set bit_width first via esp.movx.w.fft.bit.width.
With bit_width=3: index 1→4 (001→100), index 2→2 (010→010), index 3→6 (011→110).

### esp.fft.r2bf.s16 qd, qs, qt, qx, mode
Radix-2 butterfly for FFT.
**P4 finding**: sum output (qd) = (qs + qt) / 2 (NOT qs + qt). Division by 2 prevents overflow.
diff output (qx) was all zero in testing.

### esp.fft.ams.s16.ld.incp qd_load, addr, qd_out, qs, qt, qx_twiddle, qx_out, mode
FFT add-multiply-subtract with fused load. Complex multi-output instruction.

### esp.fft.cmul.s16.ld.xp qd_load, addr, stride, qs, qt, qd_out, mode
FFT complex multiply with fused load. addr += stride.

### esp.fft.vst.r32.decp qs, addr, mode
FFT store with decrementing pointer. Writes data and decrements addr.

---

## Hardware Loop

### esp.lp.counti loop_id, imm
Set hardware loop counter to immediate value. loop_id = 0 or 1.

### esp.lp.count loop_id, rs
Set hardware loop counter from register.

### esp.lp.starti loop_id, imm
Set hardware loop start address offset.

### esp.lp.endi loop_id, imm
Set hardware loop end address offset.

### esp.lp.setupi loop_id, count, body_size
Combined loop setup with immediate count and body size.
**Note**: body_size is in bytes, not instructions. Follow with exactly body_size bytes of code.

### esp.lp.setup loop_id, rs, body_size
Combined loop setup with register count and immediate body size.
