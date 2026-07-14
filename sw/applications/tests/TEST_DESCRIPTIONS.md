# Test Suite Descriptions

This document describes each test in `sw/applications/tests/`, covering what it measures, how it exercises the hardware accelerator (when applicable), and which performance bottleneck it is designed to expose.

---

## Keccak / Hash-based Tests

### `keccak-abs` — Keccak Absorption & Squeeze Characterization

Tests the SHA3 and SHAKE family of hash functions as implemented in `fips202`. Each sub-test isolates a specific operational mode and input/output size combination to characterize where computation time is spent in the Keccak sponge at different points of use in PQC algorithms.

Cycle counts are collected via the `mcycle` CSR before and after each call, enabling direct comparison of cost across variants.

#### `test_sha3_256_single_block`
Hashes the 3-byte message `"abc"` with SHA3-256 and compares against the known NIST test vector. Because the input is far smaller than the SHA3-256 rate (136 bytes), only a single Keccak-f\[1600\] permutation is required during absorption. This test establishes the baseline cost of one absorption block: it isolates the permutation latency from any padding or multi-block overhead.

#### `test_sha3_512_single_block`
Hashes the same `"abc"` message with SHA3-512. SHA3-512 has a narrower rate (72 bytes) and a larger capacity than SHA3-256, making its single-block call more expensive. Comparing the cycle count of this test against `test_sha3_256_single_block` quantifies the cost of increasing the security level through reduced rate.

#### `test_sha3_256_multi_block`
Hashes a 200-byte input with SHA3-256. Since 200 bytes exceeds the SHA3-256 rate (136 bytes), two Keccak-f permutations are required during absorption. This test reveals the **multi-block absorption bottleneck**: the linear increase in permutation invocations when hashing messages longer than one rate block, which is representative of long-key or certificate hashing scenarios.

#### `test_shake128`
Runs SHAKE128 with a 50-byte input and a 32-byte output. SHAKE128 has the widest rate among the tested functions (168 bytes), so both absorption and squeezing fit within a single permutation. The test measures the round-trip cost of the SHAKE128 XOF in its cheapest configuration, serving as the baseline for XOF-based key derivation.

#### `test_shake256`
Runs SHAKE256 with a 100-byte input and a 64-byte output. SHAKE256 has the same 136-byte rate as SHA3-256 but uses a different domain separator and is an extendable-output function. Comparing this test against `test_shake128` shows the rate-vs-security tradeoff within the XOF family, relevant for SPHINCS+ PRF and PRFmsg calls.

#### `test_multi_squeeze`
Runs SHAKE128 on a 50-byte input requesting a 500-byte output. Generating 500 bytes from a 168-byte rate requires multiple squeeze permutations (⌈500/168⌉ = 3 additional permutations beyond the first squeeze block). This test exposes the **output-side squeeze bottleneck**: in operations that demand long pseudorandom byte streams — such as Kyber's matrix expansion (`gen_matrix`) or SPHINCS+ pseudorandom key generation — the squeezing phase dominates total Keccak cost.

---

## SPHINCS+ Hash Layer Tests

### `thash` — Tweakable Hash with 1-Block Input (THASH1)

Tests the SPHINCS+ tweakable hash function `thash` with a single N-byte input block, comparing software (`thash_sw`) and hardware (`thash_hw`) implementations. This corresponds to the hash calls made during WOTS+ chain computation.

- **Test 1 — Basic**: single `thash(input, 1)` call against a reference test vector; validates correctness of both SW and HW paths and records per-call cycle counts.
- **Test 2 — Variant**: same structure with a different input and address, providing robustness coverage against input-dependent bugs.
- **Test 3 — 5-step chain**: five sequential `thash` calls where the output of each step becomes the input to the next and the hash address counter is incremented. Measures the aggregate cost of a partial WOTS chain and allows deriving the per-call overhead including the address update inside the loop.
- **Test 4 — Full WOTS chain (15 steps)**: runs `SPX_WOTS_W − 1 = 15` sequential `thash` calls, reproducing the workload of a complete WOTS+ chain evaluation during signing or verification. Cycle count scaled by `1/chain_len` gives the true per-call latency under repeated-invocation conditions.

