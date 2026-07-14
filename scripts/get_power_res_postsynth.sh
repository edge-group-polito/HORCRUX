#!/usr/bin/env bash
# get_power_res_postsynth
# Iterate over subfolders in sw/applications/tests-power and for each test:
#   1) make app-tests-power-<test_name>
#   2) make questasim-run-postsynth VCD_MODE=2
#   3) make power-analysis (SW: waves-0.vcd)
#   4) Rename reports -> reports_<test_name>_sw
#   5) Change makefile to use waves-1.vcd
#   6) make power-analysis (HW: waves-1.vcd)
#   7) Rename reports -> reports_<test_name>_hw
#   8) Restore makefile to waves-0.vcd
#
# Fixed paths:
#   - Makefile directory: "."
#   - Tests-power dir: "sw/applications/tests-power"
#   - Reports dir: "implementation/power_analysis/reports"
#
# Optional filters:
#   --only-tests keccak,cbd_eta2,... (comma-separated list)
#   --dry-run (print actions only)

set -euo pipefail

# ---------- Fixed configuration ----------
MAKE_CWD="."
TESTS_POWER_DIR="sw/applications/tests-power"
REPORTS_DIR="implementation/power_analysis/reports"
MAKEFILE="makefile"

# ---------- Defaults for optional filters ----------
ONLY_TESTS=""
DRY_RUN=0

say() { printf "[%s] %s\n" "$(date +'%F %T')" "$*"; }
die() { echo "Error: $*" >&2; exit 1; }
run() { if [[ $DRY_RUN -eq 1 ]]; then echo "DRYRUN: $*"; else eval "$@"; fi; }

usage() {
  cat <<EOF
Usage: $0 [--only-tests LIST] [--dry-run]

Fixed paths:
  Makefile dir   : ${MAKE_CWD}
  Tests-power dir: ${TESTS_POWER_DIR}
  Reports dir    : ${REPORTS_DIR}

Flow (for each test subfolder):
  1) Build   : make app-tests-power-<test_name>
  2) Simulate: make questasim-run-postsynth VCD_MODE=2
  3) Power SW: make power-analysis (using waves-0.vcd)
  4) Rename  : ${REPORTS_DIR} -> ${REPORTS_DIR}_<test_name>_sw
  5) Modify  : PWR_VCD to use waves-1.vcd
  6) Power HW: make power-analysis (using waves-1.vcd)
  7) Rename  : ${REPORTS_DIR} -> ${REPORTS_DIR}_<test_name>_hw
  8) Restore : PWR_VCD back to waves-0.vcd
EOF
}

# ---------- Parse args ----------
while [[ $# -gt 0 ]]; do
  case "$1" in
    --only-tests)
      ONLY_TESTS="$2"
      shift 2
      ;;
    --dry-run)
      DRY_RUN=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      die "Unknown arg: $1"
      ;;
  esac
done

[[ -d "$MAKE_CWD" ]] || die "Make directory not found: $MAKE_CWD"
[[ -d "$TESTS_POWER_DIR" ]] || die "Tests-power directory not found: $TESTS_POWER_DIR"
[[ -f "$MAKEFILE" ]] || die "Makefile not found: $MAKEFILE"

# ---------- Build filters ----------
declare -A ONLY_MAP=()
if [[ -n "$ONLY_TESTS" ]]; then
  IFS=',' read -r -a _only_arr <<< "$ONLY_TESTS"
  for t in "${_only_arr[@]}"; do
    t_trim="${t//[[:space:]]/}"
    [[ -n "$t_trim" ]] && ONLY_MAP["$t_trim"]=1
  done
fi

