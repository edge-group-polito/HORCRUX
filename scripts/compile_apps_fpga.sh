#!/usr/bin/env bash

##########################################################################################
#
# Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
#               Valeria Piscopo    - valeria.piscopo@polito.it
# Design Name:  FPGA Batch App Compiler (verification campaign)
# Language:     Bash script
# Date:         April 2026
#
# Description:  Builds a list of applications for the zcu104 FPGA target, optionally
#               patching the TEST_KEY/TEST_SIGN/TEST_ENC/TEST_DEC macros in main.c,
#               and copies the compiled output to sw/compiled_apps/pqc_192_256_verify/.
#
##########################################################################################

# compile_apps_fpga.sh
# Usage:
#   ./compile_apps_fpga.sh [--no-modify|-n] <app_folder> <app_name1> [app_name2 ...]
#
# For each app_name:
#   - Verify it exists in sw/applications/<app_folder>/<app_name>
#   - (Optionally) modify main.c inside it
#   - Run make app PROJECT=<app_folder>/<app_name> LINKER=on_chip TARGET=zcu104
#   - Copy SECOND_SRC_FOLDER to SECOND_DST_PARENT/<app_name><SUFFIX>

set -euo pipefail

### === FIXED CONFIG (edit if needed) === ###

BASE_APPLICATIONS="sw/applications"
REL_FILE_IN_APP="main.c"

TEST_KEY=0
TEST_SIGN=1
TEST_SIGNOPEN=0
TEST_ENC=1
TEST_DEC=0

SUFFIX=""

SECOND_SRC_FOLDER="build/sw/app"
SECOND_DST_PARENT="sw/compiled_apps/pqc_192_256_verify"

### ===================================== ###

have() { command -v "$1" >/dev/null 2>&1; }

copy_dir() {
  local src="$1" dst="$2"
  mkdir -p "$(dirname "$dst")"
  if have rsync; then
    rsync -a --delete "$src"/ "$dst"/
  else
    mkdir -p "$dst"
    cp -a "$src"/. "$dst"/
  fi
}

# -------------------- argument parsing --------------------

MODIFY_MAIN=1

while (( $# > 0 )); do
  case "$1" in
    --no-modify|-n)
      MODIFY_MAIN=0
      shift
      ;;
    --modify|-m)
      MODIFY_MAIN=1
      shift
      ;;
    --help|-h)
      echo "Usage: $0 [--no-modify|-n] <app_folder> <app_name1> [app_name2 ...]"
      exit 0
      ;;
    --) # end of options
      shift
      break
      ;;
    -*) # unknown option
      echo "ERROR: Unknown option: $1"
      echo "Usage: $0 [--no-modify|-n] <app_folder> <app_name1> [app_name2 ...]"
      exit 1
      ;;
    *) # first non-option -> stop parsing flags
      break
      ;;
  esac
done

if (( $# < 2 )); then
  echo "Usage: $0 [--no-modify|-n] <app_folder> <app_name1> [app_name2 ...]"
  exit 1
fi

APP_FOLDER="$1"
shift
APP_NAMES=("$@")

APP_FOLDER_PATH="$BASE_APPLICATIONS/$APP_FOLDER"

if [[ ! -d "$APP_FOLDER_PATH" ]]; then
  echo "ERROR: App folder '$APP_FOLDER_PATH' not found."
  exit 1
fi

# -------------------- main loop --------------------

for APP_NAME in "${APP_NAMES[@]}"; do

  APP_PATH="$APP_FOLDER_PATH/$APP_NAME"

  if [[ ! -d "$APP_PATH" ]]; then
    echo "ERROR: App '$APP_NAME' not found in '$APP_FOLDER_PATH'."
    continue
  fi

  TARGET_FILE="$APP_PATH/$REL_FILE_IN_APP"

  if [[ ! -f "$TARGET_FILE" ]]; then
    echo "ERROR: File '$TARGET_FILE' not found."
    continue
  fi

  echo "==> Processing app: $APP_FOLDER/$APP_NAME"

  if (( MODIFY_MAIN )); then
    echo "==> Modifying macros in '$TARGET_FILE'..."
    # Create exactly one backup, then edit the working file.
    cp -p "$TARGET_FILE" "${TARGET_FILE}.bak"

    sed -i -E "s|(TEST_KEY[[:space:]]+)[0-9]+|\1$TEST_KEY|g" "$TARGET_FILE"

    # Try TEST_SIGN first; if not found, try TEST_ENC
    if grep -qE 'TEST_SIGN[[:space:]]+[0-9]+' "$TARGET_FILE"; then
      sed -i -E "s|(TEST_SIGN[[:space:]]+)[0-9]+|\1$TEST_SIGN|g" "$TARGET_FILE"
    elif grep -qE 'TEST_ENC[[:space:]]+[0-9]+' "$TARGET_FILE"; then
      sed -i -E "s|(TEST_ENC[[:space:]]+)[0-9]+|\1$TEST_ENC|g" "$TARGET_FILE"
    fi

    # Try TEST_SIGN_OPEN first; if not found, try TEST_DEC
    if grep -qE 'TEST_SIGN_OPEN[[:space:]]+[0-9]+' "$TARGET_FILE"; then
      sed -i -E "s|(TEST_SIGN_OPEN[[:space:]]+)[0-9]+|\1$TEST_SIGNOPEN|g" "$TARGET_FILE"
    elif grep -qE 'TEST_DEC[[:space:]]+[0-9]+' "$TARGET_FILE"; then
      sed -i -E "s|(TEST_DEC[[:space:]]+)[0-9]+|\1$TEST_DEC|g" "$TARGET_FILE"
    fi

    echo "    Backup created at '${TARGET_FILE}.bak'"
  else
    echo "==> Skipping main.c macro modifications (flag: --no-modify)"
  fi

  echo "==> Running build..."
  make app PROJECT="${APP_FOLDER}/${APP_NAME}" LINKER=on_chip TARGET=zcu104

  echo "==> Copying compiled output..."
  SECOND_DST="$SECOND_DST_PARENT/${APP_NAME}${SUFFIX}"
  mkdir -p "$SECOND_DST"
  copy_dir "$SECOND_SRC_FOLDER" "$SECOND_DST"

  echo "✅ Done for ${APP_NAME}"
  echo

done

echo "All requested apps processed."