### `thash2` — Tweakable Hash with 2-Block Input (Merkle Node Hashing)

Tests `thash` with a 2×N-byte input, which is used when computing Merkle tree internal nodes from a pair of children: `parent = thash(left_child ∥ right_child)`. The 2-block input requires an additional Keccak absorption step compared to THASH1.

- **Test 1 — Basic**: single `thash(input, 2)` call; validates correctness and measures the incremental cost vs THASH1.
- **Test 2 — Variant**: second input/address pair for robustness.
- **Test 3 — Merkle tree level**: three sequential `thash2` calls computing the root of a 4-leaf subtree (two parent nodes at height 0, one root at height 1). Reproduces the latency pattern of one level of Merkle tree construction in SPHINCS+.

### `thash-wots` — Combined THASH and WOTS Chain Test

Exercises both THASH1 (`thash_1blk`, `thash_2blk`) and the full `gen_chain` function from `wots_chain_sw/hw`, which internally loops over `thash` calls:

- **thash\_1blk / thash\_2blk**: single-step verification matching `thash` and `thash2`.
- **chain\_1step**: `gen_chain` with one iteration — isolates the function call and address management overhead of the chain loop.
- **chain\_full**: `gen_chain` with `SPX_WOTS_W − 1 = 15` iterations — full single-chain cost for one WOTS+ symbol.
- **multi\_chain**: runs `gen_chain` for all `SPX_WOTS_LEN` WOTS symbols with varying start positions and step counts, replicating the complete WOTS+ signing or verification workload. Counts total `thash` invocations and compares SW vs HW throughput across the whole WOTS key-derivation sequence.

### `prf-addr` — SPHINCS+ PRF for WOTS+ Key Derivation

Tests the SPHINCS+ pseudorandom function `PRF(sk_seed, addr) = SHAKE256(sk_seed ∥ addr)[0:N]`, which is called once per WOTS+ chain to derive the secret chain input from the secret seed and the address structure.

- **Test 1 — Basic**: single PRF call against a reference test vector; verifies that domain separation via the address structure produces the correct N-byte output.
- **Test 2 — Variant**: different address, tests that distinct addresses produce distinct outputs.
- **Test 3 — 4-key chain generation**: four sequential PRF calls updating the chain address index, simulating keygen for a 4-element WOTS subset.
- **Test 4 — Full WOTS keygen**: `SPX_WOTS_LEN` PRF calls (one per chain symbol), reproducing the complete WOTS+ key-generation kernel and measuring the Keccak-driven key expansion cost at full scale.

### `chain-lengths` — SPHINCS+ WOTS+ Message Encoding

Tests the `chain_lengths` function, which converts a raw message hash into the sequence of base-`w` digits (and their checksum) that determine the start position and number of steps for each WOTS+ chain during signing. Variants are tested for all three SPHINCS+ security levels:

- **128f** (`len1 = 32`): 32 message nibbles + 3 checksum nibbles.
- **192f** (`len1 = 48`): 48 + 3 nibbles.
- **256f** (`len1 = 64`): 64 + 3 nibbles.

Software and hardware implementations are compared on correctness and cycle count for each variant, with speedup reported.

---

## Lattice Arithmetic Tests (BFU Accelerator)

All lattice tests follow the same structure: a software reference runs the target operation using the standard C/RISC-V integer implementation; then the hardware path uses custom RISC-V instructions that invoke the Butterfly Function Unit (BFU). Both results are compared against golden vectors, and `mcycle` counts are used to compute the speedup. These tests individually validate each operation supported by the shared BFU coprocessor.

### `dilithium-ntt` — Dilithium Number Theoretic Transform

Runs the NTT over `Z_{q_D}` (`q_D = 8380417`) on a 256-coefficient polynomial for the ML-DSA / Dilithium signature scheme. The HW path uses the custom `OP_NTT_DSA` butterfly instruction inside the standard Cooley-Tukey radix-2 DIT loop. Validates all 256 output coefficients against a precomputed golden output.

