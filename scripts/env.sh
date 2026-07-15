##########################################################################################
#
# Copyright 2025 PoliTO - EDGE Group, @VLSI Lab
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it
#               Valeria Piscopo    - valeria.piscopo@polito.it
# Design Name:  X-HEEP Toolchain Environment Setup
# Language:     Bash script (sourced)
# Date:         April 2026
#
# Description:  Sourced helper that adds the RISC-V toolchain, Verilator, Verible, and
#               OpenOCD to PATH for building and simulating crheepto/HORCRUX.
#
##########################################################################################


# Make the whole script a function so it does not pollute the calling
# environment when sourced.
function init_x_heep() {
    # Set paths
    # ---------
    # Tool directories
    local HEEP_TOOL_PATH=/software/x-heep
    local VERILATOR_PATH=$HEEP_TOOL_PATH/verilator/4.210
    # local VERIBLE_PATH=$HEEP_TOOL_PATH/verible/v0.0-2135-gb534c1fe
    local VERIBLE_PATH=$HEEP_TOOL_PATH/verible/v0.0-3253-gf85c768c
    local OPENOCD_PATH=$HEEP_TOOL_PATH/openocd/v0.11.0-rc2

    local RISCV_PATH=/software/riscv/riscv32-corev

    # Set environment variables
    # -------------------------
    echo "Configuring X-HEEP environment..."

    # Add x-heep RISC-V toolchain to PATH
    echo " - RISC-V toolchain"
    if [ -d $RISCV_PATH/bin ]; then
        export RISCV=$RISCV_PATH
        [[ ! "$PATH" =~ .*$RISCV_PATH/bin.* ]] && export PATH=$RISCV_PATH/bin:$PATH
    else
        echo "ERROR: '$RISCV_PATH/bin' does not exist" >&2
    fi

    # Add Verilator to PATH
    echo " - Verilator"
    if [ -d $VERILATOR_PATH/bin ]; then
        [[ ! "$PATH" =~ .*$VERILATOR_PATH/bin.* ]] && export PATH=$VERILATOR_PATH/bin:$PATH
    else
        echo "ERROR: '$VERILATOR_PATH/bin' does not exist" >&2
    fi

    # Add Verible to PATH
    echo " - Verible"
    if [ -d $VERIBLE_PATH/bin ]; then
        [[ ! $PATH =~ .*$VERIBLE_PATH/bin.* ]] && export PATH=$VERIBLE_PATH/bin:$PATH
    else
        echo "ERROR: '$VERIBLE_PATH/bin' does not exist" >&2
    fi

    # Add OpenOCD to PATH
    echo " - OpenOCD"
    if [ -d $OPENOCD_PATH/bin ]; then
        [[ ! $PATH =~ .*$OPENOCD_PATH/bin.* ]] && export PATH=$OPENOCD_PATH/bin:$PATH
    else
        echo "ERROR: '$OPENOCD_PATH/bin' does not exist" >&2
    fi

    echo " - "

}

# Set X-HEEP environment variables
init_x_heep "$@"

source /eda/scripts/init_vivado_2022.2
source /eda/scripts/init_questa_2020.4
source /eda/scripts/init_design_vision
source /eda/scripts/init_cadence_23_24
source /eda/scripts/init_calibre

# Initialize conda environment
echo

echo "Activating 'core-v-mini-mcu' conda environment..."
conda activate core-v-mini-mcu
