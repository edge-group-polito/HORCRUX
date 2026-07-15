# Software Applications (`sw/applications/`)

This directory contains every piece of software used to validate and benchmark the HORCRUX coprocessor (see [`hw/ip/coprocessors/README.md`](../../hw/ip/coprocessors/README.md) for the hardware side). It is organized into three categories:

| Directory | Purpose |
|---|---|
| [`tests/`](tests/) | One correctness test per HORCRUX instruction / hardware unit, each comparing a software reference implementation against the hardware-accelerated path and reporting the cycle-count speedup. |
| [`tests-power/`](tests-power/) | The subset of `tests/` used for post-synthesis power characterization (SW-baseline vs. HW-accelerated). See [`tests-power/README.md`](tests-power/README.md). |
| [`pqc/`](pqc/) | Full end-to-end PQC algorithms (KEM/DS), in `baseline` (no HW acceleration) and `optimized` (HORCRUX instructions enabled) variants. |

All applications are built and run the same way, through the top-level `makefile`:
```bash
make app PROJECT=<category>/<path-to-app>
make questasim-sim        # RTL simulation
```
See [`scripts/README.md`](../../scripts/README.md) for batch/automation helpers (`sim_all_app.sh`, `compile_apps.sh`, etc.) that wrap these commands across many applications at once.

---

## 🧪 `tests/` — Instruction-Level Tests

Each test builds a software golden reference and a hardware-accelerated path, verifies both against known vectors, and reports SW/HW cycle counts and speedup. Full per-test descriptions of methodology and bottleneck analysis are in [`tests/TEST_DESCRIPTIONS.md`](tests/TEST_DESCRIPTIONS.md); build/run details and the `run_tests.sh` helper are in [`tests/README.md`](tests/README.md).

### BFU (Butterfly Unit) — NTT / INTT
| Test | Algorithm | Operation |
|---|---|---|
| `dilithium-ntt` | ML-DSA | Forward NTT (256 coefficients) |
| `dilithium-intt` | ML-DSA | Inverse NTT (256 coefficients) |
| `dilithium-poly-ntt` | ML-DSA | Forward NTT on a K=4 polynomial vector |
| `dilithium-poly-intt` | ML-DSA | Inverse NTT on a K=4 polynomial vector |
| `kyber-ntt` | ML-KEM | Forward NTT (256 coefficients) |
| `kyber-intt` | ML-KEM | Inverse NTT (256 coefficients) |
| `kyber-poly-ntt` | ML-KEM | Forward NTT on a K=2 polynomial vector |
| `kyber-poly-intt` | ML-KEM | Inverse NTT on a K=2 polynomial vector |
| `falcon-ntt` | Falcon | Forward NTT (256 coefficients) |
| `falcon-intt` | Falcon | Inverse NTT (256 coefficients) |

### BFU — Scalar Arithmetic
| Test | Algorithm | Operation |
|---|---|---|
| `mq-montymul` | Falcon | Montgomery multiplication (`MQMULF`) |
| `falcon-fpr` | Falcon | FPR/FPR_NORM64 fixed-vector golden test |
| `falcon-fpr-norm` | Falcon | FPR_NORM64 normalisation golden test |
| `falcon-montg` | Falcon | Montgomery reduction (`MQMULF`) |
| `kyber-barrett` | ML-KEM | Barrett reduction (`BARRETT`) |
| `kyber-montg` | ML-KEM | Montgomery reduction (`MQMULK`) |
| `dilithium-montg` | ML-DSA | Montgomery reduction (`MQMULD`) |
| `dilithium-reduce32` | ML-DSA | Modular reduction (`RED32` / caddq) |
| `fqmul` | ML-KEM | `fqmul` Montgomery multiplication |
| `karats` | HQC | Karatsuba 64×64 GF(2) (`KARATS1-4`) |
| `gf-carryless` | HQC | 8-bit carry-less multiplication (`GFMUL8`) |
| `hqc-barrett` | HQC | Barrett reduction (`BARRETT_HQC{1,3,5}`) |
| `compare-u32` | HQC | Constant-time 32-bit equality (`COMPARE_U32`) |
| `gf-reduce` | HQC | Polynomial ring reduction (`GF_REDUCE`) |
| `cbd_eta1` / `cbd_eta2` / `cbd_eta3` / `cbd_eta4` | ML-KEM/ML-DSA | CBD sampling, `η ∈ {1,2,3,4}` (`CBD1-4`) |

### Keccak Accelerator
| Test | Algorithm | Operation |
|---|---|---|
| `keccak-abs` | SHA3/SHAKE | Keccak absorption — all modes |
| `keccak-abs-sha3-256-single-block` | SHA3-256 | Single-block absorption + squeeze |
| `keccak-abs-sha3-256-multi-block` | SHA3-256 | Multi-block absorption (>136 B input) |
| `keccak-abs-multi-squeeze` | SHAKE128 | Multi-squeeze (500 B XOF from 50 B input) |
| `keccak-abs-shake256` | SHAKE256 | Single-block absorption + squeeze |
| `gen-matrix` | ML-KEM | Matrix **A** generation via SHAKE128 XOF |

### SPHINCS+ Accelerator (SLH-DSA)
| Test | Algorithm | Operation |
|---|---|---|
| `thash` | SLH-DSA | Tweakable hash, 1-block input (`THASH1`) |
| `thash2` | SLH-DSA | Tweakable hash, 2-block input (`THASH2`) |
| `thash-wots` | SLH-DSA | THASH + full WOTS+ chain (`gen_chain`) |
| `prf-addr` | SLH-DSA | PRF address derivation (`PRFADDR`) |
| `chain-lengths` | SLH-DSA | WOTS+ base-w chain-length / checksum computation |

