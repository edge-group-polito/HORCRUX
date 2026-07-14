# Hardware Accelerator Tests

This directory contains comprehensive tests for the hardware accelerators used in the Horcrux PQ-crypto coprocessor:
- **BFU (Butterfly Unit)** - Core shared resource for NTT/INTT operations across Dilithium, Kyber, Falcon, and HQC
- **Keccak Accelerator** - SHA3/SHAKE hardware acceleration for hash-based operations
- **SPHINCS+ Accelerator** - Specialized instructions for SPHINCS+ hash-based signatures

## Directory Structure

```
tests/
├── run_tests.sh          # Test runner script
├── README.md             # This file
│
├── # BFU (Butterfly Unit) Tests — NTT/INTT
├── dilithium-ntt/        # Dilithium NTT (Forward Transform)
├── dilithium-intt/       # Dilithium INTT (Inverse Transform)
├── dilithium-poly-ntt/   # Dilithium polyvec NTT (K=4 polynomial vectors)
├── dilithium-poly-intt/  # Dilithium polyvec INTT (K=4 polynomial vectors)
├── kyber-ntt/            # Kyber NTT (Forward Transform)
├── kyber-intt/           # Kyber INTT (Inverse Transform)
├── kyber-poly-ntt/       # Kyber polyvec NTT (K=2 polynomial vectors)
├── kyber-poly-intt/      # Kyber polyvec INTT (K=2 polynomial vectors)
├── falcon-ntt/           # Falcon NTT (Forward Transform)
├── falcon-intt/          # Falcon INTT (Inverse Transform)
│
├── # BFU (Butterfly Unit) Tests — Scalar Arithmetic
├── mq-montymul/          # Falcon Montgomery Multiplication
├── falcon-fpr/           # Falcon FPR/FPR_NORM64 fixed-vector golden test
├── falcon-fpr-norm/      # Falcon FPR_NORM64 normalisation golden test
├── falcon-montg/         # Falcon Montgomery reduction (OP_MQMULF)
├── kyber-barrett/        # Kyber/Dilithium Barrett reduction
├── kyber-montg/          # Kyber Montgomery reduction (OP_MQMULK)
├── dilithium-montg/      # Dilithium Montgomery reduction (OP_MQMULD)
├── dilithium-reduce32/   # Dilithium reduce32/caddq
├── fqmul/                # Kyber fqmul (Montgomery Reduction)
├── karats/               # HQC Karatsuba 64x64 Multiplication
├── gf-carryless/         # HQC GF(2) 8-bit Carry-less Multiplication
├── hqc-barrett/          # HQC Barrett reduction (all three security profiles)
├── compare-u32/          # HQC constant-time 32-bit equality (OP_COMPARE_U32)
├── gf-reduce/            # HQC polynomial ring reduction (OP_GF_REDUCE)
├── cbd_eta3/             # Kyber/Dilithium CBD sampling η=3
├── cbd_eta4/             # Kyber/Dilithium CBD sampling η=4
│
├── # Keccak Accelerator Tests
├── keccak-abs/                        # Keccak/SHA3/SHAKE absorption (all modes)
├── keccak-abs-sha3-256-single-block/  # SHA3-256 single-block absorption
├── keccak-abs-sha3-256-multi-block/   # SHA3-256 multi-block absorption
├── keccak-abs-multi-squeeze/          # SHAKE128 multi-squeeze (long XOF output)
├── keccak-abs-shake256/               # SHAKE256 absorption + squeeze
├── gen-matrix/                        # Kyber gen_matrix (SHAKE128 XOF)
│
└── # SPHINCS+ Accelerator Tests
├── thash/            # SPHINCS+ THASH (tweakable hash)
├── thash-wots/       # SPHINCS+ THASH + WOTS chains
├── prf-addr/         # SPHINCS+ PRF address derivation
└── chain-lengths/    # SPHINCS+ WOTS+ chain lengths
```

## Test Overview