### `dilithium-intt` — Dilithium Inverse NTT

Runs the INTT (Gentleman-Sande radix-2 DIF) over `Z_{q_D}` for Dilithium, restoring a polynomial from NTT domain. The HW butterfly instruction includes the Montgomery scaling factor applied at exit. Validates against a golden inverse-transformed polynomial.

### `dilithium-reduce32` — Dilithium Modular Reduction

Tests the scalar operations `reduce32` (Barrett reduction to the range `(-q_D/2, q_D/2]`) and `caddq` (conditional addition of `q_D` to map to `[0, q_D)`) on a set of 20 representative scalar values. The HW path uses dedicated single-cycle BFU instructions for each operation, exposing the benefit of reducing scalar reduction overhead in, e.g., coefficient normalization during signing.

### `kyber-ntt` — Kyber Number Theoretic Transform

Runs the NTT over `Z_{q_K}` (`q_K = 3329`) on a 256-coefficient polynomial for ML-KEM / Kyber. Uses the Kyber-specific butterfly instruction and twiddle factor table. Validates all 256 output coefficients.

### `kyber-intt` — Kyber Inverse NTT

Runs the INTT over `Z_{q_K}` for Kyber. Includes the Montgomery correction applied at the end of the INTT to remove the `2^16` accumulation from the butterfly. Validates against the golden reconstructed polynomial.

### `fqmul` — Kyber Montgomery Multiplication

Tests `fqmul(a, b) = a·b·R^{-1} mod q_K` directly on 5–7 hand-crafted scalar pairs covering zero, positive, and negative operands. The hardware path uses the BFU's dedicated `OP_FQMUL` instruction. This operation is the innermost step of every Kyber NTT butterfly.

### `falcon-ntt` — Falcon Number Theoretic Transform

Runs the NTT over `Z_{q_F}` (`q_F = 12289`) on a polynomial for the Falcon lattice signature scheme. Falcon uses a power-of-2 ring with its own parameter set, requiring a distinct butterfly instruction. Verifies all output coefficients.

### `falcon-intt` — Falcon Inverse Number Theoretic Transform

Runs the INTT over `Z_{q_F}` for Falcon. Validates the inverse polynomial against a known reference.

### `mq-montymul` — Falcon Montgomery Multiplication

Tests `mq_montymul(a, b)` over `Z_{q_F}`, Falcon's Montgomery multiplication returning `a·b·2^{-16} mod q_F`. Validates multiple scalar pairs in SW and HW.

### `falcon-fpr` — Falcon Floating-Point Representation

Tests the `FPR` constructor and `FPR_NORM64` macro, which pack an (sign, exponent, mantissa) triple into Falcon's custom 64-bit floating-point format used in the Gaussian sampler. This is a software-only golden-output test that validates the bit-accurate floating-point packing before any hardware mapping is applied.

### `gf-carryless` — HQC GF(2) 8-bit Carry-less Multiplication

Tests 8-bit carry-less (polynomial) multiplication over GF(2)\[x\] for the HQC code-based encryption scheme. The HW path uses the BFU `OP_GF_CLMUL8` instruction. GF(2) polynomial multiplication is the fundamental building block of HQC's ring arithmetic and binary linear codes.

### `karats` — HQC Karatsuba 64×64 Carry-less Multiplication

Tests `gf2_mul64`, which multiplies two 64-bit GF(2) polynomials using Karatsuba decomposition, producing a 128-bit result. The HW path uses three BFU half-word carry-less multiply instructions followed by a combine instruction. This test covers the most compute-intensive primitive in HQC's ring multiplication.

### `hqc-barrett` — HQC Barrett Reduction (Three Security Profiles)

