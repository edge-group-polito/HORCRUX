// Copyright 2024 Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: crheepto_top.sv
// Author: Luigi Giuffrida, Valeria Piscopo
// Date: 09/01/2026
// Description: crheepto top-level module

module crheepto_top (
    // X-HEEP interface
% for pad in total_pad_list:
${pad.x_heep_system_interface}
% endfor


);
  import obi_pkg::*;
  import reg_pkg::*;
  import crheepto_pkg::*;
  import core_v_mini_mcu_pkg::*;

  // PARAMETERS
  localparam int unsigned ExtXbarNmasterRnd = (crheepto_pkg::ExtXbarNMaster > 0) ?
    crheepto_pkg::ExtXbarNMaster : 32'd1;
  localparam int unsigned ExtDomainsRnd = core_v_mini_mcu_pkg::EXTERNAL_DOMAINS == 0 ?
    32'd1 : core_v_mini_mcu_pkg::EXTERNAL_DOMAINS;

  // INTERNAL SIGNALS
  // ----------------
  // Synchronized reset
  logic rst_nin_sync;

  // Exit value
  logic [31:0] exit_value;

  // X-HEEP external master ports
  obi_req_t  heep_core_instr_req;
  obi_resp_t heep_core_instr_rsp;
  obi_req_t  heep_core_data_req;
  obi_resp_t heep_core_data_rsp;
  obi_req_t  heep_debug_master_req;
  obi_resp_t heep_debug_master_rsp;
  obi_req_t  [DMA_NUM_MASTER_PORTS-1:0] heep_dma_read_req;
  obi_resp_t [DMA_NUM_MASTER_PORTS-1:0] heep_dma_read_rsp;
  obi_req_t  [DMA_NUM_MASTER_PORTS-1:0] heep_dma_write_req;
  obi_resp_t [DMA_NUM_MASTER_PORTS-1:0] heep_dma_write_rsp;
  obi_req_t  [DMA_NUM_MASTER_PORTS-1:0] heep_dma_addr_req;
  obi_resp_t [DMA_NUM_MASTER_PORTS-1:0] heep_dma_addr_rsp;

  /* verilator lint_off UNUSED */
  /* verilator lint_off UNDRIVEN */
  // TODO: Remove the verilator lint_off pragmas when the signals are used

  // X-HEEP slave ports
  obi_req_t  [ExtXbarNmasterRnd-1:0] heep_slave_req;
  obi_resp_t [ExtXbarNmasterRnd-1:0] heep_slave_rsp;

  // External master ports
  obi_req_t  [ExtXbarNmasterRnd-1:0] crheepto_master_req;
  obi_resp_t [ExtXbarNmasterRnd-1:0] crheepto_master_resp;

  // External slave ports
  obi_req_t  [ExtXbarNSlaveRnd-1:0] ext_slave_req;
  obi_resp_t [ExtXbarNSlaveRnd-1:0] ext_slave_resp;

  // X-HEEP external peripheral master ports
  reg_req_t heep_peripheral_req;
  reg_rsp_t heep_peripheral_rsp;

  // Interrupt vector
  logic [core_v_mini_mcu_pkg::NEXT_INT-1:0] ext_int_vector;

  // Power Manager signals
  logic cpu_subsystem_powergate_switch_n;
  logic cpu_subsystem_powergate_switch_ack_n;
  logic peripheral_subsystem_powergate_switch_n;
  logic peripheral_subsystem_powergate_switch_ack_n;

  // External SPC interface signals
  reg_req_t [AoSPCNum:0] ext_ao_peripheral_req;
  reg_rsp_t  [AoSPCNum:0] ext_ao_peripheral_resp;
  
  // External peripherals select
  logic     [LogExtPeriphNSlave-1:0] crheepto_periph_select;
  reg_req_t [ExtPeriphNSlaveRnd-1:0] crheepto_peripheral_req;
  reg_rsp_t [ExtPeriphNSlaveRnd-1:0] crheepto_peripheral_rsp;

  // Pad controller
  reg_req_t pad_req;
  reg_rsp_t pad_rsp;
% if pads_attributes != None:
  logic [core_v_mini_mcu_pkg::NUM_PAD-1:0][${pads_attributes['bits']}] pad_attributes;
