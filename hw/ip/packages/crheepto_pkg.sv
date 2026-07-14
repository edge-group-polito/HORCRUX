// Copyright 2024 Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: crheepto_pkg.sv
// Author: Luigi Giuffrida, Valeria Piscopo
// Date: 16/10/2024
// Description: crheepto pkg

package crheepto_pkg;

  import addr_map_rule_pkg::*;
  import core_v_mini_mcu_pkg::*;

  // ---------------
  // CORE-V-MINI-MCU
  // ---------------

  // CPU
  localparam int unsigned CpuCorevPulp = 32'd1;
  localparam int unsigned CpuCorevXif = 32'd1;
  localparam int unsigned CpuFpu = 32'd1;
  localparam logic CpuRiscvZfinx = 1'd0;

  // SPC
  localparam int unsigned AoSPCNum = 32'd0;

  localparam int unsigned DMAMasterPortsNum = DMA_NUM_MASTER_PORTS;
  localparam int unsigned DMACHNum = DMA_CH_NUM;

  // --------------------
  // CV-X-IF COPROCESSORS
  // --------------------



  // ----------------
  // EXTERNAL OBI BUS
  // ----------------

  // Number of masters and slaves
  localparam int unsigned ExtXbarNMaster = 32'd0;
  localparam int unsigned ExtXbarNSlave = 32'd0;
  localparam int unsigned ExtXbarNMasterRnd = ExtXbarNMaster > 0 ? ExtXbarNMaster : 32'd1;
  localparam int unsigned ExtXbarNSlaveRnd = ExtXbarNSlave > 0 ? ExtXbarNSlave : 32'd1;
  localparam int unsigned LogExtXbarNMaster = ExtXbarNMaster > 32'd1 ? $clog2(
      ExtXbarNMaster
  ) : 32'd1;
  localparam int unsigned LogExtXbarNSlave = ExtXbarNSlave > 32'd1 ? $clog2(ExtXbarNSlave) : 32'd1;
  localparam int unsigned ExtSlaveDefaultIdx = 32'd0;

  // Dummy default address map when no external slaves are present
  localparam addr_map_rule_t [ExtXbarNSlaveRnd-1:0] ExtSlaveAddrRules = '{
      '{idx: 32'd0, start_addr: 32'h0000_0000, end_addr: 32'h0000_1000}
  };

  // --------------------
  // EXTERNAL PERIPHERALS
  // --------------------

  // Number of external peripherals
  localparam int unsigned ExtPeriphNSlave = 32'd0;
  localparam int unsigned LogExtPeriphNSlave = (ExtPeriphNSlave > 32'd1) ? $clog2(
      ExtPeriphNSlave
  ) : 32'd1;
  localparam int unsigned ExtPeriphNSlaveRnd = (ExtPeriphNSlave > 32'd1) ? ExtPeriphNSlave : 32'd1;

  // Dummy default peripheral address map when no external peripherals are present
  localparam addr_map_rule_t [ExtPeriphNSlaveRnd-1:0] ExtPeriphAddrRules = '{
      '{idx: 32'd0, start_addr: 32'h0000_0000, end_addr: 32'h0000_1000}
  };

  localparam int unsigned ExtPeriphDefaultIdx = 32'd0;

  localparam int unsigned ExtInterrupts = 32'd0;

endpackage