Tests Barrett reduction for all three HQC parameter sets (`HQC-128`, `HQC-192`, `HQC-256`) using the corresponding moduli `n₁=17669`, `n₃=35851`, `n₅=57637` and pre-computed Barrett constants `μ₁=243079`, `μ₃=119800`, `μ₅=74517`. The HW path uses dedicated single-cycle BFU instructions `HQC1_BARRETT` / `HQC3_BARRETT` / `HQC5_BARRETT` (opcodes `0x3b,0x7,0x58/59/5a`). Twenty test values per profile cover the range zero to near-`2·n`, edge cases at `n-1`, `n`, and `n+1`, and large near-maximum inputs. Comparing SW (software `%` reduction) versus HW establishes the speedup of hardware-assisted coefficient normalisation in HQC polynomial arithmetic.

### `compare-u32` — HQC Constant-Time 32-bit Equality Test

Tests the `OP_COMPARE_U32` BFU instruction, which returns `1` if `rs1 == rs2` and `0` otherwise, in constant time (no data-dependent branching). The software golden model uses the portable identity `((x ^ y) - 1) >> 31 & 1` to achieve branch-free equality. Twenty hand-crafted test vectors cover equal pairs, off-by-one pairs, zero, `UINT32_MAX`, and arbitrary large values. This instruction is used in HQC's ciphertext comparison during decapsulation.

### `gf-reduce` — HQC Polynomial Ring Reduction

Tests the `OP_GF_REDUCE` BFU instruction, which computes `gf_reduce(rs1)` in a single cycle — reducing a 64-bit GF(2) polynomial modulo HQC's ring polynomial `x^n + 1`. The software reference performs the same reduction in C. Test vectors span zero, degree-0 constants, the modulus boundary, and large values requiring multi-step reduction. This operation underlies every multiplication in HQC's public-key encryption ring arithmetic.

### `kyber-montg` — Kyber Montgomery Reduction

Tests the `OP_MQMULK` (`MONTG_KYBER`) instruction which performs Montgomery reduction over `Z_q` for ML-KEM (`q = 3329`, `q^{-1} = 62209` mod 2ˡ⁶). The software golden uses `montgomery_reduce_kem`. Twenty test values include zero, one, negative values, `INT32_MAX`, `INT32_MIN`, and multiples of `q`. This micro-test isolates the scalar Montgomery primitive shared by all Kyber NTT butterfly computations.

### `dilithium-montg` — Dilithium Montgomery Reduction

Tests the `OP_MQMULD` (`MONTG_DIL`) instruction which performs the 64-bit Montgomery reduction for ML-DSA (`q = 8380417`, `q^{-1} = 58728449` mod 2³²). The software golden uses `montgomery_reduce_dsa` with a 64-bit intermediate. Twenty test values cover the same boundary cases as `kyber-montg` but adapted for the wider 32-bit output range. This operation is the inner kernel of every Dilithium NTT butterfly step.

### `falcon-montg` — Falcon Montgomery Reduction

Tests the `MONTG_FALCON` instruction which performs Montgomery reduction over `Z_q` for Falcon (`q = 12289`, `q^{-1} = 12287` mod 2ˡ⁶). The software golden uses `montgomery_reduce_falcon`. Twenty test values include zero, one, negative, `q` itself, and near-maximum values. This micro-test isolates scalar Falcon Montgomery arithmetic, the inner step of Falcon NTT butterflies.

### `falcon-fpr-norm` — Falcon FPR_NORM64 Normalisation

Tests the `FPR_NORM64` macro, which normalises a raw 64-bit integer mantissa into Falcon's custom floating-point format by right-shifting until the leading bit is in bit 54, adjusting the exponent accordingly, and applying round-to-nearest-ties-to-even. Fixed test vectors with pre-computed golden outputs validate bit-accurate behaviour of the normalisation path before any hardware acceleration is applied. This complements `falcon-fpr` (which tests the `FPR` constructor) by covering the normalisation edge cases independently.

### `kyber-poly-ntt` — Kyber Polynomial-Vector Forward NTT

Tests the full polyvec NTT pipeline for ML-KEM with `K=2` polynomial vectors. A deterministic LCG initialises two 256-coefficient polynomials, whose coefficients are reduced to `[0, q_K)`. The software path (`kyber_polyvec_ntt_sw`) applies the scalar NTT to each polynomial; the hardware path (`kyber_polyvec_ntt_hw`) uses the 2×-unrolled `BFNTTK` butterfly loop. Each vector entry is compared coefficient-by-coefficient against the software golden. Cycle counts and speedup factor are reported for the full `K`-polynomial sweep.

