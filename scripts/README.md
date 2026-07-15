# Scripts (`scripts/`)

Build, simulation, FPGA-deployment, and power-analysis automation helpers used on top of the top-level `makefile`. See [`sw/applications/README.md`](../sw/applications/README.md) for the applications these scripts build/run, and [`sw/applications/tests-power/README.md`](../sw/applications/tests-power/README.md) for how the power-analysis scripts are used end to end.

| Script | Purpose |
|---|---|
| [`env.sh`](env.sh) | Environment setup, sourced (not executed) to configure the X-HEEP toolchain: adds the RISC-V toolchain, Verilator, Verible, and OpenOCD to `PATH`. |
| [`sim_all_app.sh`](sim_all_app.sh) | Batch-builds and simulates many applications in one call (RTL / post-synthesis / post-layout), auto-discovering apps or taking an explicit list, with pass/fail summary logging. |
| [`compile_apps.sh`](compile_apps.sh) | Prepares and builds a set of applications for FPGA (`TARGET=zcu104`), optionally patching the `TEST_KEY`/`TEST_SIGN`/`TEST_ENC`/`TEST_DEC` macros in each app's `main.c`, then copies the compiled output to `sw/compiled_apps/sphincsbaseline/`. |
| [`compile_apps_fpga.sh`](compile_apps_fpga.sh) | Same flow as `compile_apps.sh`, targeting a different compiled-output destination (`sw/compiled_apps/pqc_192_256_verify/`) for a separate FPGA verification campaign. |
| [`get_power_res_postsynth.sh`](get_power_res_postsynth.sh) | Runs the full post-synthesis power-analysis sweep over [`sw/applications/tests-power/`](../sw/applications/tests-power/README.md): build → simulate with GPIO-triggered VCD dump → `power-analysis` for both the SW baseline (`waves-0.vcd`) and the HW-accelerated path (`waves-1.vcd`), renaming the report directories per test. |
| [`extract_power_scopes.py`](extract_power_scopes.py) | Post-processes a single hierarchical power report (`crheepto_top_hier.rpt`), keeping only the scopes of interest (`cpu_subsystem_i`, `memory_subsystem_i`, `system_bus_i`, `coproc_wrapper_i`) and their children, and writes a trimmed `crheepto_top_power.rpt` without overwriting existing output files. |
| [`run_extract_power_scopes_batch.py`](run_extract_power_scopes_batch.py) | Runs `extract_power_scopes.py` across every `reports_*` subfolder produced by `get_power_res_postsynth.sh`, so all tests' power reports get trimmed in one pass. |
| [`check_log_synth.sh`](check_log_synth.sh) | Post-synthesis sanity check: scans `synth.log` for errors and inferred latches, and `timing_loop.rpt` for combinational timing loops. |
| [`run_and_notify.sh`](run_and_notify.sh) | Orchestrates the ASIC flow (rebuild → synthesis → post-synthesis sim → PnR → post-layout sim → dummy-fill) behind `ENABLE_*` flags, tails the Innovus PnR log to report stage progress, and pushes notifications to `ntfy.sh/crheepto`. |
| [`sim/compile_uart_dpi.sh`](sim/compile_uart_dpi.sh) | Compiles the UART DPI shared library (`uartdpi.so`) required by QuestaSim to simulate the UART peripheral. |

## Usage

### Batch simulation
```bash
# Post-synthesis simulation of every app under sw/applications
scripts/sim_all_app.sh --postsynthesis

# Explicit list, RTL simulation
scripts/sim_all_app.sh --rtl dilithium-ntt kyber-ntt

# Skip a set of apps when auto-discovering
scripts/sim_all_app.sh --blacklist my_blacklist.csv
```
A test is marked as `PASSED` only if its run log contains the string `TEST SUCCEEDED`. Run `scripts/sim_all_app.sh --help` for the complete flag reference (simulation mode, linker, force rebuild, build-only, dry-run).

### FPGA batch compilation
```bash
scripts/compile_apps.sh pqc/optimized/KEM/ML-KEM ml-kem-512 ml-kem-768
scripts/compile_apps.sh --no-modify pqc/baseline/DS/SLH-DSA SPHINCS-128f-robust
```

### Power-analysis sweep
```bash
# All tests-power apps, SW and HW
source scripts/get_power_res_postsynth.sh

# Only a filtered subset
source scripts/get_power_res_postsynth.sh --only-tests karats,gf-carryless,thash

# Preview the steps without running them
source scripts/get_power_res_postsynth.sh --dry-run
```
Then trim every generated report down to the scopes of interest:
```bash
python3 scripts/run_extract_power_scopes_batch.py --root implementation/power_analysis
```

### Post-synthesis log check
```bash
scripts/check_log_synth.sh
```

### Full ASIC flow with progress notifications
```bash
ENABLE_SYNTH=true ENABLE_PNR=true scripts/run_and_notify.sh
```
