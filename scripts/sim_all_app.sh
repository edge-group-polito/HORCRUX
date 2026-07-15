#!/usr/bin/env bash

##########################################################################################
#
# Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
#               Valeria Piscopo    - valeria.piscopo@polito.it
# Design Name:  Batch Application Simulator
# Language:     Bash script
# Date:         April 2026
#
# Description:  Builds and simulates many applications (RTL / post-synthesis / post-layout),
#               auto-discovering apps or taking an explicit list, with pass/fail summary
#               logging based on the TEST SUCCEEDED marker in each run log.
#
##########################################################################################


# If not running under bash (e.g., invoked via sh), re-exec with bash to support bashisms (arrays, [[ ]], (( ))).
if [ -z "${BASH_VERSION:-}" ]; then
	exec /usr/bin/env bash "$0" "$@"
fi

set -euo pipefail

# Print help and exit
usage() {
	cat <<'EOF'
Usage: scripts/sim_all_app.sh [options] [APP1 APP2 ...]

Options:
	Simulation mode (choose one; default: postsynthesis)
		--rtl | -r             Use RTL        (targets: questasim-build / questasim-run)
		--postsynthesis | -s   Post-synthesis (targets: questasim-build-postsynthesis / questasim-run-postsynth)
		--postlayout | -l      Post-layout    (targets: questasim-build-postlayout / questasim-run-postlayout)

	Linker (default: flash_load)
		--flash | -a           LINKER=flash_load  -> BOOT_MODE=flash
		--onchip | -c          LINKER=on_chip     -> BOOT_MODE=force

	Rebuild / Run control
		--force | -f           Force QuestaSim pre-build even if log shows complete
		--build-only | -b      Only build apps; skip QuestaSim pre-build and run
		--dry-run | -n         Print what would run; do not execute any make

	App selection
		[APP1 APP2 ...]        Explicit app names (overrides discovery + blacklist)
		APPS_CSV=...           Env var with comma-separated apps (overrides discovery + blacklist)
		--blacklist FILE | -x  CSV or newline file with app names to skip (only when auto-discovering)

Behavior:
	- Auto-discovers apps from: sw/applications and hw/vendor/x-heep/sw/applications (merged, de-duped)
	- When explicit apps (args or APPS_CSV) are provided, blacklist is ignored
	- BOOT_MODE derives from LINKER: flash_load->flash, on_chip->force
	- A test is considered PASSED only if the run log contains: "TEST SUCCEEDED"
EOF
}
# Determine repo root based on this script's location
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="${SCRIPT_DIR%/scripts}"

# Defaults
SIM_MODE="postsynthesis"
LINKER="flash_load"
TEST_OK_STRING="TEST SUCCEEDED"
FATAL_CYCLE_ABORT_STRING="Fatal: Simulation aborted due to maximum cycle limit"