### `kyber-poly-intt` — Kyber Polynomial-Vector Inverse NTT

Tests the full polyvec INTT pipeline for ML-KEM with `K=2`. Input vectors are placed into NTT domain via the SW NTT, then passed through SW INTT and HW INTT independently, and the results compared. The hardware path uses the staged INTT — short stages (len ≤ 64) with 2×-unrolled `BFINTTK` butterflies plus a merged final stage (`len=128`) that combines the last butterfly with four `MQMULK` Montgomery scaling steps. This test validates round-trip correctness (NTT→INTT = identity) for multi-polynomial workloads.

### `dilithium-poly-ntt` — Dilithium Polynomial-Vector Forward NTT

Tests the full polyvec NTT pipeline for ML-DSA with `K=4` polynomial vectors. A deterministic LCG seeded by `(i*251 + j*65537 + salt*1000003) * 1664525 + 1013904223` initialises four 256-coefficient polynomials reduced to `[0, q_D)`. The software path (`dilithium_polyvec_ntt_sw`) applies the scalar NTT to each polynomial; the hardware path (`dilithium_polyvec_ntt_hw`) uses the 2×-unrolled `BFNTTD` butterfly loop with `RDRDHI` readout. All 1024 output coefficients across the `K=4` vector are compared against the software golden. Cycle counts and speedup are reported.

### `dilithium-poly-intt` — Dilithium Polynomial-Vector Inverse NTT

Tests the full polyvec INTT pipeline for ML-DSA with `K=4`. Input vectors are placed into NTT domain by the SW NTT, then both SW INTT and HW INTT are applied and the results compared. The hardware path uses 2×-unrolled `BFINTTD` butterflies (with `RDRDHI2` readout) plus a separate final scaling loop that applies `MQMULD` to all 256 coefficients per polynomial with the Dilithium Montgomery correction factor `f=41978`. Round-trip correctness is validated across all 1024 coefficients of the `K=4` vector.

---

## Keccak-Accelerated PQC Primitive Test

### `gen-matrix` — Kyber Matrix Generation via SHAKE128

Tests `gen_a` and `gen_at` (transposed), which generate the public matrix **A** of Kyber by applying rejection sampling to SHAKE128 output streams seeded with a public seed and a row/column index. This is one of the most Keccak-intensive operations in Kyber: for each matrix entry a new SHAKE128 stream is opened, and output bytes are consumed until enough valid coefficients (in range `[0, q_K)`) are collected, potentially requiring multiple squeeze calls. The test verifies correctness against golden reference values for the first four coefficients of each matrix entry, and compares SW (pure software Keccak) vs HW (Keccak coprocessor) cycle counts to quantify the gain from hardware-accelerating the XOF core in a realistic multi-stream scenario.

### `keccak-abs-sha3-256-single-block` — SHA3-256 Single-Block Absorption

Hashes a short message (well below the 136-byte SHA3-256 rate) using SHA3-256 and verifies the 32-byte digest against a known test vector. Only one Keccak-f\[1600\] permutation is required. This split test isolates pure single-block absorption cost, providing a clean baseline for comparing with multi-block and multi-squeeze scenarios, and for measuring SW vs HW permutation latency independently.

### `keccak-abs-sha3-256-multi-block` — SHA3-256 Multi-Block Absorption

Hashes a 200-byte input with SHA3-256. Because 200 bytes exceeds the SHA3-256 rate (136 bytes), two Keccak-f permutations are needed during absorption. A 32-byte golden digest is pre-computed and checked. This test quantifies the linear cost growth of multi-block absorption — the dominant overhead in scenarios with long keys or certificates.

### `keccak-abs-shake256` — SHAKE256 Absorption and Squeeze

Runs SHAKE256 on a fixed-length input and checks the squeezed output against a known reference. SHAKE256 shares the 136-byte rate of SHA3-256 but uses a distinct domain separator and is an extendable-output function. Comparing this test against the SHAKE128 multi-squeeze variant exposes the rate-versus-security tradeoff within the XOF family, relevant for SPHINCS+ PRF and PRFmsg calls.