% endif
% if total_pad_muxed > 0:
  logic [core_v_mini_mcu_pkg::NUM_PAD-1:0][${max_total_pad_mux_bitlengh-1}:0] pad_muxes;
% endif

  // External power domains
  logic [ExtDomainsRnd-1:0] external_subsystem_powergate_switch_n;
  logic [ExtDomainsRnd-1:0] external_subsystem_powergate_switch_ack_n;
  logic [ExtDomainsRnd-1:0] external_subsystem_powergate_iso_n;

  // External RAM banks retentive mode control
  logic [ExtDomainsRnd-1:0] external_ram_banks_set_retentive_n;

  // External domains reset
  logic [ExtDomainsRnd-1:0] external_subsystem_rst_n;

  // External domains clock-gating
  logic [ExtDomainsRnd-1:0] external_subsystem_clkgate_en_n;

  logic ext_debug_req;
  logic ext_debug_reset_n;

  if_xif #(.X_NUM_RS(3)) ext_xif ();
  coproc_wrapper coproc_wrapper_i(
    .clk_i (clk_i),
    .rst_ni(rst_ni),
    // eXtension interface
    .xif_compressed_if(ext_xif),
    .xif_issue_if(ext_xif),
    .xif_commit_if(ext_xif),
    .xif_mem_if(ext_xif),
    .xif_mem_result_if(ext_xif),
    .xif_result_if(ext_xif)
  );

  // CORE-V-MINI-MCU input/output pins
% for pad in total_pad_list:
${pad.internal_signals}
% endfor

  // Drive to zero bypassed pins
% for pad in total_pad_list:
% if pad.pad_type == 'bypass_inout' or pad.pad_type == 'bypass_input':
% for i in range(len(pad.pad_type_drive)):
% if pad.driven_manually[i] == False:
  assign ${pad.in_internal_signals[i]} = 1'b0;
