# HORCRUX Coprocessor (`hw/ip/coprocessors/`)

This directory contains the RTL of the **HORCRUX coprocessor**, a tightly-coupled accelerator exposed to the RISC-V core through the [CV-X-IF](https://docs.openhwgroup.org/projects/openhw-group-core-v-xif/en/latest/#) interface. It executes a unified custom Instruction-Set Extension (ISE) that covers **ML-KEM**, **ML-DSA**, **SLH-DSA**, **HQC**, and **Falcon** by sharing hardware resources (a single multiplier tree, one Keccak core, one sampler, one FPR unit) across all algorithm families instead of instantiating per-algorithm accelerators.

See the top-level [README](../../../README.md) for the overall system figure and for pointers to the software and script documentation.

## 📁 Module Overview

### Pipeline glue
| Module | Description |
|---|---|
| [`horcrux.sv`](horcrux.sv) | Top-level controller. Instantiates the decode, functional-unit, and commit stages and orchestrates instruction dispatch. |
| [`id_stage.sv`](id_stage.sv) | Instruction decode stage. Parses the custom opcodes and routes operands (`rs1`, `rs2`, `rs3`) to the appropriate functional unit. |
| [`commit_stage.sv`](commit_stage.sv) | Commit stage. Writes results back to the RISC-V core register file in program order. |
| [`horcrux_top.sv`](horcrux_top.sv) | Interface wrapper that connects `horcrux.sv` to the X-HEEP SoC via CV-X-IF. |
| [`coproc_wrapper.sv`](coproc_wrapper.sv) | Coprocessor wrapper instantiated inside the CPU subsystem. |

### Shared state
| Module | Description |
|---|---|
| [`horcrux_register.sv`](horcrux_register.sv) | Internal 50-word × 32-bit register file mirroring the 1600-bit Keccak sponge state. Persists state across `absorb`/`permute`/`squeeze` phases and supports the `LOAD`, `STORE`, `KINIT`, bulk writeback, and `KABSORB` (indexed XOR injection) access modes. |

### Keccak / SHA-3 engine (`keccak/`)
| Module | Description |
|---|---|
| [`keccak/keccak_f.sv`](keccak/keccak_f.sv) | Top level of the Keccak-f[1600] permutation core. |
| [`keccak/keccak_round.sv`](keccak/keccak_round.sv) | Single round function (θ, ρ, π, χ, ι), implemented as a fully combinational logic cloud. |
| [`keccak/keccak_cu.sv`](keccak/keccak_cu.sv) | Control unit: a 3-state FSM that iterates the combinational round 24 times per permutation. |
| [`keccak/keccak_dp.sv`](keccak/keccak_dp.sv) | Datapath holding the registered 1600-bit state between rounds. |
| [`keccak/keccak_round_constants_gen.sv`](keccak/keccak_round_constants_gen.sv) | Generates the round constants for the ι step. |
| [`keccak/pkg_keccak.sv`](keccak/pkg_keccak.sv) | Keccak-specific SystemVerilog package (types/parameters). |

### SPHINCS+ / hash-based signatures
| Module | Description |
|---|---|
| [`sphincs_ops.sv`](sphincs_ops.sv) | Orchestrates SLH-DSA operations on top of the shared Keccak engine: `WOTS`, `PRFADDR`, and the robust `THASH1`/`THASH2` tweakable hash, with dedicated register layouts per security level (128/192/256-bit). |
| [`chain_lengths.sv`](chain_lengths.sv) | Combinationally converts a message hash into WOTS+ base-w digits and checksum for all three SLH-DSA security levels. |

### Unified multiplier & lattice/code-based arithmetic
| Module | Description |
|---|---|
| [`multiplier_tree.sv`](multiplier_tree.sv) | Unified datapath for NTT butterflies (ML-KEM/ML-DSA/Falcon) and Montgomery multiplication, built around the Shared Multiplication Logic. |
| [`shared_multiplication_logic.sv`](shared_multiplication_logic.sv) | Core reduction skeleton: computes the universal Montgomery reduction for lattice schemes and routes to `unified_mul_32x32` for HQC's carry-less products. |
| [`unified_mul_32x32.sv`](unified_mul_32x32.sv) | Dual-mode 32×32 multiplier: standard carry-propagating integer multiplication or carry-free `GF(2)` accumulation, selected by a single `carryless_mode_i` control signal. |
| [`barrett.sv`](barrett.sv) | Barrett reduction unit shared between ML-KEM (mod 3329) and HQC (mod 17669/35851/57637), implemented with hardwired shifts and additions instead of a divider. |

### Sampling
| Module | Description |
|---|---|
| [`sampler.sv`](sampler.sv) | Unified sampler for ML-KEM/ML-DSA: Centered Binomial Distribution (CBD) sampling, rejection sampling for matrix **A** and secret vectors, and `UNPACK_Z` for `γ1`-range mask coefficients. |
| [`cbd_eta.sv`](cbd_eta.sv) | CBD sub-unit used by `sampler.sv`, parameterized over `η ∈ {1,2,3,4}`. |

### Falcon floating point
| Module | Description |
|---|---|
| [`fpr.sv`](fpr.sv) | Falcon FPR unit. Packs/normalizes the custom 64-bit floating-point representation used by Falcon's Gaussian sampler, multiplexing the 64-bit result over the 32-bit RISC-V register interface across multiple instructions. |

### Packages (`include/`)
| Module | Description |
|---|---|
| [`include/horcrux_pkg.sv`](include/horcrux_pkg.sv) | HORCRUX ISA package: instruction encodings and shared types used by every module above. |
| [`include/cv32e40px_pkg.sv`](include/cv32e40px_pkg.sv), [`include/cv32e40px_core_v_xif_pkg.sv`](include/cv32e40px_core_v_xif_pkg.sv) | CV32E40P core and CV-X-IF interface packages required to integrate the coprocessor into the pipeline. |

## 🧬 The HORCRUX Instruction Set Architecture

All instructions use standard RISC-V `R-type` (2 source registers) or `R4-type` (3 source registers) encoding — no changes to the core decoder or pipeline are required. The naming convention reflects the high degree of hardware reuse across algorithms.

| Instruction | Type | Algorithm(s) | Description |
|---|:-:|---|---|
| **Data Movement & State Management** | | | |
| `LOAD` | R4 | All | Loads two 32-bit words from CPU to internal RF. |
| `STORE` | R | All | Reads a 32-bit word from internal RF to CPU. |
| `COMPARE_U32` | R | All | Constant-time comparison. |
| **Keccak / SHA-3 Acceleration** | | | |
| `KINIT` | R | All | Clears the 1600-bit internal register file to zero. |
| `KSTART` | R4 | All | Triggers 24-round Keccak-f[1600] on internal state. |
| `KPERM` | R4 | All | Performs in-place permutation with direct writeback. |
| `KABSORB` | R4 | All | XORs data (`rs1`, `rs2`) into internal state (absorption). |
| `KREAD3` | R | All | Reads 3 bytes from state for rejection sampling. |
| **Hash-Based Signatures** | | | |
| `WOTS_{128,192,256}` | R | SLH-DSA | Computes WOTS+ chains for 128, 192, or 256-bit security. |
| `PRFADDR_{128,192,256}` | R | SLH-DSA | PRF: computes `SHAKE256(sk_seed ‖ addr)`. |
| `THASH1_{128,192,256}` | R | SLH-DSA | Tweakable hash (robust) for 1-block input. |
| `THASH2_{128,192,256}` | R | SLH-DSA | Tweakable hash (robust) for 2-block input. |
| **Unified Multiplier & NTT Operations** | | | |
| `BFNTTK` | R4 | ML-KEM | Cooley-Tukey butterfly in ℤ₃₃₂₉ (`a ± b·ζ`). |
| `BFINTTK` | R4 | ML-KEM | Gentleman-Sande butterfly in ℤ₃₃₂₉. |
| `BFNTTD` | R4 | ML-DSA | Forward NTT butterfly for Dilithium (low bits). |
| `BFINTTD` | R4 | ML-DSA | Inverse NTT butterfly for Dilithium (low bits). |
| `BFNTTDH` | R | ML-DSA | Retrieves high bits of previous Dilithium NTT result. |
| `BFINTTDH` | R | ML-DSA | Retrieves high bits of previous Dilithium INTT result. |
| `BFNTTF` | R4 | Falcon | Forward NTT butterfly in ℤ₁₂₂₈₉. |
| `BFINTTF` | R4 | Falcon | Inverse NTT butterfly in ℤ₁₂₂₈₉. |
| `MQMULK` | R | ML-KEM | Montgomery mul: `a·b·R⁻¹ mod 3329`. |
| `MQMULD` | R | ML-DSA | Montgomery mul: `a·b·R⁻¹ mod 8380417`. |
| `MQMULF` | R | Falcon | Montgomery mul: `a·b·R⁻¹ mod 12289`. |
| `MODP_MONTYMUL` | R4 | Falcon | Montgomery multiplication in mod-p path (dynamic prime, runtime keygen). |
| `BARRETT` | R | ML-KEM | Barrett reduction for ℤ₃₃₂₉. |
| `RED32` | R | ML-DSA | Conditional reduction for 32-bit signed integers. |
| **Binary Field & Reductions** | | | |
| `GFMUL8` | R | HQC | Carry-less 8×8 multiplication over `GF(2)`. |
| `GF_REDUCE` | R | HQC | Full `GF(2^8)` reduction modulo `0x11D`. |
| `KARATS1` | R | HQC | Karatsuba step 1: computes `a_lo × b_lo`. |
| `KARATS2` | R | HQC | Karatsuba step 2: computes `a_hi × b_hi`. |
| `KARATS3` | R | HQC | Karatsuba step 3: computes cross-term. |
| `KARATS4` | R | HQC | Karatsuba step 4: reconstruction read. |
| `BARRETT_HQC{1,3,5}` | R | HQC | Barrett reduction for `N ∈ {17669, 35851, 57637}`. |
| **Sampling & Rejection** | | | |
| `CBD{1,2,3,4}` | R | lattice | CBD sampling for `η ∈ {1, 2, 3, 4}`. |
| `REJ_UNIFORM` | R | ML-DSA | Rejection sampling mod `Q_MLDSA` for Matrix A. |
| `REJ_ETA{2,4}` | R | ML-DSA | Nibble-based rejection sampling for secret vectors. |
| `UNPACK_Z` | R | ML-DSA | `γ1`-range coefficient unpacking for ExpandMask. |
| **Falcon FPR Helpers** | | | |
| `FPR_LOAD_SE` | R | Falcon | Loads sign/exponent state for Falcon FPR. |
| `FPR_EXEC` | R | Falcon | Executes Falcon FPR packing and returns low 32 bits. |
| `FPR_RDHI` | R | Falcon | Reads high 32 bits from previous `FPR_EXEC`. |
| `FPR_NORM64_EXEC` | R4 | Falcon | Normalizes 64-bit mantissa and returns low 32 bits. |
| `FPR_NORM64_RDHI` | R | Falcon | Reads high 32 bits from previous `FPR_NORM64_EXEC`. |
| `FPR_NORM64_RDE` | R | Falcon | Reads adjusted exponent from previous `FPR_NORM64_EXEC`. |

The exact instruction encodings and mnemonics are defined in [`include/horcrux_pkg.sv`](include/horcrux_pkg.sv). Each instruction is individually exercised by a dedicated test in [`sw/applications/tests/`](../../../sw/applications/tests/README.md).