# Parse optional key/value CLI args before discovering apps
# Supported switches:
#   Simulation mode (choose ONE, default postsynthesis):
#     --rtl | -r                => SIM_MODE=rtl (target: questasim-build)
#     --postsynthesis | -s      => SIM_MODE=postsynthesis (target: questasim-build-postsynthesis)
#     --postlayout | -l         => SIM_MODE=postlayout (target: questasim-build-postlayout)
#   Linker selection (default flash_load):
#     --flash | -a              => LINKER=flash_load
#     --onchip | -c             => LINKER=on_chip
#   Force QuestaSim rebuild even if log indicates completed:
#     --force | -f              => FORCE rebuild
#   Build-only (skip QuestaSim pre-build and run):
#     --build-only | -b         => only build apps
#   Blacklist apps (applies only when auto-discovering):
#     --blacklist <file> | -x <file>  => CSV or newline-separated app names to skip
FORCE_REBUILD=0
BUILD_ONLY=0
DRY_RUN=0
BLACKLIST_FILE=""
positional=()
while (( $# > 0 )); do
	case "$1" in
		-h|--help)
			usage
			exit 0
			;;
		# Simulation mode switches
		--rtl|-r)
			SIM_MODE="rtl"
			shift
			;;
		--postsynthesis|-s)
			SIM_MODE="postsynth"
			shift
			;;
		--postlayout|-l)
			SIM_MODE="postlayout-timing"
			shift
			;;
		--flash|-a)
			LINKER="flash_load"
			shift
			;;
		--onchip|-c)
			LINKER="on_chip"
			shift
			;;
		--force|-f)
			FORCE_REBUILD=1
			shift
			;;
		--build-only|-b)
			BUILD_ONLY=1
			shift
			;;
		--dry-run|-n)
			DRY_RUN=1
			shift
			;;
		--blacklist|-x)
			if (( $# < 2 )); then
				echo "Error: --blacklist requires a file path" >&2
				exit 2
			fi
			BLACKLIST_FILE="$2"
			shift 2
			;;
		--)
			shift
			while (( $# > 0 )); do positional+=("$1"); shift; done
			;;
		*)
			positional+=("$1")
			shift
			;;
	esac
done
if (( ${#positional[@]} > 0 )); then
	set -- "${positional[@]}"
else
	set --
fi

# Build the APPS array
# Priority:
# 1) CLI args: ./sim_all_app.sh app1 app2
# 2) APPS_CSV env: APPS_CSV="app1,app2"
# 3) Auto-discover subfolders in sw/applications
APPS=()
EXPLICIT_APPS=0
if (( $# > 0 )); then
	# Use command line arguments as app names
	APPS=("$@")
	EXPLICIT_APPS=1
elif [[ -n "${APPS_CSV:-}" ]]; then
	IFS=',' read -r -a APPS <<< "${APPS_CSV}"
	EXPLICIT_APPS=1
else
	# Discover applications from both local and vendor x-heep trees
	LOCAL_APP_DIR="${REPO_ROOT}/sw/applications"
	VENDOR_APP_DIR="${REPO_ROOT}/hw/vendor/x-heep/sw/applications"
	local_apps=()
	vendor_apps=()
	if [[ -d "${LOCAL_APP_DIR}" ]]; then
		# shellcheck disable=SC2207
		local_apps=($(find "${LOCAL_APP_DIR}" -mindepth 1 -maxdepth 1 -type d -printf '%f\n'))
	fi
	if [[ -d "${VENDOR_APP_DIR}" ]]; then
		# shellcheck disable=SC2207
		vendor_apps=($(find "${VENDOR_APP_DIR}" -mindepth 1 -maxdepth 1 -type d -printf '%f\n'))
	fi
	# Merge and de-duplicate
	# shellcheck disable=SC2207
	APPS=($(printf '%s\n' "${local_apps[@]}" "${vendor_apps[@]}" | sort -u))
fi

# Apply blacklist only when apps are auto-discovered
if (( EXPLICIT_APPS == 0 )) && [[ -n "${BLACKLIST_FILE}" ]]; then
	if [[ ! -f "${BLACKLIST_FILE}" ]]; then
		echo "Error: blacklist file not found: ${BLACKLIST_FILE}" >&2
		exit 2
	fi
	echo "🗒️  Loading blacklist from: ${BLACKLIST_FILE}"
	# Read CSV/newline list, trim spaces/comments, drop empties
	BL_CONTENT=$(tr ',' '\n' < "${BLACKLIST_FILE}" | sed 's/#.*$//' | tr -d '\r' | sed 's/^\s*//;s/\s*$//' | sed '/^$/d')
	if [[ -z "${BL_CONTENT}" ]]; then
		echo "ℹ️  Blacklist has no entries (empty or only comments). Proceeding without filtering."
	else
		declare -A BL_SET=()
		while IFS= read -r _bl; do
			BL_SET["${_bl}"]=1
		done <<< "${BL_CONTENT}"
		filtered=()
		for app in "${APPS[@]}"; do
			if [[ -n "${BL_SET[$app]+x}" ]]; then
				echo "⛔ Skipping blacklisted app: '${app}'"
			else
				filtered+=("${app}")
			fi
		done
		APPS=("${filtered[@]}")
	fi
fi

if (( ${#APPS[@]} == 0 )); then
	echo "⚠️  No applications found. Provide app names as args or set APPS_CSV."
	exit 1
fi

echo "🚀 Starting simulations"
echo "• SIM_MODE: ${SIM_MODE}"
echo "• LINKER:   ${LINKER}"
echo "🧪 Apps to simulate: ${APPS[*]}"

# Logging layout
LOG_ROOT="${REPO_ROOT}/build/logs"
RUN_LOG_ROOT="${LOG_ROOT}/run_all_sim"
SUMMARY_LOG="${LOG_ROOT}/run_all_sim.summary.log"
mkdir -p "${RUN_LOG_ROOT}"

if (( BUILD_ONLY == 0 )); then
	# Pre-build QuestaSim artifacts quietly with logging
	QBUILD_LOG="${RUN_LOG_ROOT}/questasim-build-${SIM_MODE}.log"
	# Marker string that indicates the QuestaSim build is complete and reusable.
	QBUILD_OK_STRING="Errors: 0"
	if [[ "${SIM_MODE}" == "rtl" ]]; then
		QBUILD_TARGET="questasim-build"
	else
		QBUILD_TARGET="questasim-build-${SIM_MODE}"
	fi

	# Skip build if already complete unless forced
	if (( FORCE_REBUILD == 0 )) && [[ -f "${QBUILD_LOG}" ]] && [[ -n "${QBUILD_OK_STRING}" ]] && grep -q -- "${QBUILD_OK_STRING}" "${QBUILD_LOG}"; then
		echo "⏭️  Skipping QuestaSim build (target='${QBUILD_TARGET}') — already complete per log"
	else
		if (( FORCE_REBUILD == 1 )); then
			echo "🔁 Force rebuild requested (-f)."
		fi
		echo "🛠️  Building QuestaSim artifacts (target='${QBUILD_TARGET}')…"
		if (( DRY_RUN == 1 )); then
			echo "[dry-run] make ${QBUILD_TARGET} > ${QBUILD_LOG} 2>&1"
		elif make "${QBUILD_TARGET}" >"${QBUILD_LOG}" 2>&1; then
			echo "✅ QuestaSim build OK"
		else
			echo "❌ QuestaSim build FAILED"
			echo "Aborting before running app builds."
			exit 1
		fi
	fi
else
	echo "⏭️  Build-only mode: skipping QuestaSim pre-build"
fi

declare -a FAILED_APPS=()
declare -a FAILED_BUILDS=()
declare -a FAILED_RUNS=()
declare -a FAILED_TESTS=()

# Determine run target based on SIM_MODE
if [[ "${SIM_MODE}" == "rtl" ]]; then
	QRUN_TARGET="questasim-run"
elif [[ "${SIM_MODE}" == "postsynth" ]]; then
	QRUN_TARGET="questasim-run-postsynth"
else
	QRUN_TARGET="questasim-run-postlayout-timing"
fi

# Determine BOOT_MODE from LINKER
case "${LINKER}" in
	flash_load) BOOT_MODE="flash" ;;
	on_chip)    BOOT_MODE="force" ;;
	*)          BOOT_MODE="force"; echo "⚠️  Unknown LINKER='${LINKER}', defaulting BOOT_MODE='force'" ;;
esac

for app in "${APPS[@]}"; do
	echo "🔧 Testing app: '${app}' (MODE=${SIM_MODE}, LINKER=${LINKER}, BOOT_MODE=${BOOT_MODE})"
	APP_LOG_DIR="${RUN_LOG_ROOT}/${app}"
	mkdir -p "${APP_LOG_DIR}"
	LOG_FILE="${APP_LOG_DIR}/build.log"
	# Suppress make output; write to per-app log, and capture success/failure
	if (( DRY_RUN == 1 )); then
		echo "[dry-run] make app PROJECT=${app} LINKER=${LINKER} > ${LOG_FILE} 2>&1"
		build_ok=1
	elif make app PROJECT="${app}" LINKER="${LINKER}" >"${LOG_FILE}" 2>&1; then
		build_ok=1
	else
		build_ok=0
	fi
	if (( build_ok == 1 )); then
		echo "✅ Build: SUCCESS"
		if (( BUILD_ONLY == 0 )); then
			# Run QuestaSim for this app
			RUN_LOG_FILE="${APP_LOG_DIR}/run.log"
			echo "▶️  Running simulation target '${QRUN_TARGET}' for '${app}'…"
			# Always run the simulation command and capture its output to the run log.
			# We will not rely solely on the `make` exit code to decide test success.
			if (( DRY_RUN == 1 )); then
				echo "[dry-run] make ${QRUN_TARGET} PROJECT=${app} BOOT_MODE=${BOOT_MODE} > ${RUN_LOG_FILE} 2>&1"
				# In dry-run mode we can't inspect a real log; treat as skipped for run-time checks
			else
				# Invoke make and always write stdout/stderr to the run log. We ignore the exit
				# code here for pass/fail detection; instead we'll scan the log for the test marker.
				make "${QRUN_TARGET}" PROJECT="${app}" BOOT_MODE="${BOOT_MODE}" >"${RUN_LOG_FILE}" 2>&1 || true
				# Inform about run command completion
				echo "▶️  Run finished (logs: ${RUN_LOG_FILE}). Inspecting for test marker..."
				# Check test pass marker in the run log
				if [[ -f "${RUN_LOG_FILE}" ]] && grep -q -- "${TEST_OK_STRING}" "${RUN_LOG_FILE}"; then
					echo "✅ Test: PASSED"
				else
					# Provide a more informative reason if available
					if [[ -f "${RUN_LOG_FILE}" ]] && grep -q -- "${FATAL_CYCLE_ABORT_STRING}" "${RUN_LOG_FILE}"; then
						echo "❌ Test: FAILED — simulation aborted due to maximum cycle limit"
					else
						echo "❌ Test: FAILED"
					fi
					FAILED_APPS+=("${app}")
					# Record as test failure (not a build/run failure)
					FAILED_TESTS+=("${app}")
				fi
			fi
		else
			echo "⏭️  Build-only mode: skipping run for '${app}'"
		fi
	else
		echo "❌ Build: FAILURE"
		FAILED_APPS+=("${app}")
		FAILED_BUILDS+=("${app}")
	fi
done

echo
# Accumulate and print the summary, then save it to a log file
SUMMARY_TEXT=""
add_summary() { SUMMARY_TEXT+="$1"$'\n'; echo "$1"; }

add_summary "📊 Summary Report"
add_summary "📦 Total apps: ${#APPS[@]}"
add_summary "🧱 Build failures: ${#FAILED_BUILDS[@]}"
add_summary "🚫 Run failures:   ${#FAILED_RUNS[@]}"
add_summary "🧪 Test failures:  ${#FAILED_TESTS[@]}"
TOTAL_FAILS=$(( ${#FAILED_BUILDS[@]} + ${#FAILED_RUNS[@]} + ${#FAILED_TESTS[@]} ))
if (( TOTAL_FAILS > 0 )); then
	if (( ${#FAILED_BUILDS[@]} > 0 )); then
		add_summary "   • build: ${FAILED_BUILDS[*]}"
	fi
	if (( ${#FAILED_RUNS[@]} > 0 )); then
		add_summary "   • run:   ${FAILED_RUNS[*]}"
	fi
	if (( ${#FAILED_TESTS[@]} > 0 )); then
		add_summary "   • test:  ${FAILED_TESTS[*]}"
	fi
fi

# Save the summary as displayed
mkdir -p "${LOG_ROOT}"
printf "%s" "${SUMMARY_TEXT}" > "${SUMMARY_LOG}"
echo "📝 Summary saved to: ${SUMMARY_LOG}"

# Exit non-zero if any failure occurred
(( TOTAL_FAILS == 0 )) || exit 1

echo "🎉 All done."