### BFU (Butterfly Unit) Tests

#### NTT / INTT Tests
| Test | Algorithm | Operation |
|------|-----------|-----------|
| dilithium-ntt | Dilithium | Forward NTT (256 coefficients) |
| dilithium-intt | Dilithium | Inverse NTT (256 coefficients) |
| dilithium-poly-ntt | Dilithium | Forward NTT on a K=4 polynomial vector |
| dilithium-poly-intt | Dilithium | Inverse NTT on a K=4 polynomial vector |
| kyber-ntt | Kyber | Forward NTT (256 coefficients, 2×/4× unroll variants) |
| kyber-intt | Kyber | Inverse NTT (256 coefficients) |
| kyber-poly-ntt | Kyber | Forward NTT on a K=2 polynomial vector |
| kyber-poly-intt | Kyber | Inverse NTT on a K=2 polynomial vector |
| falcon-ntt | Falcon | Forward NTT (256 coefficients) |
| falcon-intt | Falcon | Inverse NTT (256 coefficients) |

#### Scalar Arithmetic Tests
| Test | Algorithm | Operation |
|------|-----------|-----------|
| mq-montymul | Falcon | Montgomery Multiplication (`OP_MQMULF`) |
| falcon-fpr | Falcon | FPR/FPR_NORM64 fixed-vector golden test |
| falcon-fpr-norm | Falcon | FPR_NORM64 normalisation golden test |
| falcon-montg | Falcon | Montgomery reduction (`OP_MQMULF`) |
| kyber-barrett | Kyber | Barrett reduction |
| kyber-montg | Kyber | Montgomery reduction (`OP_MQMULK`) |
| dilithium-montg | Dilithium | Montgomery reduction (`OP_MQMULD`) |
| dilithium-reduce32 | Dilithium | Modular Reduction (reduce32 / caddq) |
| fqmul | Kyber | `fqmul` Montgomery Multiplication |
| karats | HQC | Karatsuba 64×64 GF(2) |
| gf-carryless | HQC | 8-bit Carry-less Multiplication |
| hqc-barrett | HQC | Barrett reduction (profiles 1 / 3 / 5) |
| compare-u32 | HQC | Constant-time 32-bit equality (`OP_COMPARE_U32`) |
| gf-reduce | HQC | Polynomial ring reduction (`OP_GF_REDUCE`) |
| cbd_eta3 | Dilithium | CBD sampling η=3 (5 lanes per word) |
| cbd_eta4 | Kyber/Dilithium | CBD sampling η=4 (4 lanes per word) |

### Keccak Accelerator Tests
| Test | Algorithm | Operation |
|------|-----------|-----------|
| keccak-abs | SHA3/SHAKE | Keccak absorption — all modes (single/multi-block, SHA3/SHAKE) |
| keccak-abs-sha3-256-single-block | SHA3-256 | Single-block absorption + squeeze |
| keccak-abs-sha3-256-multi-block | SHA3-256 | Multi-block absorption (>136 B input) |
| keccak-abs-multi-squeeze | SHAKE128 | Multi-squeeze (500 B XOF output from 50 B input) |
| keccak-abs-shake256 | SHAKE256 | Single-block absorption + squeeze |
| gen-matrix | Kyber | Matrix A generation using SHAKE128 XOF |

### SPHINCS+ Accelerator Tests
| Test | Algorithm | Operation |
|------|-----------|-----------|
| thash | SPHINCS+ | Tweakable hash (THASH1 - 1 block input) |
| thash-wots | SPHINCS+ | THASH + WOTS chain generation |
| prf-addr | SPHINCS+ | PRF address derivation for secret keys |
| chain-lengths | SPHINCS+ | WOTS+ chain lengths computation |

## Usage

### Using the Test Runner Script

