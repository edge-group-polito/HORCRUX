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
  localparam int unsigned CpuCorevPulp = 32'd${cpu_corev_pulp};
  localparam int unsigned CpuCorevXif = 32'd${cpu_corev_xif};
  localparam int unsigned CpuFpu = 32'd${cpu_fpu};
  localparam logic CpuRiscvZfinx = 1'd${cpu_riscv_zfinx};

  // SPC
  localparam int unsigned AoSPCNum = 32'd${ao_spc_num};

  localparam int unsigned DMAMasterPortsNum = DMA_NUM_MASTER_PORTS;
  localparam int unsigned DMACHNum = DMA_CH_NUM;

  // --------------------
  // CV-X-IF COPROCESSORS
  // --------------------

  

  // ----------------
  // EXTERNAL OBI BUS
  // ----------------

  // Number of masters and slaves
  localparam int unsigned ExtXbarNMaster = 32'd${xbar_nmasters};
  localparam int unsigned ExtXbarNSlave = 32'd${xbar_nslaves};
  localparam int unsigned ExtXbarNMasterRnd = ExtXbarNMaster > 0 ? ExtXbarNMaster : 32'd1;
  localparam int unsigned ExtXbarNSlaveRnd = ExtXbarNSlave > 0 ? ExtXbarNSlave : 32'd1; 
  localparam int unsigned LogExtXbarNMaster = ExtXbarNMaster > 32'd1 ? $clog2(ExtXbarNMaster) : 32'd1;
  localparam int unsigned LogExtXbarNSlave = ExtXbarNSlave > 32'd1 ? $clog2(ExtXbarNSlave) : 32'd1;
  localparam int unsigned ExtSlaveDefaultIdx = 32'd0;
  
% if (xbar_nslaves > 0):

% for a_slave in slaves:

    // Memory map
    // ----------
    // ${a_slave['name']}
    localparam int unsigned ${a_slave['name']}Idx = 32'd${a_slave['idx']};
    localparam logic [31:0] ${a_slave['name']}StartAddr = EXT_SLAVE_START_ADDRESS + 32'h${a_slave['offset']};
    localparam logic [31:0] ${a_slave['name']}Size = 32'h${a_slave['size']};
    localparam logic [31:0] ${a_slave['name']}EndAddr = ${a_slave['name']}StartAddr + 32'h${a_slave['size']};
% endfor

    // External slaves address map
    localparam addr_map_rule_t [ExtXbarNSlave-1:0] ExtSlaveAddrRules = '{
% for slave_idx, a_slave in enumerate(slaves):
% if (slave_idx < len(slaves)-1):
      '{idx: ${a_slave['name']}Idx, start_addr: ${a_slave['name']}StartAddr, end_addr: ${a_slave['name']}EndAddr},
% else:
      '{idx: ${a_slave['name']}Idx, start_addr: ${a_slave['name']}StartAddr, end_addr: ${a_slave['name']}EndAddr}
%endif
%endfor
    };
% else:
  // Dummy default address map when no external slaves are present
  localparam addr_map_rule_t [ExtXbarNSlaveRnd-1:0] ExtSlaveAddrRules = '{
    '{idx: 32'd0, start_addr: 32'h0000_0000, end_addr: 32'h0000_1000}
  };
% endif
  
  // --------------------
  // EXTERNAL PERIPHERALS
  // --------------------

  // Number of external peripherals
  localparam int unsigned ExtPeriphNSlave = 32'd${periph_nslaves};
  localparam int unsigned LogExtPeriphNSlave = (ExtPeriphNSlave > 32'd1) ? $clog2(ExtPeriphNSlave) : 32'd1;
  localparam int unsigned ExtPeriphNSlaveRnd = (ExtPeriphNSlave > 32'd1) ? ExtPeriphNSlave : 32'd1;

% if (periph_nslaves > 0):

% for a_slave in peripherals:

    // Memory map
    // ----------
    // ${a_slave['name']}
    localparam int unsigned ${a_slave['name']}PeriphIdx = 32'd${a_slave['idx']};
    localparam logic [31:0] ${a_slave['name']}PeriphStartAddr = EXT_PERIPHERAL_START_ADDRESS + 32'h${a_slave['offset']};
    localparam logic [31:0] ${a_slave['name']}PeriphSize = 32'h${a_slave['size']};
    localparam logic [31:0] ${a_slave['name']}PeriphEndAddr = ${a_slave['name']}PeriphStartAddr + 32'h${a_slave['size']};
% endfor
    
    // External peripherals address map
    localparam addr_map_rule_t [ExtPeriphNSlave-1:0] ExtPeriphAddrRules = '{
% for slave_idx, a_slave in enumerate(peripherals):
% if (slave_idx < len(peripherals)-1):
      '{idx: ${a_slave['name']}PeriphIdx, start_addr: ${a_slave['name']}PeriphStartAddr, end_addr: ${a_slave['name']}PeriphEndAddr},
% else:
      '{idx: ${a_slave['name']}PeriphIdx, start_addr: ${a_slave['name']}PeriphStartAddr, end_addr: ${a_slave['name']}PeriphEndAddr}
% endif
% endfor
    };

% else:
  // Dummy default peripheral address map when no external peripherals are present
  localparam addr_map_rule_t [ExtPeriphNSlaveRnd-1:0] ExtPeriphAddrRules = '{
    '{idx: 32'd0, start_addr: 32'h0000_0000, end_addr: 32'h0000_1000}
  };
% endif

  localparam int unsigned ExtPeriphDefaultIdx = 32'd0;

  localparam int unsigned ExtInterrupts = 32'd${ext_interrupts};

endpackage