### ML-DSA Sampling (Unified Sampler)
| Test | Algorithm | Operation |
|---|---|---|
| `mldsa-rej-uniform` | ML-DSA | Rejection sampling for Matrix A (`REJ_UNIFORM`) |
| `mldsa-rej-eta` | ML-DSA | Nibble-based rejection sampling, `η ∈ {2,4}` (`REJ_ETA2`/`REJ_ETA4`) |
| `mldsa-unpack-z` | ML-DSA | `γ1`-range coefficient unpacking (`UNPACK_Z`) |

### How to run
```bash
# Via the test runner (from sw/applications/tests/)
./run_tests.sh -t dilithium-ntt          # single test, SW+HW
./run_tests.sh -t dilithium-ntt,kyber-ntt --hw-only
./run_tests.sh --all
./run_tests.sh --list

# Or directly with make
make app PROJECT=tests/dilithium-ntt SW_TEST_ENABLED=1
make questasim-sim
```

---

## ⚡ `tests-power/` — Power-Characterization Tests

A curated subset of the tests above (30 apps), used to run post-synthesis SW-vs-HW power comparisons. Building/running an individual app is the same as for `tests/`:
```bash
make app PROJECT=tests-power/karats
make questasim-run-postsynth VCD_MODE=2
make power-analysis
```
The full power-extraction flow across all (or a filtered set of) tests is automated by [`scripts/get_power_res_postsynth.sh`](../../scripts/get_power_res_postsynth.sh). See [`tests-power/README.md`](tests-power/README.md) for what each test measures and [`scripts/README.md`](../../scripts/README.md) for the automation details.

Available tests: `cbd_eta2`, `cbd_eta3`, `chain-lengths`, `dilithium-intt`, `dilithium-ntt`, `dilithium-poly-intt`, `dilithium-poly-ntt`, `falcon-fpr`, `falcon-fpr-norm`, `falcon-intt`, `falcon-ntt`, `fqmul`, `gen-matrix`, `gen_chain`, `gf-carryless`, `gf-reduce`, `karats`, `keccak-abs-multi-squeeze`, `keccak-abs-sha3-256-multi-block`, `keccak-abs-sha3-256-single-block`, `keccak-abs-shake256`, `kyber-intt`, `kyber-ntt`, `kyber-poly-intt`, `kyber-poly-ntt`, `mldsa-rej-eta`, `mq-montymul`, `prf-addr`, `thash`, `thash2`, `thash-wots`.

---

## 🔐 `pqc/` — Full PQC Algorithms

Each application runs a **self-contained KAT test** (adapted from the algorithm's official NIST reference implementation) covering key generation, encapsulation/signing, and decapsulation/verification. Which phases run is controlled inside each app's `main.c` via `TEST_KEY` / `TEST_ENC` (or `TEST_SIGN`) / `TEST_DEC` (or `TEST_SIGN_OPEN`) macros.

| Variant | Description |
|---|---|
| `baseline/` | Pure software implementation, no HORCRUX instructions. |
| `optimized/` | Same algorithms re-implemented to use the HORCRUX custom ISA. |

| Scheme | Algorithm | Versions | `baseline` path | `optimized` path |
|---|---|---|---|---|
| KEM | ML-KEM | `ml-kem-512`, `ml-kem-768`, `ml-kem-1024` | `pqc/baseline/KEM/ML-KEM/<version>` | `pqc/optimized/KEM/ML-KEM/<version>` |
| KEM | HQC | `HQC-1`, `HQC-3`, `HQC-5` | `pqc/baseline/KEM/HQC-2025/<version>` | `pqc/optimized/KEM/HQC/HQC-2025/<version>` |
| DS | ML-DSA | `ML-DSA-44`, `ML-DSA-65`, `ML-DSA-87` | `pqc/baseline/DS/ML-DSA/<version>` | `pqc/optimized/DS/ML-DSA/<version>` |
| DS | Falcon | `falcon-512`, `falcon-1024` | `pqc/baseline/DS/FALCON/<version>` | `pqc/optimized/DS/FALCON/<version>` |
| DS | SLH-DSA | `SPHINCS-128f-simple`, `SPHINCS-128f-robust`, `SPHINCS-192f-simple`, `SPHINCS-192f-robust`, `SPHINCS-256f-simple`, `SPHINCS-256f-robust` | `pqc/baseline/DS/SLH-DSA/<version>` | `pqc/optimized/DS/SLH-DSA/<version>` |

### How to run
```bash
# Baseline ML-KEM-512
make app PROJECT=pqc/baseline/KEM/ML-KEM/ml-kem-512
make questasim-sim

# HORCRUX-optimized ML-DSA-65
make app PROJECT=pqc/optimized/DS/ML-DSA/ML-DSA-65
make questasim-sim
```

For FPGA builds and batch compilation across many algorithms, see [`scripts/compile_apps.sh`](../../scripts/compile_apps.sh), [`scripts/compile_apps_fpga.sh`](../../scripts/compile_apps_fpga.sh) and [`scripts/sim_all_app.sh`](../../scripts/sim_all_app.sh), documented in [`scripts/README.md`](../../scripts/README.md).
