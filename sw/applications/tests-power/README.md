# Power-Characterization Tests (`sw/applications/tests-power/`)

This directory mirrors a subset of [`sw/applications/tests/`](../tests/README.md), stripped down for **post-synthesis power measurement** instead of functional/cycle-count verification. See [`sw/applications/README.md`](../README.md) for how this fits alongside the `tests/` and `pqc/` categories.

## What's different from `tests/`

Each app here removes the correctness-checking and cycle-counting code and instead brackets the operation under test with the [`vcd_util`](../../external/lib/runtime/vcd_util.h) helper:

```c
if (vcd_init() != 0) return 1;
vcd_enable();
karats_hw_compute(results_hw);   // the operation being power-characterized
vcd_disable();
```

`vcd_enable()`/`vcd_disable()` toggle a dedicated GPIO (`VCD_GPIO`) around the region of interest. In simulation, `VCD_MODE=2` makes QuestaSim start/stop VCD dumping on that GPIO edge, so the resulting waveform contains **only** the target operation — not testbench setup, UART logging, or verification code — giving a clean window for power analysis.

Like the rest of the codebase, the `SW_TEST_ENABLED` macro selects which implementation is compiled into the binary:
- `SW_TEST_ENABLED=1` → software-only reference implementation is exercised (`waves-0.vcd`)
- `SW_TEST_ENABLED=0` → HORCRUX hardware-accelerated path is exercised (`waves-1.vcd`)

Each test is therefore built and simulated **twice** (once per configuration) to obtain comparable SW-baseline and HW-accelerated power reports for the same operation.

## Available Tests

Each test characterizes the same operation as its `tests/` counterpart — see [`tests/TEST_DESCRIPTIONS.md`](../tests/TEST_DESCRIPTIONS.md) for the full description of what each one exercises and why.

| Test | Algorithm | Operation |
|---|---|---|
| `dilithium-ntt` | ML-DSA | Forward NTT |
| `dilithium-intt` | ML-DSA | Inverse NTT |
| `dilithium-poly-ntt` | ML-DSA | Forward NTT, K=4 polynomial vector |
| `dilithium-poly-intt` | ML-DSA | Inverse NTT, K=4 polynomial vector |
| `kyber-ntt` | ML-KEM | Forward NTT |
| `kyber-intt` | ML-KEM | Inverse NTT |
| `kyber-poly-ntt` | ML-KEM | Forward NTT, K=2 polynomial vector |
| `kyber-poly-intt` | ML-KEM | Inverse NTT, K=2 polynomial vector |
| `falcon-ntt` | Falcon | Forward NTT |
| `falcon-intt` | Falcon | Inverse NTT |
| `mq-montymul` | Falcon | Montgomery multiplication (`MQMULF`) |
| `falcon-fpr` | Falcon | FPR fixed-vector golden test |
| `falcon-fpr-norm` | Falcon | FPR_NORM64 normalisation |
| `fqmul` | ML-KEM | `fqmul` Montgomery multiplication |
| `karats` | HQC | Karatsuba 64×64 GF(2) multiplication |
| `gf-carryless` | HQC | 8-bit carry-less multiplication |
| `gf-reduce` | HQC | Polynomial ring reduction |
| `cbd_eta2` / `cbd_eta3` | ML-KEM/ML-DSA | CBD sampling, `η=2` / `η=3` |
| `keccak-abs-sha3-256-single-block` | SHA3-256 | Single-block absorption + squeeze |
| `keccak-abs-sha3-256-multi-block` | SHA3-256 | Multi-block absorption |
| `keccak-abs-multi-squeeze` | SHAKE128 | Multi-squeeze XOF output |
| `keccak-abs-shake256` | SHAKE256 | Absorption + squeeze |
| `gen-matrix` | ML-KEM | Matrix **A** generation (SHAKE128 XOF) |
| `thash` | SLH-DSA | Tweakable hash, 1-block input |
| `thash2` | SLH-DSA | Tweakable hash, 2-block input |
| `thash-wots` | SLH-DSA | THASH + WOTS+ chain generation |
| `gen_chain` | SLH-DSA | WOTS+ chain generation (`gen_chain`) |
| `prf-addr` | SLH-DSA | PRF address derivation |
| `chain-lengths` | SLH-DSA | WOTS+ chain-length / checksum computation |
| `mldsa-rej-eta` | ML-DSA | Nibble-based rejection sampling (`η∈{2,4}`) |

## Running a Single Test

```bash
# Build for the SW baseline
make app PROJECT=tests-power/karats SW_TEST_ENABLED=1

# Run post-synthesis simulation with GPIO-triggered VCD dumping
make questasim-run-postsynth VCD_MODE=2

# Extract the power report for the dumped window (waves-0.vcd = SW)
make power-analysis

# Rebuild with SW_TEST_ENABLED=0, re-simulate, and re-run power-analysis
# on waves-1.vcd to get the HW-accelerated report
```

## Running the Full Sweep

Doing the above by hand for every test and swapping `PWR_VCD` between `waves-0.vcd`/`waves-1.vcd` is tedious and error-prone, so it is automated end-to-end by [`scripts/get_power_res_postsynth.sh`](../../../scripts/get_power_res_postsynth.sh):

```bash
# All tests, both SW and HW
source scripts/get_power_res_postsynth.sh

# Only a subset
source scripts/get_power_res_postsynth.sh --only-tests karats,gf-carryless,thash
```

This produces two power reports per test (`reports_<test>_sw`, `reports_<test>_hw`) under `implementation/power_analysis/`. See [`scripts/README.md`](../../../scripts/README.md) for the full flow — including how the resulting per-block reports are further sliced by [`scripts/extract_power_scopes.py`](../../../scripts/extract_power_scopes.py) and batched by [`scripts/run_extract_power_scopes_batch.py`](../../../scripts/run_extract_power_scopes_batch.py).
