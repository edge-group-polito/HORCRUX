# Copyright 2025 Politecnico di Torino.
# Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
# SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
#
# File: makefile
# Author: Luigi Giuffrida, Valeria Piscopo, Alessandra Dolmeta
# Date: 09/01/2026
# Description: Top-level makefile for crheepto

# ----- CONFIGURATION ----- #

# Global configuration
ROOT_DIR			:= $(realpath .)
BUILD_DIR 			:= build
SW_BUILD_DIR		:= sw/build

# FUSESOC and Python values (default)
ifndef CONDA_DEFAULT_ENV
$(info USING VENV)
FUSESOC = ./.venv/bin/fusesoc
PYTHON  = ./.venv/bin/python
else
$(info USING MINICONDA $(CONDA_DEFAULT_ENV))
FUSESOC := $(shell which fusesoc)
PYTHON  := $(shell which python)
endif
FUSESOC_BUILD_DIR			= $(shell find $(BUILD_DIR) -type d -name 'polito_vlsi_crheepto_*' 2>/dev/null | sort | head -n 1)

# X-HEEP configuration
XHEEP_DIR			:= $(ROOT_DIR)/hw/vendor/x-heep
XHEEP_CACHE         := $(ROOT_DIR)/$(BUILD_DIR)/xheep_cache.pickle
LINK_FOLDER			:= $(XHEEP_DIR)/sw/linker
X_HEEP_CFG  		?= $(ROOT_DIR)/config/mcu-gen.hjson
PADS_CFG			?= $(ROOT_DIR)/config/heep-pads.hjson
EXTERNAL_DOMAINS	:= 
MCU_GEN_OPTS		:= \
	--cached_path $(XHEEP_CACHE) \
	--cached 
MCU_GEN_LOCK			:= $(BUILD_DIR)/.mcu-gen.lock
crheepto_LOCK			:= $(BUILD_DIR)/.crheepto.lock
crheepto_GEN_LOCK := build/.crheepto-gen.lock

# crheepto configuration
crheepto_GEN_CFG	 := config/crheepto-cfg.hjson
crheepto_GEN_OPTS := \
	--cfg $(crheepto_GEN_CFG)

crheepto_PKG_TPL		:= $(ROOT_DIR)/hw/ip/packages/crheepto_pkg.sv.tpl
crheepto_TOP_TPL		:= $(ROOT_DIR)/hw/ip/crheepto_top.sv.tpl
crheepto_PERIPH_TPL		:= $(ROOT_DIR)/hw/ip/peripherals/crheepto_peripherals.sv.tpl
PAD_RING_TPL			:= $(ROOT_DIR)/hw/ip/pad-ring/pad_ring.sv.tpl
crheepto_H    			:= $(ROOT_DIR)/sw/external/lib/runtime/crheepto.h.tpl
crheepto_PKG    		:= $(ROOT_DIR)/hw/ip/packages/crheepto_pkg.sv.tpl


# Simulation DPI libraries
DPI_LIBS			:= $(BUILD_DIR)/sw/sim/uartdpi.so
DPI_CINC			:= -I$(dir $(shell which verilator))../share/verilator/include/vltstd

# Simulation configuration
LOG_LEVEL			?= LOG_LOW
BOOT_MODE			?= flash # jtag: wait for JTAG (DPI module), flash (default): boot from flash, force: load firmware into SRAM
FIRMWARE			?= $(ROOT_DIR)/build/sw/app/main.hex
LINKER 				?= flash_load
VCD_MODE			?= 0 # QuestaSim-only - 0: no dump, 1: dump always active, 2: dump triggered by GPIO 0
MAX_CYCLES			?= 500000000000000000000000000000000000000000000000000000000
FUSESOC_FLAGS		?=
FUSESOC_ARGS		?=

# Flash file
FLASHWRITE_FILE		?= $(FIRMWARE)

