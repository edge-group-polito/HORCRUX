#!/bin/bash

##########################################################################################
#
# Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
#               Valeria Piscopo    - valeria.piscopo@polito.it
# Design Name:  Post-Synthesis Log Checker
# Language:     Bash script
# Date:         April 2026
#
# Description:  Scans synth.log for synthesis errors and inferred latches, and
#               timing_loop.rpt for combinational timing loops after ASIC synthesis.
#
##########################################################################################


# Relative paths
REL_LOG="implementation/synthesis/last_output/report/synth.log"
REL_TIMING="implementation/synthesis/last_output/report/timing_loop.rpt"

# Get absolute paths
LOG_FILE="$(realpath "$REL_LOG" 2>/dev/null || readlink -f "$REL_LOG")"
TIMING_FILE="$(realpath "$REL_TIMING" 2>/dev/null || readlink -f "$REL_TIMING")"

# --- Check if files exist ---
if [ ! -f "$LOG_FILE" ]; then
    echo "❌ File not found: $LOG_FILE"
    exit 1
fi
if [ ! -f "$TIMING_FILE" ]; then
    echo "❌ File not found: $TIMING_FILE"
    exit 1
fi

found=false

# --- Check for "error:" in synth.log ---
if grep -iq "error:" "$LOG_FILE"; then
    found=true
    echo "⚠️  There is an error in $LOG_FILE"
    echo ""
    echo "🔎 Matching 'error:' lines:"
    echo "-------------------------------------------"
    grep -i -C 1 "error:" "$LOG_FILE"
    echo ""
fi

# --- Check for "latch" in synth.log with filters ---
LATCH_LINES=$(grep -i "latch" "$LOG_FILE" | \
              grep -iv "Sequential cell: latch" | \
              grep -iv "|")

if [ -n "$LATCH_LINES" ]; then
    found=true
    echo "🔒 Latch detected in $LOG_FILE"
    echo ""
    echo "🔎 Matching 'latch' lines (excluding safe cases):"
    echo "-------------------------------------------"
    echo "$LATCH_LINES"
    echo ""
fi

# --- Check timing_loop.rpt for "No loops." ---
if grep -q "^No loops\.$" "$TIMING_FILE"; then
    echo "✅ Timing check passed: No loops found in $TIMING_FILE"
else
    found=true
    echo "⛔ Timing check failed: Loops detected in $TIMING_FILE"
    echo ""
    echo "🔎 Suspicious lines:"
    echo "-------------------------------------------"
    grep -i "loop" "$TIMING_FILE"
    echo ""
fi

# --- If nothing bad was found ---
if [ "$found" = false ]; then
    echo "✅ All checks passed successfully!"
fi
