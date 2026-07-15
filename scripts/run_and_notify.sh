#!/usr/bin/env bash

##########################################################################################
#
# Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
#               Valeria Piscopo    - valeria.piscopo@polito.it
# Design Name:  ASIC Flow Orchestrator and Notifier
# Language:     Bash script
# Date:         April 2026
#
# Description:  Drives the rebuild / synthesis / post-synthesis-sim / PnR / post-layout-sim /
#               dummy-fill flow behind ENABLE_* flags, watches the Innovus PnR log for stage
#               progress, and pushes notifications to ntfy.sh/crheepto.
#
##########################################################################################

set -euo pipefail

# ---- Flow control flags ----
ENABLE_REBUILD=false
ENABLE_SYNTH=false
ENABLE_POSTSYNTH_SIM=false          # postsynthesis sim (SRAM boot)
ENABLE_POSTSYNTH_SIM_FLASH=false    # postsynthesis sim (flash boot)
ENABLE_PNR=true
ENABLE_POSTPNR_SIM=false             # postlayout sim (SRAM boot)
ENABLE_POSTPNR_SIM_FLASH=true       # postlayout sim (flash boot)
ENABLE_DUMMYFILL=true

# Dry run flag: when true and PnR is enabled, we will NOT invoke the real make pnr
# but instead simulate an Innovus log emitting the expected stage banners so that
# the watcher & notifications can be validated quickly.
DRY_RUN=${DRY_RUN:-false}

notify() {
  local title="$1"
  local body="${2:-}"
  curl -sS -o /dev/null \
    -H "Title: ${title}" \
    -d "${body:-${title}}" \
    ntfy.sh/crheepto || true
}