# QuestaSim
QUESTA_SIM_DIR						= $(FUSESOC_BUILD_DIR)/sim-modelsim
QUESTA_SIM_xxxlib_DIR				= $(FUSESOC_BUILD_DIR)/sim_rtl_xxxlib-modelsim
QUESTA_SIM_POSTSYNTH_DIR 			= $(FUSESOC_BUILD_DIR)/sim_postsynthesis-modelsim
QUESTA_SIM_POSTLAYOUT_DIR 			= $(FUSESOC_BUILD_DIR)/sim_postlayout-modelsim
QUESTA_SIM_POSTLAYOUT_TIMING_DIR 	= $(FUSESOC_BUILD_DIR)/sim_postlayout_timing-modelsim

# Power Analysis
PWR_VCD ?= $(QUESTA_SIM_POSTSYNTH_DIR)/logs/waves-0.vcd

# Custom preprocessor definitions
CDEFS				?=

# Software build configuration
SW_DIR		:= sw
LINK_FOLDER := $(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))")/sw/linker

# Dummy target to force software rebuild
PARAMS = $(PROJECT)



# Application spacific makefile
APP_MAKE 	:= $(wildcard sw/applications/$(PROJECT)/*akefile)
APP_PARAMS  ?=

# Arch for the software build
# ARCH ?= rv32imfdc_zicsr
# ARCH ?= rv32imac_zicsr_zba_zbb_zbc

ARCH ?= rv32imfdc_zicsr_xcvbitmanip
# Note: CV32E40P uses custom CORE-V extensions (xcv*), not standard RISC-V B extensions (zb*)
# Available custom extensions:
#   xcvbitmanip - Bit manipulation (replaces zba, zbb)
#   xcvalu - ALU extensions
#   xcvmac - Multiply-Accumulate
#   xcvsimd - SIMD operations
#   xcvmem - Post-increment load/store
#   xcvhwlp - Hardware loops
#   xcvelw - Event load
#rv32imc_zicsr

# Compiler prefix for the software build
# Note: The build system automatically appends '-elf-', so don't include it here
COMPILER_PREFIX ?= riscv32-corev-

# Waves
SIM_VCD 			?= $(BUILD_DIR)/polito_vlsi_crheepto_0/sim_postsynthesis-modelsim/logs/questa-waves.fst

# Artefacts
GDS ?= ""

# ----- BUILD RULES ----- #

# Get the path of this Makefile to pass to the Makefile help generator
MKFILE_PATH = $(shell dirname "$(realpath $(firstword $(MAKEFILE_LIST)))")
export FILE_FOR_HELP = $(MKFILE_PATH)/makefile
export XHEEP_DIR


## Print the Makefile help
## @param WHICH=xheep,all,<none> Which Makefile help to print. Leaving blank (<none>) prints only polheppo's.
help:
ifndef WHICH
	${XHEEP_DIR}/util/MakefileHelp
else ifeq ($(filter $(WHICH),xheep x-heep),)
	${XHEEP_DIR}/util/MakefileHelp
	$(MAKE) -C $(XHEEP_DIR) help
else
	$(MAKE) -C $(XHEEP_DIR) help
endif

# Default alias
# -------------
.PHONY: all
all: crheepto-gen

## @section RTL & SW generation

## X-HEEP MCU system
.PHONY: mcu-gen
mcu-gen: $(MCU_GEN_LOCK)
$(MCU_GEN_LOCK): $(MCU_CFG) $(PADS_CFG) $(X_HEEP_CFG) | $(BUILD_DIR)/ 
	@echo "### Building X-HEEP MCU..."
	$(MAKE) -f $(XHEEP_MAKE) mcu-gen LINK_FOLDER=$(LINK_FOLDER)
	touch $@
	$(RM) -f $(crheepto_GEN_LOCK)
	@echo "### DONE! X-HEEP MCU generated successfully"

.PHONY: crheepto-gen-force
crheepto-gen-force:
	rm -rf build/.mcu-gen.lock build/.crheepto-gen.lock;
	$(MAKE) crheepto-gen

## Generate crheepto files
## @param TARGET=asic(default),pynq-z2,zcu104
.PHONY: crheepto-gen
crheepto-gen: $(crheepto_GEN_LOCK)
$(crheepto_GEN_LOCK): $(crheepto_GEN_CFG) $(crheepto_GEN_TPL) $(crheepto_TOP_TPL) $(crheepto_PERIPH_TPL) $(PAD_RING_TPL) $(MCU_GEN_LOCK) $(ROOT_DIR)/tb/tb_util.svh.tpl $(crheepto_PKG) $(crheepto_H)
	@echo "### Generating crheepto top and pad rings for ASIC..."
	python3 util/crheepto_mcu_gen.py $(MCU_GEN_OPTS) \
		--outtpl $(crheepto_TOP_TPL)
	python3 util/crheepto_gen.py $(crheepto_GEN_OPTS) \
		--outdir hw/ip/packages \
		--tpl-sv $(crheepto_PKG_TPL)
	python3 util/crheepto_gen.py $(crheepto_GEN_OPTS) \
		--outdir hw/ip/peripherals \
		--tpl-sv $(crheepto_PERIPH_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS) \
		--outtpl $(PAD_RING_TPL)
	python3 $(XHEEP_DIR)/util/mcu_gen.py $(MCU_GEN_OPTS) \
		--outtpl $(ROOT_DIR)/tb/tb_util.svh.tpl
	@echo "### Generating crheepto files..."
	python3 util/crheepto_gen.py $(crheepto_GEN_OPTS) \
		--outdir sw/external/lib/runtime \
		--tpl-c $(crheepto_H)
	python3 util/crheepto_gen.py $(crheepto_GEN_OPTS) \
		--outdir hw/ip/packages \
		--tpl-sv $(crheepto_PKG) \
		--corev_pulp $(COREV_PULP)
	$(FUSESOC) run --no-export --target format polito:vlsi:crheepto
	$(FUSESOC) run --no-export --target lint polito:vlsi:crheepto
	@echo "### DONE! crheepto files generated successfully"
	touch $@

## @section Synthesis

## crheepto synthesis with Synopsys DC Shell
.PHONY: synthesis
synthesis: $(crheepto_GEN_LOCK)
	$(FUSESOC) run --no-export --target asic_synthesis --build $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		$(FUSESOC_ARGS) 2>&1 | tee fusesoc_synthesis.log
	./scripts/check_log_synth.sh

## @section Place and Route

## PnR debug (loads variables but not a design)
.PHONY: pnr_debug
pnr_debug:
	pushd implementation/pnr/ ; ./run_pnr_flow.csh debug; popd;

## PnR only
.PHONY: pnr
pnr:
	pushd implementation/pnr/ ; ./run_pnr_flow.csh; popd;

## Reopen existing PnR build directory (GUI or batch) without creating a new one
## Usage:
##   make pnr_reopen                        # reuse build/innovus_latest
##   make pnr_reopen PNR_BUILD=path/to/dir  # reuse specified dir
##   make pnr_reopen_debug                  # reuse build/innovus_latest in debug
##   make pnr_reopen_debug PNR_BUILD=path/to/dir
## Notes:
##   - PNR_BUILD may be relative; it will be converted to an absolute path.
##   - The old BUILD variable is no longer honored here.
.PHONY: pnr_reopen pnr_reopen_debug
pnr_reopen:
	@PNR_DIR="$(strip $(PNR_BUILD))"; \
	if [ -z "$$PNR_DIR" ]; then PNR_DIR="build/innovus_latest"; fi; \
	if [ ! -d "$$PNR_DIR" ]; then \
		echo "### ERROR: PnR build dir '$$PNR_DIR' not found. Run 'make pnr' or set PNR_BUILD=<dir>." >&2; exit 1; fi; \
	ABS_PNR_DIR=$$(realpath "$$PNR_DIR"); \
	echo "### Reopening PnR build: $$ABS_PNR_DIR"; \
	pushd implementation/pnr/ > /dev/null; ./run_pnr_flow.csh "$$ABS_PNR_DIR"; RC=$$?; popd > /dev/null; exit $$RC

pnr_reopen_debug:
	@PNR_DIR="$(strip $(PNR_BUILD))"; \
	if [ -z "$$PNR_DIR" ]; then PNR_DIR="build/innovus_latest"; fi; \
	if [ ! -d "$$PNR_DIR" ]; then \
	echo "### ERROR: PnR build dir '$$PNR_DIR' not found. Run 'make pnr' or set PNR_BUILD=<dir>." >&2; exit 1; fi; \
	ABS_PNR_DIR=$$(realpath "$$PNR_DIR"); \
	echo "### Reopening PnR build (debug): $$ABS_PNR_DIR"; \
	pushd implementation/pnr/ > /dev/null; ./run_pnr_flow.csh "$$ABS_PNR_DIR" debug; RC=$$?; popd > /dev/null; exit $$RC

## Export PnR to common location (has to be created)
.PHONY: export_pnr
export_pnr:
	cp -r build/innovus_latest/artefacts/export/ /shared/crheepto/pnr/artefacts/

## Fill metal and poly before DRC
.PHONY: dummyfill
dummyfill:
	pushd implementation/dummyfill ; make clean; make GDS_GZ=$(realpath $(GDS)); popd;

## Generate the logo GDSII
.PHONY: logo
logo: util/svg_2_gds.py logo/crheepto.svg
	python3 $^ logo/logo.gds

## Open Calibre for GDS viewing
.PHONY: calibre_gds
calibre_gds:
ifeq ($(strip $(GDS)),)
# 	pushd implementation/drc/ ; calibredrv -endDepth 200 ; popd;
	pushd implementation/drc/ ; calibredrv -dl layerprops -endDepth 200 ; popd;
else
	@echo "Loading... $(realpath $(GDS))"
# 	pushd implementation/drc/ ; calibredrv $(realpath $(GDS)) -endDepth 200 ; popd;
	pushd implementation/drc/ ; calibredrv $(realpath $(GDS)) -dl layerprops -endDepth 200 ; popd;
endif

## Open Calibre for GDS viewing of the dummyfilled GDS
.PHONY: calibre_dummyfilled
calibre_dummyfilled:
	@echo "Loading... $(realpath implementation/dummyfill/crheepto_filled.gds.gz)"
	pushd implementation/drc/ ; calibredrv $(realpath implementation/dummyfill/crheepto_filled.gds.gz) -endDepth 200 -threads 16 -dl layerprops ; popd;


## Open Calibre LVS
.PHONY: calibre_lvs
calibre_lvs:
	pushd implementation/lvs/ ; $(MAKE) run_lvs ; popd;

## Open Calibre LVS
.PHONY: calibre_lvs_report
calibre_lvs_report:
	pushd implementation/lvs/ ; $(MAKE) show_report ; popd;

# Launch Innovus GUI
.PHONY: launch-ivs
launch-ivs: .check-innovus
	innovus -common_ui -execute "gui_show"
# Check tools
.PHONY: .check-innovus
.check-innovus:
	@if [ `which innovus &> /dev/null` ]; then \
	printf -- "### ERROR: 'innovus' is not in PATH.\n" >&2; \
	exit 1; fi

## @section Simulation

## @subsection Verilator RTL simulation

## Build simulation model (do not launch simulation)
.PHONY: verilator-build
verilator-build: $(crheepto_GEN_LOCK)
	$(FUSESOC) run --no-export --target sim --tool verilator --build $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		$(FUSESOC_ARGS)

## Build simulation model and launch simulation
.PHONY: verilator-sim
verilator-sim: | check-firmware verilator-build .verilator-check-params
	$(FUSESOC) run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--log_level=$(LOG_LEVEL) \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		--trace=true \
		$(FUSESOC_ARGS)
	cat $(FUSESOC_BUILD_DIR)/sim-verilator/uart.log

## Launch simulation
.PHONY: verilator-run
verilator-run: | check-firmware .verilator-check-params
	$(FUSESOC) run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--log_level=$(LOG_LEVEL) \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		--trace=true \
		--no_err \
		$(FUSESOC_ARGS)
	cat $(FUSESOC_BUILD_DIR)/sim-verilator/uart.log

## Launch simulation without waveform dumping
.PHONY: verilator-opt
verilator-opt: | check-firmware .verilator-check-params
	$(FUSESOC) run --no-export --target sim --tool verilator --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--log_level=$(LOG_LEVEL) \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		--trace=false \
		$(FUSESOC_ARGS)
	cat $(FUSESOC_BUILD_DIR)/sim-verilator/uart.log

# Open dumped waveform with GTKWave
.PHONY: verilator-waves
verilator-waves: $(BUILD_DIR)/sim-common/waves.fst | .check-gtkwave
	gtkwave -a tb/misc/verilator-waves.gtkw $<

## @subsection QuestaSim RTL simulation

## Build simulation model
.PHONY: questasim-build
questasim-build: $(crheepto_GEN_LOCK) $(DPI_LIBS)
	$(FUSESOC) run --no-export --target sim --tool modelsim --build $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		$(FUSESOC_ARGS)
	cd $(QUESTA_SIM_DIR) ; make opt

## Build simulation model and launch simulation
.PHONY: questasim-sim
questasim-sim: | check-firmware questasim-build $(QUESTA_SIM_DIR)/logs/
	$(FUSESOC) run --no-export --target sim --tool modelsim --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(QUESTA_SIM_DIR)/uart.log

## Launch simulation
.PHONY: questasim-run
questasim-run: | check-firmware $(QUESTA_SIM_DIR)/logs/
	$(FUSESOC) run --no-export --target sim --tool modelsim --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		$(FUSESOC_ARGS)
	cat $(QUESTA_SIM_DIR)/uart.log

## Launch simulation in GUI mode
.PHONY: questasim-gui
questasim-gui: | check-firmware $(QUESTA_SIM_DIR)/logs/
	$(MAKE) -C $(QUESTA_SIM_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE)"

## Open dumped waveforms in GTKWave
.PHONY: questasim-waves
questasim-waves: $(SIM_VCD) | .check-gtkwave
	gtkwave -a tb/misc/questasim-waves.gtkw $<

$(BUILD_DIR)/polito_vlsi_crheepto_0/sim_postsynthesis-modelsim/logs/questa-waves.fst: $(BUILD_DIR)/polito_vlsi_crheepto_0/sim_postsynthesis-modelsim/logs/waves-0.vcd | .check-gtkwave
	@echo "### Converting $< to FST..."
	vcd2fst $< $@

## DPI libraries for QuestaSim
.PHONY: tb-dpi
tb-dpi: $(DPI_LIBS)
$(BUILD_DIR)/sw/sim/uartdpi.so: hw/vendor/x-heep/hw/vendor/lowrisc_opentitan/hw/dv/dpi/uartdpi/uartdpi.c | $(BUILD_DIR)/sw/sim/
	$(CC) -shared -Bsymbolic -fPIC -o $@ $< -lutil

## @subsection QuestaSim RTL simulation with xxxlib cells

## Build simulation model
.PHONY: questasim-build-xxxlib
questasim-build-xxxlib: $(crheepto_GEN_LOCK) $(DPI_LIBS)
	$(FUSESOC) run --no-export --target sim_rtl_xxxlib --tool modelsim --build $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		$(FUSESOC_ARGS)
	cd $(QUESTA_SIM_xxxlib_DIR) ; make opt

## Build simulation model and launch simulation
.PHONY: questasim-sim-xxxlib
questasim-sim-xxxlib: | check-firmware questasim-build-xxxlib $(QUESTA_SIM_xxxlib_DIR)/logs/
	$(FUSESOC) run --no-export --target sim_rtl_xxxlib --tool modelsim --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(QUESTA_SIM_xxxlib_DIR)/uart.log

## Launch simulation
.PHONY: questasim-run-xxxlib
questasim-run-xxxlib: | check-firmware $(QUESTA_SIM_xxxlib_DIR)/logs/
	$(FUSESOC) run --no-export --target sim_rtl_xxxlib --tool modelsim --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(QUESTA_SIM_xxxlib_DIR)/uart.log

## @subsection QuestaSim postsynthesis simulation

## Build simulation model
.PHONY: questasim-build-postsynth
questasim-build-postsynth: $(crheepto_GEN_LOCK) $(DPI_LIBS)
	$(FUSESOC) run --no-export --target sim_postsynthesis --tool modelsim --build $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		$(FUSESOC_ARGS)
	cd $(QUESTA_SIM_POSTSYNTH_DIR) ; make opt

## Build simulation model and launch simulation
.PHONY: questasim-sim-postsynth
questasim-sim-postsynth: | check-firmware questasim-build-postsynth $(QUESTA_SIM_POSTSYNTH_DIR)/logs/
	$(FUSESOC) run --no-export --target sim_postsynthesis --tool modelsim --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(QUESTA_SIM_POSTSYNTH_DIR)/uart.log

## Launch simulation
.PHONY: questasim-run-postsynth
questasim-run-postsynth: | check-firmware $(QUESTA_SIM_POSTSYNTH_DIR)/logs/
	$(FUSESOC) run --no-export --target sim_postsynthesis --tool modelsim --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(QUESTA_SIM_POSTSYNTH_DIR)/uart.log

## Launch simulation in GUI mode
.PHONY: questasim-gui-postsynth
questasim-gui-postsynth: | check-firmware $(QUESTA_SIM_POSTSYNTH_DIR)/logs/
	$(MAKE) -C $(QUESTA_SIM_POSTSYNTH_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE)"

## @subsection QuestaSim postlayout simulation

## Build simulation model
.PHONY: questasim-build-postlayout
questasim-build-postlayout: $(crheepto_GEN_LOCK) $(DPI_LIBS)
	$(FUSESOC) run --no-export --target sim_postlayout --tool modelsim --build $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		$(FUSESOC_ARGS)
	cd $(QUESTA_SIM_POSTLAYOUT_DIR) ; make opt

## Build simulation model and launch simulation
.PHONY: questasim-sim-postlayout
questasim-sim-postlayout: | check-firmware questasim-build-postlayout $(QUESTA_SIM_POSTLAYOUT_DIR)/logs/
	$(FUSESOC) run --no-export --target sim_postlayout --tool modelsim --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(QUESTA_SIM_POSTLAYOUT_DIR)/uart.log

## Launch simulation
.PHONY: questasim-run-postlayout
questasim-run-postlayout: | check-firmware $(QUESTA_SIM_POSTLAYOUT_DIR)/logs/
	$(FUSESOC) run --no-export --target sim_postlayout --tool modelsim --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		--max_cycles=$(MAX_CYCLES) \
		$(FUSESOC_ARGS)
	cat $(QUESTA_SIM_POSTLAYOUT_DIR)/uart.log

## Launch simulation in GUI mode
.PHONY: questasim-gui-postlayout
questasim-gui-postlayout: | check-firmware $(QUESTA_SIM_POSTLAYOUT_DIR)/logs/
	$(MAKE) -C $(QUESTA_SIM_POSTLAYOUT_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE)"

## Questasim post-layout simulation with timing
.PHONY: questasim-build-postlayout-timing
questasim-build-postlayout-timing: $(DPI_LIBS)
	$(FUSESOC) run --no-export --target sim_postlayout_timing --tool modelsim --build $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		$(FUSESOC_ARGS);
	cd $(QUESTA_SIM_POSTLAYOUT_TIMING_DIR) ; make opt | tee fusesoc_questasim_postlayout_timing.log

## Questasim post-layout run with timing
.PHONY: questasim-run-postlayout-timing
questasim-run-postlayout-timing: check-firmware
	$(FUSESOC) run --no-export --target sim_postlayout_timing --tool modelsim --run $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		--firmware=$(FIRMWARE) \
		--boot_mode=$(BOOT_MODE) \
		--vcd_mode=$(VCD_MODE) \
		$(FUSESOC_ARGS)
	cat $(BUILD_DIR)/sim-common/uart.log

## Launch post-layout simulation in GUI mode with timing
.PHONY: questasim-gui-postlayout-timing
questasim-gui-postlayout-timing: check-firmware
	$(MAKE) -C $(QUESTA_SIM_POSTLAYOUT_TIMING_DIR) run-gui RUN_OPT=1 PLUSARGS="firmware=$(FIRMWARE) boot_mode=$(BOOT_MODE) vcd_mode=$(VCD_MODE)"

## Perform power analysis
.PHONY: power-analysis
power-analysis:
	@echo "### Running power analysis..."
	rm -rf implementation/power_analysis/reports/*
	pushd implementation/power_analysis/; ./run_pwr_flow.sh $(PWR_VCD) crheepto_top; popd;

## @section Software

## crheepto applications
.PHONY: app
app: $(crheepto_GEN_LOCK) | $(BUILD_DIR)/sw/app/
	@echo "### Build Configuration:"
	@echo "###   ARCH: $(ARCH)"
	@echo "###   COMPILER_PREFIX: $(COMPILER_PREFIX)"
	@echo "###   PROJECT: $(PROJECT)"
ifneq ($(APP_MAKE),)
	@echo "### Calling application-specific makefile '$(APP_MAKE)'..."
	$(MAKE) -C $(dir $(APP_MAKE)) APP_PARAMS="$(APP_PARAMS)" ARCH="$(ARCH)"
endif
	@echo "### Building application for SRAM execution with GCC compiler..."
	CDEFS=$(CDEFS) $(MAKE) -f $(XHEEP_MAKE) $(MAKECMDGOALS) LINKER=$(LINKER) LINK_FOLDER=$(LINK_FOLDER) ARCH=$(ARCH) COMPILER_PREFIX=$(COMPILER_PREFIX) RISCV=$(RISCV) $(FUSESOC_FLAGS) $(FUSESOC_ARGS)
	find sw/build/ -maxdepth 1 -type f -name "main.*" -exec cp '{}' $(BUILD_DIR)/sw/app/ \;

## @section FPGA

## Synthesize crheepto for FPGA
## @param FPGA_BOARD=pynq-z2,zcu104
.PHONY: vivado-fpga-synth 
vivado-fpga-synth: $(crheepto_GEN_LOCK)
	$(FUSESOC) run --no-export --target $(FPGA_BOARD) --tool vivado --build $(FUSESOC_FLAGS) polito:vlsi:crheepto \
		$(FUSESOC_ARGS)

## Loads the generated bitstream into the FPGA
## @param FPGA_BOARD=pynq-z2,zcu104
.PHONY: vivado-fpga-pgm 
vivado-fpga-pgm:
	$(MAKE) -C build/polito_vlsi_crheepto_0/$(FPGA_BOARD)-vivado pgm

## @section Rogue

mage-gen:
	$(MAKE) -C $(ROOT_DIR)/hw/vendor/rogue $(MAKECMDGOALS)

## @section Utilities

## Check if the firmware is compiled
.PHONY: .check-firmware
check-firmware:
	@if [ ! -f $(FIRMWARE) ]; then \
		echo "\033[31mError: FIRMWARE has not been compiled! Simulation won't work!\033[0m"; \
		exit 1; \
	fi

## DPI libraries for QuestaSim
.PHONY: tb-dpi
tb-dpi: $(DPI_LIBS)
$(BUILD_DIR)/sw/sim/uartdpi.so: hw/vendor/x-heep/hw/vendor/lowrisc_opentitan/hw/dv/dpi/uartdpi/uartdpi.c | $(BUILD_DIR)/sw/sim/
	$(CC) -shared -Bsymbolic -fPIC -o $@ $< -lutil

## Update vendored IPs
.PHONY: vendor-update
vendor-update:
	@echo "### Updating vendored IPs..."
	find hw/vendor -maxdepth 1 -type f -name "*.vendor.hjson" -exec python3 util/vendor.py -vU '{}' \;
	$(MAKE) clean-lock

## Check if fusesoc is available
.PHONY: .check-fusesoc
.check-fusesoc:
	@if [ ! `which fusesoc` ]; then \
	printf -- "### ERROR: 'fusesoc' is not in PATH. Is the correct conda environment active?\n" >&2; \
	exit 1; fi

## Check if GTKWave is available
.PHONY: .check-gtkwave
.check-gtkwave:
	@if [ ! `which gtkwave` ]; then \
	printf -- "### ERROR: 'gtkwave' is not in PATH. Is the correct conda environment active?\n" >&2; \
	exit 1; fi

## Check simulation parameters
.PHONY: .verilator-check-params
.verilator-check-params:
	@if [ "$(BOOT_MODE)" = "flash" ]; then \
		echo "### ERROR: Verilator simulation with flash boot is not supported" >&2; \
		exit 1; \
	fi

## Create directories
%/:
	mkdir -p $@

## @section Cleaning

## Clean build directory
.PHONY: clean clean-lock
clean:
	$(RM) $(crheepto_GEN_LOCK)
	$(RM) hw/ip/packages/crheepto_pkg.sv
	$(RM) hw/ip/crheepto_top.sv
	$(RM) hw/ip/peripherals/crheepto_peripherals.sv
	$(RM) hw/ip/pad-ring/pad-ring.sv
	$(RM) hw/ip/poolheepo-ctrl/rtl/crheepto_ctrl_reg_top.sv
	$(RM) hw/ip/poolheepo-ctrl/rtl/crheepto_ctrl_reg_pkg.sv
	$(RM) hw/ip/poolheepo-ctrl/rtl/crheepto_ctrl_reg.sv
	$(RM) sw/device/include/crheepto.h
	$(RM) sw/device/include/crheepto_ctrl_reg.h
	$(RM) -r $(BUILD_DIR)
	$(MAKE) -C $(HEEP_DIR) clean-all
clean-lock:
	$(RM) $(BUILD_DIR)/.*.lock

## @section Format and Variables

## Verible format
.PHONY: format
format: $(crheepto_GEN_LOCK)
	@echo "### Formatting crheepto RTL files..."
	util/format-verible

.PHONY: lint
	@echo "### Linting crheepto RTL files..."
	$(FUSESOC) run --no-export --target lint polito:vlsi:crheepto


## Static analysis
.PHONY: lint
lint: $(crheepto_GEN_LOCK)
	@echo "### Checking crheepto syntax and code style..."
	$(FUSESOC) run --no-export --target lint polito:vlsi:crheepto

## Print variables
.PHONY: .print
.print:
	@echo "APP_MAKE: $(APP_MAKE)"
	@echo "KERNEL_PARAMS: $(KERNEL_PARAMS)"
	@echo "FUSESOC_ARGS: $(FUSESOC_ARGS)"


# ----- INCLUDE X-HEEP RULES ----- #
export X_HEEP_CFG
export XHEEP_CACHE
export PADS_CFG
export EXTERNAL_DOMAINS
export FLASHWRITE_FILE
export HEEP_DIR = $(ROOT_DIR)/hw/vendor/x-heep
XHEEP_MAKE 		= $(HEEP_DIR)/external.mk
# include $(XHEEP_MAKE)