# ---------- Discover test subfolders ----------
declare -a TEST_ARR=()
for dir in "$TESTS_POWER_DIR"/*/; do
  [[ -d "$dir" ]] || continue
  test_name="$(basename "$dir")"
  # Skip if filtering and test not in list
  if [[ ${#ONLY_MAP[@]} -gt 0 && -z "${ONLY_MAP[$test_name]:-}" ]]; then
    continue
  fi
  TEST_ARR+=( "$test_name" )
done

# Sort tests alphabetically
IFS=$'\n' TEST_ARR=($(sort <<<"${TEST_ARR[*]}")); unset IFS

(( ${#TEST_ARR[@]} > 0 )) || die "No test subfolders found in ${TESTS_POWER_DIR} after filters."

say "Discovered ${#TEST_ARR[@]} test(s) in ${TESTS_POWER_DIR}:"
for t in "${TEST_ARR[@]}"; do
  say "  - $t"
done

# ---------- Helper: switch PWR_VCD between waves-0 and waves-1 ----------
set_pwr_vcd() {
  local vcd_num="$1"  # 0 or 1
  say "Setting PWR_VCD to waves-${vcd_num}.vcd"
  if [[ $DRY_RUN -eq 1 ]]; then
    echo "DRYRUN: sed -i 's|waves-[01]\\.vcd|waves-${vcd_num}.vcd|' \"$MAKEFILE\""
  else
    sed -i "s|waves-[01]\\.vcd|waves-${vcd_num}.vcd|" "$MAKEFILE"
  fi
}

# ---------- Main loop: for each test ----------
say "=== Starting power analysis flow ==="
for test_name in "${TEST_ARR[@]}"; do
  say "========================================"
  say "Processing test: ${test_name}"
  say "========================================"

  # Step 1: Build the test application
  say "[Step 1] Build: make app PROJECT=tests-power/${test_name}"
  run "make -C \"$MAKE_CWD\" app PROJECT=tests-power/${test_name}"

  # Step 2: Run questasim post-synthesis simulation
  say "[Step 2] Simulate: make questasim-run-postsynth VCD_MODE=2"
  run "make -C \"$MAKE_CWD\" questasim-run-postsynth VCD_MODE=2"

  # Step 3: Power analysis (SW - waves-0.vcd)
  say "[Step 3] Power analysis (SW - waves-0.vcd)"
  set_pwr_vcd 0
  run "make -C \"$MAKE_CWD\" power-analysis"

  # Step 4: Rename reports to _sw
  if [[ -d "$REPORTS_DIR" || $DRY_RUN -eq 1 ]]; then
    dest_sw="${REPORTS_DIR}_${test_name}_sw"
    say "[Step 4] Renaming \"$REPORTS_DIR\" -> \"$dest_sw\""
    run "rm -rf \"$dest_sw\""
    run "mv \"$REPORTS_DIR\" \"$dest_sw\""
  else
    say "WARNING: Reports dir not found: $REPORTS_DIR (skipping rename for SW)"
  fi

  # Step 5: Switch to waves-1.vcd for HW analysis
  say "[Step 5] Switching to waves-1.vcd for HW analysis"
  set_pwr_vcd 1

  # Step 6: Power analysis (HW - waves-1.vcd)
  say "[Step 6] Power analysis (HW - waves-1.vcd)"
  run "make -C \"$MAKE_CWD\" power-analysis"

  # Step 7: Rename reports to _hw
  if [[ -d "$REPORTS_DIR" || $DRY_RUN -eq 1 ]]; then
    dest_hw="${REPORTS_DIR}_${test_name}_hw"
    say "[Step 7] Renaming \"$REPORTS_DIR\" -> \"$dest_hw\""
    run "rm -rf \"$dest_hw\""
    run "mv \"$REPORTS_DIR\" \"$dest_hw\""
  else
    say "WARNING: Reports dir not found: $REPORTS_DIR (skipping rename for HW)"
  fi

  # Restore waves-0.vcd for next iteration
  say "[Cleanup] Restoring PWR_VCD to waves-0.vcd"
  set_pwr_vcd 0

  say "Completed: ${test_name}"
done

say "========================================"
say "All ${#TEST_ARR[@]} tests completed!"
say "========================================"
