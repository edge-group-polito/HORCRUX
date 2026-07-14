// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Keccak Control Unit                                                    //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  FSM control for Keccak-f[1600] permutation with round counter and     //
//               datapath handshake. Generates start signals and completion flags.      //
///////////////////////////////////////////////////////////////////////////////////////////

module keccak_cu (
     input logic  clk_i,
     input logic  rst_ni,
     input logic  start_i,
     input logic  ready_dp_i, 
     output logic start_dp_o,
     output logic status_d,
     output logic keccak_intr
   );


   parameter wait_start = 0,
             do_permutation = 1,
             permutation_finished = 2;
   
   reg[4:0]  			       counter;
   //shortint unsigned		       counter;
 			       
   
   reg [1:0] 			       State, State_next;

   // State reg
   always_ff @(posedge clk_i or negedge rst_ni) begin
      if (!rst_ni) begin
	 State <= wait_start;
	 counter <= 0;
      end else begin 
	if ( State_next == do_permutation) begin
	      counter <= counter+1;
	end else begin
	   counter <= 0;
	end 
	State <= State_next;
      end
   end 

   // Comb logic      
   always_comb begin
      case (State)
	wait_start : begin
	   keccak_intr = 0;
       status_d = 0;
	   if (start_i && ready_dp_i) begin
	      start_dp_o = 1;
	      State_next = do_permutation;
	   end else begin
	      start_dp_o = 0;
	      State_next = wait_start;	      
	   end
	end
	do_permutation : begin
	   start_dp_o = 0;
	   status_d = 0;
	   keccak_intr = 0;
	   if (counter == 24) begin
	      //din_keccak_o = 0;
	      State_next = permutation_finished;
	   end else begin
	      State_next = do_permutation;
	   end
        end
	permutation_finished : begin
	   start_dp_o = 0;
	   status_d = 1;
	   keccak_intr = 1;
	   State_next = wait_start;
        end 
	default : begin
	   start_dp_o = 0;
	   status_d = 0;
	   keccak_intr = 0;
	   State_next = wait_start;
	end
   endcase
   end // always_comb @

   endmodule : keccak_cu

	    
      
	 
	 
 