% endif
% endfor
% endif
% endfor

  // --------------
  // SYSTEM MODULES
  // --------------

  // Reset generator
  // ---------------
  rstgen u_rstgen (
    .clk_i      (clk_in_x),
    .rst_ni     (rst_nin_x),
    .test_mode_i(1'b0 ), // not implemented
    .rst_no     (rst_nin_sync),
    .init_no    () // unused
  );



  // CORE-V-MINI-MCU (microcontroller)
  // ---------------------------------
  core_v_mini_mcu #(
    .COREV_PULP      (CpuCorevPulp),
    .FPU             (CpuFpu),
    .ZFINX           (CpuRiscvZfinx),
    .EXT_XBAR_NMASTER(ExtXbarNMaster),
    .X_EXT           (CpuCorevXif),
    .AO_SPC_NUM      (AoSPCNum),
    .EXT_HARTS       (1)
  ) u_core_v_mini_mcu (
    .rst_ni (rst_nin_sync),
    // MCU pads
% for pad in pad_list:
${pad.core_v_mini_mcu_bonding}
% endfor

`ifdef FPGA
    .spi_flash_cs_1_o (),
    .spi_flash_cs_1_i ('0),
    .spi_flash_cs_1_oe_o(),
`endif

    // CORE-V eXtension Interface
    .xif_compressed_if (ext_xif.cpu_compressed),
    .xif_issue_if      (ext_xif.cpu_issue),
    .xif_commit_if     (ext_xif.cpu_commit),
    .xif_mem_if        (ext_xif.cpu_mem),
    .xif_mem_result_if (ext_xif.cpu_mem_result),
    .xif_result_if     (ext_xif.cpu_result),

    // Pad controller interface
    .pad_req_o  (pad_req),
    .pad_resp_i (pad_rsp),

    // External slave ports
    .ext_xbar_master_req_i (heep_slave_req),
    .ext_xbar_master_resp_o (heep_slave_rsp),

    // External master ports
    .ext_core_instr_req_o (heep_core_instr_req),
    .ext_core_instr_resp_i (heep_core_instr_rsp),
    .ext_core_data_req_o (heep_core_data_req),
    .ext_core_data_resp_i (heep_core_data_rsp),
    .ext_debug_master_req_o (heep_debug_master_req),
    .ext_debug_master_resp_i (heep_debug_master_rsp),
    .ext_dma_read_req_o (heep_dma_read_req),
    .ext_dma_read_resp_i (heep_dma_read_rsp),
    .ext_dma_write_req_o (heep_dma_write_req),
    .ext_dma_write_resp_i (heep_dma_write_rsp),
    .ext_dma_addr_req_o (heep_dma_addr_req),
    .ext_dma_addr_resp_i (heep_dma_addr_rsp),

    // External peripherals slave ports
    .ext_peripheral_slave_req_o  (heep_peripheral_req),
    .ext_peripheral_slave_resp_i (heep_peripheral_rsp),

    // SPC signals
    .ext_ao_peripheral_slave_req_i(ext_ao_peripheral_req),
    .ext_ao_peripheral_slave_resp_o(ext_ao_peripheral_resp),

    // Power switches connected by the backend
    .cpu_subsystem_powergate_switch_no            (cpu_subsystem_powergate_switch_n),
    .cpu_subsystem_powergate_switch_ack_ni        (cpu_subsystem_powergate_switch_ack_n),
    .peripheral_subsystem_powergate_switch_no     (peripheral_subsystem_powergate_switch_n),
    .peripheral_subsystem_powergate_switch_ack_ni (peripheral_subsystem_powergate_switch_ack_n),

    .external_subsystem_powergate_switch_no(external_subsystem_powergate_switch_n),
    .external_subsystem_powergate_switch_ack_ni(external_subsystem_powergate_switch_ack_n),
    .external_subsystem_powergate_iso_no(external_subsystem_powergate_iso_n),

    // Control signals for external peripherals
    .external_subsystem_rst_no (external_subsystem_rst_n),
    .external_ram_banks_set_retentive_no (external_ram_banks_set_retentive_n),
    .external_subsystem_clkgate_en_no (external_subsystem_clkgate_en_n),

    // External interrupts
    .intr_vector_ext_i (ext_int_vector),

    .ext_dma_slot_tx_i('0),
    .ext_dma_slot_rx_i('0),

    .ext_debug_req_o(ext_debug_req),
    .ext_debug_reset_no(ext_debug_reset_n),
    .ext_cpu_subsystem_rst_no(),

    .ext_dma_stop_i('0),
    .dma_done_o(),
    
    .hw_fifo_req_o(),
    .hw_fifo_resp_i(),
    .hw_fifo_done_i(),
    
    .intr_ext_peripheral_i(),

    .exit_value_o (exit_value)
  );


  assign cpu_subsystem_powergate_switch_ack_n = cpu_subsystem_powergate_switch_n;
  assign peripheral_subsystem_powergate_switch_ack_n = peripheral_subsystem_powergate_switch_n;

  // External peripherals
  // --------------------
  crheepto_peripherals u_crheepto_peripherals (
      .clk_i (clk_in_x),
  % if (xbar_nmasters == 0 and xbar_nslaves == 0 and periph_nslaves == 0 and ext_interrupts == 0):
      .rst_ni (rst_nin_sync)
  % else:
      .rst_ni (rst_nin_sync),
  % endif
  % if (xbar_nmasters > 0):
      // External peripherals master ports
      .crheepto_master_req_o(),
      .crheepto_master_resp_i(),
  % endif

  % if (xbar_nslaves > 0):
      // External peripherals slave ports
      .crheepto_slave_req_i(ext_slave_req),
      .crheepto_slave_resp_o(ext_slave_resp),
  % endif

  % if (periph_nslaves > 0):
      // External peripherals configuration ports
      .crheepto_peripheral_req_i(crheepto_peripheral_req),
      .crheepto_peripheral_rsp_o(crheepto_peripheral_rsp),
  % endif

  % if (ext_interrupts > 0):
      // External peripherals interrupt ports
      .crheepto_peripheral_int_o(ext_int_vector[${ext_interrupts-1}:0]),
  %endif

  );


  // Unused signals
  // --------------
  assign crheepto_master_req = '0;
  assign ext_int_vector[11:${ext_interrupts}] = '0;

  localparam int unsigned IdxWidth = cf_math_pkg::idx_width(ExtXbarNSlave);

  // External BUS
  // ------------
  //ext_bus #(
  //  .EXT_XBAR_NMASTER (ExtXbarNMasterRnd),
  //  .EXT_XBAR_NSLAVE  (ExtXbarNSlaveRnd)
  //) u_ext_bus (
  //  .clk_i (clk_in_x),
  //  .rst_ni (rst_nin_sync),
//
  //  // Address map
  //  .addr_map_i(ExtSlaveAddrRules),
  //  .default_idx_i(ExtSlaveDefaultIdx[IdxWidth-1:0]),
//
  //  // X-HEEP master ports
  //  .heep_core_instr_req_i(heep_core_instr_req),
  //  .heep_core_instr_resp_o(heep_core_instr_rsp),
//
  //  .heep_core_data_req_i(heep_core_data_req),
  //  .heep_core_data_resp_o(heep_core_data_rsp),
//
  //  .heep_debug_master_req_i(heep_debug_master_req),
  //  .heep_debug_master_resp_o(heep_debug_master_rsp),
//
  //  .heep_dma_read_req_i(heep_dma_read_req),
  //  .heep_dma_read_resp_o(heep_dma_read_rsp),
//
  //  .heep_dma_write_req_i(heep_dma_write_req),
  //  .heep_dma_write_resp_o(heep_dma_write_rsp),
//
  //  .heep_dma_addr_req_i(heep_dma_addr_req),
  //  .heep_dma_addr_resp_o(heep_dma_addr_rsp),
//
  //  // X-HEEP master ports
  //  .ext_master_req_i(crheepto_master_req),
  //  .ext_master_resp_o(),
//
  //  // X-HEEP slave ports
  //  .heep_slave_req_o(heep_slave_req),
  //  .heep_slave_resp_i(heep_slave_rsp),
//
  //  // External slave ports
  //  .ext_slave_req_o(ext_slave_req),
  //  .ext_slave_resp_i(ext_slave_resp)
//
  //);

  // External peripherals bus
  // ------------------------
  addr_decode #(
      .NoIndices(crheepto_pkg::ExtPeriphNSlaveRnd),
      .NoRules(crheepto_pkg::ExtPeriphNSlaveRnd),
      .addr_t(logic [31:0]),
      .rule_t(addr_map_rule_pkg::addr_map_rule_t)
  ) u_crheepto_perpheral_decoder (
      .addr_i(heep_peripheral_req.addr),
      .addr_map_i(crheepto_pkg::ExtPeriphAddrRules),
      .idx_o(crheepto_periph_select),
      .dec_valid_o(),
      .dec_error_o(),
      .en_default_idx_i(1'b0),
      .default_idx_i('0)
  );

  reg_demux #(
      .NoPorts(crheepto_pkg::ExtPeriphNSlaveRnd),
      .req_t  (reg_pkg::reg_req_t),
      .rsp_t  (reg_pkg::reg_rsp_t)
  ) u_crheepto_reg_demux (
      .clk_i(clk_in_x),
      .rst_ni(rst_nin_sync),
      .in_select_i(crheepto_periph_select),
      .in_req_i(heep_peripheral_req),
      .in_rsp_o(heep_peripheral_rsp),
      .out_req_o(crheepto_peripheral_req),
      .out_rsp_i(crheepto_peripheral_rsp)
  );

  // External interrupts
  // -------------------

  // Pad ring
  // --------

  assign exit_value_out_x = exit_value[0];

  pad_ring u_pad_ring (
% for pad in total_pad_list:
${pad.pad_ring_bonding_bonding}
% endfor

    // Pad attributes
% if pads_attributes != None:
    .pad_attributes_i(pad_attributes)
% else:
    .pad_attributes_i('0)
% endif
  );

  // Constant pad signals
${pad_constant_driver_assign}

  // Shared pads multiplexing
${pad_mux_process}

  // Pad control
  // -----------
  pad_control #(
    .reg_req_t (reg_req_t),
    .reg_rsp_t (reg_rsp_t),
    .NUM_PAD   (NUM_PAD)
  ) u_pad_control (
    .clk_i            (clk_in_x),
    .rst_ni           (rst_nin_sync),
    .reg_req_i        (pad_req),
    .reg_rsp_o        (pad_rsp)
% if total_pad_muxed > 0 or pads_attributes != None:
      ,
% endif
% if pads_attributes != None:
      .pad_attributes_o(pad_attributes)
% if total_pad_muxed > 0:
      ,
% endif
% endif
% if total_pad_muxed > 0:
      .pad_muxes_o(pad_muxes)
% endif
  );

endmodule // crheepto_top