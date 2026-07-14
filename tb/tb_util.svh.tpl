// Copyright 2022 OpenHW Group
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1

// crheepto top-level

`ifdef VERILATOR
`define TOP u_crheepto_top
`else
`define TOP u_tb_system.u_crheepto_top
`endif

// task for loading 'mem' with SystemVerilog system task $readmemh()
export "DPI-C" task tb_readHEX;
export "DPI-C" task tb_loadHEX;
export "DPI-C" task tb_getMemSize;
export "DPI-C" task tb_set_exit_loop;

import core_v_mini_mcu_pkg::*;

function bit tb_check_if_any_not_X(logic [31:0] input_word);
  for(int unsigned i = 0; i < 32; i=i+1) begin
    if ( input_word[i] !== 1'bx ) return 1;
  end
  return 0;
endfunction


task tb_getMemSize;
  output int mem_size;
  output int num_banks;
  mem_size  = core_v_mini_mcu_pkg::MEM_SIZE;
  num_banks = core_v_mini_mcu_pkg::NUM_BANKS;
endtask

task tb_readHEX;
  input string file;
  output logic [7:0] stimuli[core_v_mini_mcu_pkg::MEM_SIZE];
  $readmemh(file, stimuli);
endtask

task tb_loadHEX;
  input string file;
  //whether to use debug to write to memories
  logic [7:0] stimuli[core_v_mini_mcu_pkg::MEM_SIZE];
  int i, stimuli_base, w_addr, NumBytes, NumBanks;
  logic [31:0] addr;
  logic [31:0] word;
  int write_res;

  tb_readHEX(file, stimuli);
  tb_getMemSize(NumBytes, NumBanks);

`ifdef LOADHEX_DBG

`ifdef VERILATOR
  $fatal("ERR! LOADHEX_DBG not supported in Verilator");
`endif

  for (i = 0; i < NumBytes; i = i + 4) begin

    if( tb_check_if_any_not_X({stimuli[i+3], stimuli[i+2], stimuli[i+1], stimuli[i]}) ) begin
      @(posedge `TOP.u_core_v_mini_mcu.clk_i);
      addr = i;
      #1;
      // write to memory
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_req_o = 1'b1;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_addr_o = addr;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_we_o = 1'b1;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_be_o = 4'b1111;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_wdata_o = {
        stimuli[i+3], stimuli[i+2], stimuli[i+1], stimuli[i]
      };

      while(!`TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_gnt_i) begin
        @(posedge `TOP.u_core_v_mini_mcu.clk_i);
      end

      #1;
      force `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_req_o = 1'b0;

      wait (`TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_rvalid_i);

      #1;
    end

  end

  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_req_o;
  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_addr_o;
  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_we_o;
  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_be_o;
  release `TOP.u_core_v_mini_mcu.debug_subsystem_i.dm_obi_top_i.master_wdata_o;

`else // LOADHEX_DBG

% for bank in xheep.iter_ram_banks():
  for (i=${bank.start_address()}; i < ${bank.end_address()}; i = i + 4) begin
    if (((i/4) & ${2**bank.il_level()-1}) == ${bank.il_offset()}) begin
      w_addr = ((i/4) >> ${bank.il_level()}) % ${bank.size()//4};
      tb_writetoSram${bank.name()}(w_addr, stimuli[i+3], stimuli[i+2],
                                          stimuli[i+1], stimuli[i]);
    end
  end
% endfor

`ifndef VERILATOR
  // Release memory signals
% for bank in range(xheep.ram_numbanks()):
  tb_releaseSram${bank}();
% endfor

`endif // VERILATOR

`endif // LOADHEX_DBG
endtask

% for bank in xheep.iter_ram_banks():
task tb_writetoSram${bank.name()};
  input int addr;
  input [7:0] val3;
  input [7:0] val2;
  input [7:0] val1;
  input [7:0] val0;

`ifdef VCS
  force `TOP.u_core_v_mini_mcu.memory_subsystem_i.ram${bank.name()}_i.tc_ram_i.sram[addr] = {
    val3, val2, val1, val0
  };
  release `TOP.u_core_v_mini_mcu.memory_subsystem_i.ram${bank.name()}_i.tc_ram_i.sram[addr];
`else
`ifndef SYNTHESIS
  `TOP.u_core_v_mini_mcu.memory_subsystem_i.ram${bank.name()}_i.tc_ram_i.sram[addr] = {
    val3, val2, val1, val0
  };
`else
  parameter numWordAddr = 13;
  parameter numRowAddr = 9;
  parameter numCMAddr = 4;

  logic [numWordAddr-numCMAddr-1:0] iRowAddr;
  logic [numCMAddr-1:0] iColAddr;

  assign iRowAddr = addr[numWordAddr-1:numCMAddr];
  assign iColAddr = addr[numCMAddr-1:0];

  `TOP.u_core_v_mini_mcu.memory_subsystem_i.ram${bank.name()}_i.genblk2_sram_i.MEMORY[iRowAddr][iColAddr] = {
    val3, val2, val1, val0
  };
`endif
`endif
endtask
% endfor

`ifndef VERILATOR

% for bank in xheep.iter_ram_banks():
function void tb_releaseSram${bank.name()};
`ifndef SYNTHESIS
  release `TOP.u_core_v_mini_mcu.memory_subsystem_i.ram${bank.name()}_i.tc_ram_i.sram;
`else
  release `TOP.u_core_v_mini_mcu.memory_subsystem_i.ram${bank.name()}_i.genblk2_sram_i.MEMORY;
`endif
endfunction
% endfor

`endif // VERILATOR


task tb_set_exit_loop;
`ifdef VCS
  force `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.testbench_set_exit_loop[0] = 1'b1;
  release `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.testbench_set_exit_loop[0];
`elsif VERILATOR
  `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.testbench_set_exit_loop[0] = 1'b1;
`else
  `ifndef SYNTHESIS
    `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.testbench_set_exit_loop[0] = 1'b1;
  `else
    // careful this code may change after synthesis
    while ( 1 ) begin
      // NOTE: do not remove the following line. It prevents the wait()
      // statement from detecting the request on a clock edge, that results in
      // the rdata signal being set for the *next* cycle instead of the current
      // one.
      #1;
      wait ( `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.reg_req_i[69] );  //this should be the valid signals
      if ( `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.reg_req_i[35:34] == 2'b11 ) begin //this should be the address 2000_000C
        force `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.reg_rsp_o[0] = 1'b1;
        @(posedge `TOP.u_core_v_mini_mcu.clk_i);
        release `TOP.u_core_v_mini_mcu.ao_peripheral_subsystem_i.soc_ctrl_i.reg_rsp_o;
        break;
      end else @(posedge `TOP.u_core_v_mini_mcu.clk_i);
    end
  `endif
`endif
endtask

