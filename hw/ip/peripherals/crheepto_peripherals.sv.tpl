// Copyright 2024 Politecnico di Torino.
// Copyright and related rights are licensed under the Solderpad Hardware
// License, Version 2.0 (the "License"); you may not use this file except in
// compliance with the License. You may obtain a copy of the License at
// http://solderpad.org/licenses/SHL-2.0. Unless required by applicable law
// or agreed to in writing, software, hardware and materials distributed under
// this License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
// File: crheepto_peripherals.sv
// Author(s):
//   Luigi Giuffrida, Valeria Piscopo
// Date: 09/01/2026
// Description: Template for the crheepto peripherals module

module crheepto_peripherals (
    /* verilator lint_off UNUSED */
    input logic clk_i,
% if (xbar_nmasters == 0 and xbar_nslaves == 0 and periph_nslaves == 0 and ext_interrupts == 0):
    input logic rst_ni
% else:
    input logic rst_ni,
% endif
  /* verilator lint_off UNDRIVEN */
  // TODO: Remove the verilator lint_off pragmas when the signals are used

% if (xbar_nmasters > 0):
    // External peripherals master ports
    output obi_pkg::obi_req_t  [crheepto_pkg::ExtXbarNMasterRnd-1:0] crheepto_master_req_o,
    input obi_pkg::obi_resp_t [crheepto_pkg::ExtXbarNMasterRnd-1:0] crheepto_master_resp_i${'' if (xbar_nslave + periph_nslaves + ext_interrupts == 0) else ','}
% endif

% if (xbar_nslaves > 0):
    // External peripherals slave ports
    input obi_pkg::obi_req_t  [crheepto_pkg::ExtXbarNSlaveRnd-1:0] crheepto_slave_req_i,
    output obi_pkg::obi_resp_t [crheepto_pkg::ExtXbarNSlaveRnd-1:0] crheepto_slave_resp_o${'' if (periph_nslaves + ext_interrupts == 0) else ','}
% endif

% if (periph_nslaves > 0):
    // External peripherals configuration ports
    input reg_pkg::reg_req_t [crheepto_pkg::ExtPeriphNSlaveRnd-1:0] crheepto_peripheral_req_i,
    output reg_pkg::reg_rsp_t [crheepto_pkg::ExtPeriphNSlaveRnd-1:0] crheepto_peripheral_rsp_o${'' if (ext_interrupts == 0) else ','}
% endif

% if (ext_interrupts > 0):
    // External peripherals interrupt ports
    output logic [crheepto_pkg::ExtInterrupts-1:0] crheepto_peripheral_int_o,
%endif

);



endmodule
