// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Keccak Package                                                         //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Type definitions and parameters for Keccak-f[1600] (lanes, planes,    //
//               state). Based on design by M. Peeters and G. Van Assche.              //
///////////////////////////////////////////////////////////////////////////////////////////

package pkg_keccak;


    localparam int NUM_PLANE             = 5;
    localparam int NUM_SHEET             = 5;
    localparam int unsigned KEC_N        = 64;
    localparam int unsigned IN_BUF_SIZE  = 64;
    localparam int unsigned OUT_BUF_SIZE = 64;


    typedef logic   [KEC_N-1:0]             k_lane;
    typedef k_lane  [NUM_SHEET-1:0]     k_plane;
    typedef k_plane [NUM_PLANE-1:0]     k_state;


    function automatic int ABS (int numberIn);
        ABS = (numberIn < 0) ? -numberIn : numberIn;
    endfunction


endpackage