```bash
# Run all tests with both SW and HW
./run_tests.sh --all

# Run all tests with HW only (skip software reference)
./run_tests.sh --all --hw-only

# Run specific test
./run_tests.sh -t dilithium-ntt

# Run multiple tests
./run_tests.sh -t dilithium-ntt,kyber-ntt,falcon-ntt

# Build only (no simulation)
./run_tests.sh -t dilithium-ntt -b

# List available tests
./run_tests.sh --list

# Show help
./run_tests.sh --help
```

### Manual Build and Run

Each test can be built individually using the project's makefile:

```bash
# Build with SW tests enabled (default)
make app PROJECT=tests/dilithium-ntt SW_TEST_ENABLED=1

# Build with HW tests only
make app PROJECT=tests/dilithium-ntt SW_TEST_ENABLED=0

# Run simulation
make sim PROJECT=tests/dilithium-ntt
```

## Test Structure

Each test follows a consistent structure:

1. **CSR Initialization** - Enable cycle counting
   ```c
   CSR_CLEAR_BITS(CSR_REG_MCOUNTINHIBIT, 0x1);
   CSR_WRITE(CSR_REG_MCYCLE, 0);
   ```

2. **Software Reference Test** (optional, controlled by `SW_TEST_ENABLED`)
   - Runs the pure software implementation
   - Measures cycle count
   - Verifies against known test vectors

3. **Hardware Accelerated Test**
   - Runs using BFU hardware instructions
   - Measures cycle count
   - Verifies against known test vectors

4. **Comparison Report**
   - Reports SW vs HW cycle counts
   - Calculates speedup factor

## SW_TEST_ENABLED Flag

The `SW_TEST_ENABLED` flag controls whether software reference tests are run:

- `SW_TEST_ENABLED=1` (default): Run both SW and HW tests
- `SW_TEST_ENABLED=0`: Run HW tests only

This can be set:
- At compile time: `make app ... SW_TEST_ENABLED=0`
- Using the test runner: `./run_tests.sh --hw-only`

## Example Output

```
======================================
    DILITHIUM NTT TEST
======================================
--- Dilithium NTT (Software) ---
PASSED: Dilithium NTT (Software)
Software Cycles: 125432

--- Dilithium NTT (Hardware BFU) ---
PASSED: Dilithium NTT (Hardware)
Hardware Cycles: 8521

======================================
SW Cycles: 125432 | HW Cycles: 8521
Speedup: 14.72x
======================================
FINAL STATUS: ALL TESTS PASSED
```

## Hardware Instructions

The tests use custom RISC-V instructions for the BFU accelerator:

| Instruction | Operation | Use Case |
|-------------|-----------|----------|
| BFNTTK | Kyber NTT Butterfly | Kyber NTT |
| BFINTTK | Kyber INTT Butterfly | Kyber INTT |
| BFNTTD | Dilithium NTT Butterfly | Dilithium NTT |
| BFINTTD | Dilithium INTT Butterfly | Dilithium INTT |
| BFNTTF | Falcon NTT Butterfly | Falcon NTT |
| BFINTTF | Falcon INTT Butterfly | Falcon INTT |
| MQMULF | Falcon Montgomery Mul | Falcon |
| MQMULK | Kyber Montgomery Mul | Kyber |
| MQMULD | Dilithium Montgomery Mul | Dilithium |
| KARATS_1/2/3 | Karatsuba Steps | HQC |
| GFMUL8 | 8-bit Carry-less Mul | HQC |
| RED32 | Dilithium reduce32 | Dilithium |
| CADDQ | Dilithium caddq | Dilithium |
| OP_COMPARE_U32 | Constant-time 32-bit equality | HQC |
| OP_GF_REDUCE | HQC polynomial ring reduction | HQC |
| HQC1/3/5_BARRETT | Barrett reduction (3 profiles) | HQC |

## Contributing

When adding new tests:

1. Create a new directory under `tests/`
2. Copy the relevant source files (bfu.h, bfu.c, algorithm headers)
3. Create a `main.c` following the existing pattern
4. Add the test name to `AVAILABLE_TESTS` in `run_tests.sh`
5. Update this README