### `keccak-abs-multi-squeeze` — SHAKE128 Multi-Squeeze (Long XOF Output)

Runs SHAKE128 on a 50-byte input requesting a 500-byte XOF output. Generating 500 bytes from a 168-byte rate requires multiple squeeze permutations (⌈500/168⌉ = 3 permutations beyond the initial absorption). Golden output bytes are verified and SW vs HW cycle counts are compared. This test exposes the squeeze-side bottleneck visible in Kyber's `gen_matrix` and SPHINCS+ pseudorandom key generation whenever the XOF demand exceeds one output block.

---

## Kyber Sampling and Compression Micro-tests

### `cbd_eta/cbd_eta1` - Kyber CBD with eta = 1

This test validates the hardware implementation of Kyber's centered binomial sampler for `eta = 1`. For each 32-bit input word, it extracts 16 lanes, where each lane uses two bits `(a, b)` and outputs `a - b` in `{-1, 0, 1}`. The main loop processes 100 fixed input words and compares all 16 lane outputs against golden vectors.


The performance bottleneck characterized here is bit extraction and per-lane signed accumulation at very fine granularity. In software this stage is branchless but shift-and-mask heavy; the hardware instruction offloads the lane computation and is expected to reduce instruction count per generated coefficient.

### `cbd_eta/cbd_eta2` - Kyber CBD with eta = 2

This test validates the hardware implementation of CBD for `eta = 2`. Each lane consumes 4 bits (two 2-bit partial sums) and returns `a - b` in `{-2, -1, 0, 1, 2}`. For each 32-bit word, 8 coefficients are generated, and all outputs are checked against 100 golden test vectors.

Compared with `eta = 1`, this test stresses wider per-lane arithmetic and fewer lanes per input word. It highlights the same front-end bottleneck (bit slicing and local population counts), but with a higher per-lane arithmetic cost.

### `cbd_eta3` - CBD with eta = 3

Tests the centered binomial sampler for `η=3`. Each 32-bit word contains five lanes; each lane uses two 3-bit fields `(a, b)` (bits `[6j+2:6j]` and `[6j+5:6j+3]`) and outputs `a - b` in `{-3, …, +3}`. One hundred fixed input words are processed, and all 500 lane outputs are compared against a golden reference. This variant is used in ML-DSA (Dilithium) secret polynomial generation where `η=3` is required to achieve the target security level.

### `cbd_eta4` - CBD with eta = 4

Tests the centered binomial sampler for `η=4`. Each 32-bit word contains four lanes; each lane uses two 4-bit nibbles `(a, b)` (bits `[8j+3:8j]` and `[8j+7:8j+4]`) and outputs `a - b` in `{-4, …, +4}`. One hundred fixed input words are processed across the full set of 400 lane outputs, verified against golden values. This parameterisation appears in ML-DSA variants and legacy Dilithium parameter sets that require wider coefficient distributions for the secret key.

### `optimized/compress1` - Kyber coefficient compression (4-bit)

This test checks the custom `COMPRESS1` instruction on 100 representative coefficients modulo `q = 3329`, comparing each output against a precomputed golden table. The observed output range (`0..15`) corresponds to 4-bit quantization.

The bottleneck exposed is scalar quantization throughput: this operation appears in ciphertext/key packing paths where many coefficients must be compressed quickly. The test reports total cycle count for a batched loop and validates functional exactness per element.

### `optimized/compress2` - Kyber coefficient compression (5-bit)

This test is analogous to `compress1` but uses `COMPRESS2` and validates a 5-bit output range (`0..31`) over 100 fixed inputs.

Relative to 4-bit compression, it captures the cost of a different quantization granularity used by other Kyber packing stages. The primary bottleneck remains coefficient-wise scaling/rounding and packing-oriented integer arithmetic.

### `optimized/compress3` - Kyber coefficient compression (10-bit)

This test validates `COMPRESS3` over 100 coefficients, with golden outputs spanning the 10-bit range (`0..1023`).