# Watch Innovus PnR progress by tailing the latest log and emitting checkpoints.
# A checkpoint is detected when a line matches: @file <n>: source scripts/<step>.tcl
# Assumes build/innovus_latest -> latest run directory containing innovus.log[v].
watch_pnr_progress() {
  local link_dir="build/innovus_latest"
  local start_time=$(date +%s)
  local latest_dir=""
  local poll_sleep=5

  # Wait for latest dir (symlink or folder) to appear
  while true; do
    if [ -L "${link_dir}" ] || [ -d "${link_dir}" ]; then
      latest_dir=$(readlink -f "${link_dir}" 2>/dev/null || echo "${link_dir}")
      [ -d "${latest_dir}" ] && break
    fi
    sleep ${poll_sleep}
  done

  # Define ordered list of expected PnR stages (uppercase as printed by log_stage)
  # Keep in sync with run_pnr_flow.tcl log_stage calls
  local -a steps=(
    "SETUP LIBRARIES AND READ DESIGN"
    "FLOORPLAN"
    "POWERGRID"
    "PLACEMENT"
    "CTS"
    "ROUTE"
    "OPTIMIZE DESIGN"
    "EXPORT DESIGN"
  )
  local total=${#steps[@]}

  # Choose a log file candidate (prefer verbose)
  local logfile=""
  for cand in "${latest_dir}/innovus.logv" "${latest_dir}/innovus.log" "${latest_dir}/innovus.logv1"; do
    [ -f "$cand" ] && { logfile="$cand"; break; }
  done
  # Wait for log file to exist
  while [ ! -f "${logfile}" ]; do
    sleep ${poll_sleep}
    for cand in "${latest_dir}/innovus.logv" "${latest_dir}/innovus.log" "${latest_dir}/innovus.logv1"; do
      [ -f "$cand" ] && { logfile="$cand"; break; }
    done
  done

  echo "[PnR] Progress watcher attached to ${logfile} (known steps: ${total})"
  notify "PnR progress" "Watcher attached (known steps: ${total})."

  # Tail and parse
  # Use stdbuf to avoid buffering delays if available
  if command -v stdbuf >/dev/null 2>&1; then
    tail_cmd=(stdbuf -oL -eL tail -n 0 -F "${logfile}")
  else
    tail_cmd=(tail -n 0 -F "${logfile}")
  fi

  "${tail_cmd[@]}" 2>/dev/null | while IFS= read -r line; do
    # Strip ANSI escape sequences for robust matching
    local clean
    clean=$(echo -e "$line" | sed -r 's/\x1B\[[0-9;]*[A-Za-z]//g')
    # Match lines like: [YYYY-MM-DD HH:MM:SS] STARTING STAGE <NAME> (<secs> sec)
    if [[ $clean =~ \[[0-9]{4}-[0-9]{2}-[0-9]{2}\ [0-9]{2}:[0-9]{2}:[0-9]{2}\]\ STARTING\ STAGE\ ([A-Z0-9\ \-]+)\ \([0-9]+\ sec\) ]]; then
      local stage="${BASH_REMATCH[1]}"
      # Trim trailing spaces (just in case)
      stage="${stage%% }"
      local idx=""
      if [ $total -gt 0 ]; then
        local i
        for i in "${!steps[@]}"; do
          if [ "${steps[$i]}" = "$stage" ]; then
            idx=$((i+1))
            break
          fi
        done
      fi
      local elapsed=$(( $(date +%s) - start_time ))
      local h=$((elapsed/3600)); local m=$(((elapsed%3600)/60)); local s=$((elapsed%60))
      local etime
      printf -v etime "%02d:%02d:%02d" "$h" "$m" "$s"
      local msg="[PnR] Stage"
      if [ -n "$idx" ] && [ $total -gt 0 ]; then
        msg+=" ${idx}/${total}"
      fi
      msg+=": ${stage} (elapsed ${etime})"
      echo "$msg"
      notify "PnR progress" "$msg"
    fi
  done
}

# Simulate a PnR log by creating a fake build/innovus_latest directory with an
# innovus.log file and appending stage banner lines similar to those produced
# by log_stage in crheepto.tcl. This lets us exercise the watcher & curl notifications.
simulate_pnr_log() {
  local sim_root="build/sim_innovus_$(date +%Y%m%d_%H%M%S)"
  mkdir -p "$sim_root/artefacts" "$sim_root/scripts"
  : > "$sim_root/innovus.log"  # create empty log
  ln -sfn "$sim_root" build/innovus_latest
  echo "[DRY-RUN] Created simulation directory: $sim_root"

  local -a stages=(
    "SETUP LIBRARIES AND READ DESIGN"
    "FLOORPLAN"
    "POWERGRID"
    "PLACEMENT"
    "CTS"
    "ROUTE"
    "OPTIMIZE DESIGN"
    "EXPORT DESIGN"
  )

  local logfile="$sim_root/innovus.log"
  local i=0
  for st in "${stages[@]}"; do
    i=$((i+1))
    # Mimic the colored output (bold white + timestamp + green stage name); ANSI will be stripped by watcher.
    local ts="$(date +%Y-%m-%d\ %H:%M:%S)"
    local epoch="$(date +%s)"
    printf '\033[1m\033[37m###\033[m\n' >> "$logfile"
    printf '\033[1m\033[37m[%s] STARTING STAGE \033[32m%s\033[m (%s sec)\n' "$ts" "$st" "$epoch" >> "$logfile"
    printf '\033[1m\033[37m###\033[m\n' >> "$logfile"
    # Add some filler lines that should NOT match
    echo "Info: doing work for stage $st ..." >> "$logfile"
    # Short sleep to let watcher process
    sleep 1
  done
  echo "[DRY-RUN] Simulation complete. Log at $logfile"
}

# Run a command quietly (suppress stdout/stderr) but fail on errors
run_quiet() {
  "$@" > /dev/null 2>&1
}

echo "🚀 Starting crheepto flow"
echo "---------------------------------"

if [ "$ENABLE_REBUILD" = true ]; then
  echo "🧹 Cleaning and generating…"
  echo "🧹 Cleaning workspace…"
  run_quiet make clean
  echo "🧩 Generating project artifacts…"
  run_quiet make crheepto-gen
fi 

if [ "$ENABLE_SYNTH" = true ]; then
  notify "Start synthesis" "Starting synthesis."
  echo "⚙️  Running synthesis…"
    run_quiet make synthesis

  # Check synthesis logs and notify; abort on failure
  tmp_out=$(mktemp)
  if ./scripts/check_log_synth.sh | tee "$tmp_out"; then
    notify "Synthesis check passed" "$(cat "$tmp_out")"
    echo "✅ Synthesis checks passed."
  else
    notify "Synthesis check FAILED" "$(cat "$tmp_out")"
    rm -f "$tmp_out"
    echo "❌ Synthesis checks failed."
    exit 1
  fi
  rm -f "$tmp_out"
fi


if [ "$ENABLE_POSTSYNTH_SIM" = true ]; then
  notify "Running postsynthesis (SRAM)" "Running postsynthesis simulation without flash."
    echo "🧰 Building app (SRAM boot)…"
    run_quiet make app
    echo "🔬 Running postsynthesis simulation (SRAM boot)…"
    run_quiet make questasim-sim-postsynth

  # Check postsynthesis simulation logs (SRAM boot)
  POSTSYN_DIR="build/polito_vlsi_crheepto_0/sim_postsynthesis-modelsim"
  TRANSCRIPT_FILE="${POSTSYN_DIR}/transcript"
  UART_FILE="${POSTSYN_DIR}/uart.log"

  postsyn_ok=true
  postsyn_msg=""

  if [ ! -f "${TRANSCRIPT_FILE}" ]; then
    postsyn_ok=false
    postsyn_msg+="Missing transcript: ${TRANSCRIPT_FILE}\n"
  fi
  if [ ! -f "${UART_FILE}" ]; then
    postsyn_ok=false
    postsyn_msg+="Missing UART log: ${UART_FILE}\n"
  fi

  if [ "${postsyn_ok}" = true ]; then
    if ! grep -q "TEST SUCCEEDED" "${TRANSCRIPT_FILE}"; then
      postsyn_ok=false
      postsyn_msg+="Transcript does not contain 'TEST SUCCEEDED'.\n"
    fi
    if ! grep -q "hello world!" "${UART_FILE}"; then
      postsyn_ok=false
      postsyn_msg+="UART log does not contain 'hello world!'.\n"
    fi
  fi

  if [ "${postsyn_ok}" = true ]; then
    notify "Postsynthesis (SRAM) simulation check passed" "Both conditions met (TEST SUCCEEDED, hello world!)."
    echo "✅ Postsynthesis (SRAM) simulation passed."
  else
    notify "Postsynthesis (SRAM) simulation check FAILED" "${postsyn_msg}"
    echo "❌ Postsynthesis (SRAM) simulation failed."
    exit 1
  fi
fi

if [ "$ENABLE_POSTSYNTH_SIM_FLASH" = true ]; then
  notify "Running postsynthesis (flash)" "Running postsynthesis simulation with flash."
    echo "🧰 Building app (flash boot)…"
    run_quiet make app LINKER=flash_load
    echo "🔬 Running postsynthesis simulation (flash boot)…"
    run_quiet make questasim-sim-postsynth BOOT_MODE=flash

  # Check postsynthesis simulation logs (flash boot)
  POSTSYN_DIR="build/polito_vlsi_crheepto_0/sim_postsynthesis-modelsim"
  TRANSCRIPT_FILE="${POSTSYN_DIR}/transcript"
  UART_FILE="${POSTSYN_DIR}/uart.log"

  postsyn_ok=true
  postsyn_msg=""

  if [ ! -f "${TRANSCRIPT_FILE}" ]; then
    postsyn_ok=false
    postsyn_msg+="Missing transcript: ${TRANSCRIPT_FILE}\n"
  fi
  if [ ! -f "${UART_FILE}" ]; then
    postsyn_ok=false
    postsyn_msg+="Missing UART log: ${UART_FILE}\n"
  fi

  if [ "${postsyn_ok}" = true ]; then
    if ! grep -q "TEST SUCCEEDED" "${TRANSCRIPT_FILE}"; then
      postsyn_ok=false
      postsyn_msg+="Transcript does not contain 'TEST SUCCEEDED'.\n"
    fi
    if ! grep -q "hello world!" "${UART_FILE}"; then
      postsyn_ok=false
      postsyn_msg+="UART log does not contain 'hello world!'.\n"
    fi
  fi

  if [ "${postsyn_ok}" = true ]; then
    notify "Postsynthesis (flash) simulation check passed" "Both conditions met (TEST SUCCEEDED, hello world!)."
    echo "✅ Postsynthesis (flash) simulation passed."
  else
    notify "Postsynthesis (flash) simulation check FAILED" "${postsyn_msg}"
    echo "❌ Postsynthesis (flash) simulation failed."
    exit 1
  fi
fi


if [ "$ENABLE_PNR" = true ]; then
  if [ "$DRY_RUN" = true ]; then
    notify "PnR dry-run" "Simulating PnR stages (no real flow)."
    echo "🏗️  DRY-RUN: Simulating PnR stages…"
    watch_pnr_progress &
    PNR_WATCHER_PID=$!
    simulate_pnr_log
    # Allow last lines to be consumed
    sleep 2
    kill "$PNR_WATCHER_PID" 2>/dev/null || true
    notify "PnR dry-run complete" "Simulation finished."
    echo "🏁 DRY-RUN: PnR stage simulation completed."
  else
    notify "PnR started" "Starting place-and-route."
    echo "🏗️  Running PnR…"
    # Start watcher in background
    watch_pnr_progress &
    PNR_WATCHER_PID=$!
    # Ensure watcher is cleaned up
    cleanup_pnr_watcher() { kill "$PNR_WATCHER_PID" 2>/dev/null || true; }
    trap cleanup_pnr_watcher EXIT INT TERM
    run_quiet make pnr
    cleanup_pnr_watcher
    notify "PnR done" "PnR finished, running postlayout simulation."
    echo "🏁 PnR completed."
  fi
fi

# Check postlayout simulation logs (SRAM boot)
POSTLAY_DIR="build/polito_vlsi_crheepto_0/sim_postlayout-modelsim"
POSTLAY_TRANSCRIPT_FILE="${POSTLAY_DIR}/transcript"
POSTLAY_UART_FILE="${POSTLAY_DIR}/uart.log"

if [ "$ENABLE_POSTPNR_SIM" = true ]; then
  notify "Running postlayout (SRAM)" "Running postlayout simulation without flash."
    echo "🧰 Building app (SRAM boot) for postlayout…"
    run_quiet make app
    echo "🧪 Running postlayout simulation (SRAM boot)…"
    run_quiet make questasim-sim-postlayout

  postlay_ok=true
  postlay_msg=""

  if [ ! -f "${POSTLAY_TRANSCRIPT_FILE}" ]; then
    postlay_ok=false
    postlay_msg+="Missing transcript: ${POSTLAY_TRANSCRIPT_FILE}\n"
  fi
  if [ ! -f "${POSTLAY_UART_FILE}" ]; then
    postlay_ok=false
    postlay_msg+="Missing UART log: ${POSTLAY_UART_FILE}\n"
  fi

  if [ "${postlay_ok}" = true ]; then
    if ! grep -q "TEST SUCCEEDED" "${POSTLAY_TRANSCRIPT_FILE}"; then
      postlay_ok=false
      postlay_msg+="Transcript does not contain 'TEST SUCCEEDED'.\n"
    fi
    if ! grep -q "hello world!" "${POSTLAY_UART_FILE}"; then
      postlay_ok=false
      postlay_msg+="UART log does not contain 'hello world!'.\n"
    fi
  fi

  if [ "${postlay_ok}" = true ]; then
    notify "Postlayout simulation check passed" "Both conditions met (TEST SUCCEEDED, hello world!). Running postlayout with flash."
    echo "✅ Postlayout (SRAM) simulation passed."
  else
    notify "Postlayout simulation check FAILED" "${postlay_msg}"
    echo "❌ Postlayout (SRAM) simulation failed."
    exit 1
  fi
fi

if [ "$ENABLE_POSTPNR_SIM_FLASH" = true ]; then
  notify "Running postlayout (flash)" "Running postlayout simulation with flash."
    echo "🧰 Building app (flash boot) for postlayout…"
    run_quiet make app LINKER=flash_load
    echo "🧪 Running postlayout simulation (flash boot)…"
    run_quiet make questasim-sim-postlayout BOOT_MODE=flash

  # Re-check postlayout simulation logs after flash boot
  postlay_ok=true
  postlay_msg=""

  if [ ! -f "${POSTLAY_TRANSCRIPT_FILE}" ]; then
    postlay_ok=false
    postlay_msg+="Missing transcript: ${POSTLAY_TRANSCRIPT_FILE}\n"
  fi
  if [ ! -f "${POSTLAY_UART_FILE}" ]; then
    postlay_ok=false
    postlay_msg+="Missing UART log: ${POSTLAY_UART_FILE}\n"
  fi

  if [ "${postlay_ok}" = true ]; then
    if ! grep -q "TEST SUCCEEDED" "${POSTLAY_TRANSCRIPT_FILE}"; then
      postlay_ok=false
      postlay_msg+="Transcript does not contain 'TEST SUCCEEDED'.\n"
    fi
    if ! grep -q "hello world!" "${POSTLAY_UART_FILE}"; then
      postlay_ok=false
      postlay_msg+="UART log does not contain 'hello world!'.\n"
    fi
  fi

  if [ "${postlay_ok}" = true ]; then
    notify "Postlayout (flash) simulation check passed" "Both conditions met (TEST SUCCEEDED, hello world!)."
    echo "✅ Postlayout (flash) simulation passed."
  else
    notify "Postlayout (flash) simulation check FAILED" "${postlay_msg}"
    echo "❌ Postlayout (flash) simulation failed."
    exit 1
  fi
fi

if [ "$ENABLE_DUMMYFILL" = true ]; then
  notify "Dummy fill started" "Starting dummy fill."
  echo "🧱 Running dummy fill…"
    run_quiet make dummyfill
  notify "Dummy fill done" "Dummy fill finished."
fi