It models higher-precision compression used in paths where more signal fidelity is retained per coefficient. The bottleneck shown is the same per-coefficient quantization pipeline, now at a wider target bit width.

### `optimized/compress4` - Kyber coefficient compression (11-bit)

This test validates `COMPRESS4` over 100 coefficients, with output values in the 11-bit range (`0..2047`).

It represents the widest compression mode among the four tests and is useful to discuss how quantization width affects cycle cost and datapath pressure. Together, `compress1..4` provide a compact characterization of the tradeoff between packing density and arithmetic overhead in Kyber serialization routines.

---

*All tests share a common measurement methodology: the `mcycle` CSR is zeroed before each timed block and read back immediately after, providing a cycle-accurate performance figure. Where both SW and HW paths are enabled (`SW_TEST_ENABLED = 1`), the speedup is printed as `cycles_sw / cycles_hw` with two decimal places.*

---

## ML-DSA Sampling Tests (Unified Sampler Unit)

These tests validate the new ML-DSA (Dilithium) sampling instructions added to the unified `sampler` hardware unit, which consolidates all sampling operations for both ML-KEM and ML-DSA.

### `mldsa-rej-uniform` — Rejection Sampling for Matrix A

Tests the `OP_REJ_UNIFORM` instruction which implements rejection sampling for generating uniformly random coefficients in `[0, Q-1]` where `Q = 8380417` (ML-DSA modulus). This is used in the `poly_uniform` / `ExpandA` function to generate the public matrix **A**.

**Input:** 24-bit value (3 bytes from SHAKE128 stream)  
**Output:** `{valid[31], 8'b0, coeff[22:0]}` — the MSB indicates acceptance (1 if value < Q)

The test covers:
- Values clearly below Q (accepted)
- Values at the boundary Q-1 (accepted) and Q (rejected)
- Values above Q (rejected)
- Edge cases around the 23-bit mask boundary

This instruction eliminates the comparison and branch overhead in the inner loop of matrix generation, which is one of the most time-consuming operations in ML-DSA key generation.

### `mldsa-rej-eta` — Rejection Sampling for Secret Vectors

Tests `OP_REJ_ETA2` and `OP_REJ_ETA4` instructions which implement nibble-based rejection sampling for generating secret polynomial coefficients with bounded Hamming weight.

**OP_REJ_ETA2 (η=2):**
- Input: byte containing two 4-bit nibbles, nibble selector
- Output: `{valid[31], sign_extended_coeff[30:0]}` in range `[-2, +2]`
- Rejection threshold: nibble >= 15
- Used for: ML-DSA-44 and ML-DSA-65 secret vectors s1, s2

**OP_REJ_ETA4 (η=4):**
- Input: byte containing two 4-bit nibbles, nibble selector  
- Output: `{valid[31], sign_extended_coeff[30:0]}` in range `[-4, +4]`
- Rejection threshold: nibble >= 9
- Used for: Legacy Dilithium parameter sets

The test validates:
- All 16 possible nibble values (0x0-0xF)
- Both low and high nibble extraction
- Correct coefficient transformation and sign extension
- Proper rejection/acceptance behavior

### `mldsa-unpack-z` — Gamma1-Range Coefficient Unpacking

Tests the `OP_UNPACK_Z` instruction which unpacks 18-bit packed coefficients for the ExpandMask function (masking vector **y** generation).

**Input:** 32-bit packed data word, extraction selector (0-3)  
**Output:** Signed 32-bit coefficient in range `[-(γ1-1), γ1]` = `[-131071, 131072]`

For ML-DSA-44, `γ1 = 2^17 = 131072`. The instruction:
1. Extracts an 18-bit value from the packed input based on the selector
2. Transforms it via `coeff = γ1 - raw_value`

The test covers:
- Zero and maximum packed values
- Boundary values producing coefficients at ±γ1
- All four extraction positions (selectors 0-3)
- Verification of correct signed transformation

This instruction accelerates the mask vector sampling which is called repeatedly during each signing attempt in ML-DSA.

