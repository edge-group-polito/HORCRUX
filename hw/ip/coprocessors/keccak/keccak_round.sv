// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  Keccak Round Function                                                  //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  Single Keccak-f[1600] round with theta, pi, rho, chi, iota steps.     //
//               Based on design by M. Peeters and G. Van Assche.                      //
///////////////////////////////////////////////////////////////////////////////////////////

import pkg_keccak::k_plane;
import pkg_keccak::k_state;
import pkg_keccak::KEC_N;
import pkg_keccak::ABS;

module keccak_round (
    input   k_state         Round_in,
    input   [7:0]     Round_constant_signal,
    output  k_state         Round_out);


k_state theta_in, theta_out, pi_in, pi_out, rho_in, rho_out,
        chi_in, chi_out, iota_in, iota_out;

k_plane sum_sheet;

// Connections

// Order is theata, pi, rho, chi, iota
assign  theta_in    = Round_in;
assign  pi_in       = rho_out;
assign  rho_in      = theta_out;
assign  chi_in      = pi_out;
assign  iota_in     = chi_out;
assign  Round_out   = iota_out;

genvar y,x,i;


// Chi
generate
    for(y = 0; y <= 4; y++)
        for(x = 0; x <= 2; x++)
            for(i = 0; i <= KEC_N-1; i++)
                assign chi_out[y][x][i] = chi_in[y][x][i] ^ ( ~(chi_in[y][x+1][i]) & chi_in[y][x+2][i]);
endgenerate

generate
    for(y = 0; y <= 4; y++)
        for(i = 0; i <= KEC_N-1; i++)
            assign chi_out[y][3][i] = chi_in[y][3][i] ^ ( ~(chi_in[y][4][i]) & chi_in[y][0][i]);
endgenerate

generate
    for(y = 0; y <= 4; y++)
        for(i = 0; i <= KEC_N-1; i++)
            assign chi_out[y][4][i] = chi_in[y][4][i] ^ ( ~(chi_in[y][0][i]) & chi_in[y][1][i]);
endgenerate

// Theta

//compute the sum of the columns
generate
    for(x = 0; x <= 4; x++)
        for(i = 0; i <= KEC_N-1; i++)
            assign sum_sheet[x][i] = theta_in[0][x][i] ^ theta_in[1][x][i] ^ theta_in[2][x][i] ^ theta_in[3][x][i] ^ theta_in[4][x][i];
endgenerate

generate
    for(y = 0; y <= 4; y++)
        for(x = 1; x <= 3; x++) begin

            assign theta_out[y][x][0] = theta_in[y][x][0] ^ sum_sheet[x-1][0] ^ sum_sheet[x+1][KEC_N-1];

            for(i = 1; i <= KEC_N-1; i++)
                assign theta_out[y][x][i] = theta_in[y][x][i] ^ sum_sheet[x-1][i] ^ sum_sheet[x+1][i-1];
        end
endgenerate

generate
    for(y = 0; y <= 4; y++) begin
        assign theta_out[y][0][0] = theta_in[y][0][0] ^ sum_sheet[4][0] ^ sum_sheet[1][KEC_N-1];

        for(i = 1; i <= KEC_N-1; i++)
            assign theta_out[y][0][i] = theta_in[y][0][i] ^ sum_sheet[4][i] ^ sum_sheet[1][i-1];
    end
endgenerate

generate
    for(y = 0; y <= 4; y++) begin
        assign theta_out[y][4][0] = theta_in[y][4][0] ^ sum_sheet[3][0] ^ sum_sheet[0][KEC_N-1];

        for(i = 1; i <= KEC_N-1; i++)
            assign theta_out[y][4][i] = theta_in[y][4][i] ^ sum_sheet[3][i] ^ sum_sheet[0][i-1];
    end
endgenerate

// Pi
generate
    for(y = 0; y <= 4; y++)
        for(x = 0; x <= 4; x++)
            for(i = 0; i <= KEC_N-1; i++)
                assign pi_out[(2*x+3*y) % 5][0*x+1*y][i] = pi_in[y][x][i];
endgenerate

// Rho
always_comb begin
    for(int ri = 0; ri < KEC_N; ri++) begin
        rho_out[0][0][ri] = rho_in[0][0][ri];
        rho_out[0][1][ri] = rho_in[0][1][ABS((ri-1)  % KEC_N)];
        rho_out[0][2][ri] = rho_in[0][2][ABS((ri-62) % KEC_N)];
        rho_out[0][3][ri] = rho_in[0][3][ABS((ri-28) % KEC_N)];
        rho_out[0][4][ri] = rho_in[0][4][ABS((ri-27) % KEC_N)];

        rho_out[1][0][ri] = rho_in[1][0][ABS((ri-36) % KEC_N)];
        rho_out[1][1][ri] = rho_in[1][1][ABS((ri-44) % KEC_N)];
        rho_out[1][2][ri] = rho_in[1][2][ABS((ri-6)  % KEC_N)];
        rho_out[1][3][ri] = rho_in[1][3][ABS((ri-55) % KEC_N)];
        rho_out[1][4][ri] = rho_in[1][4][ABS((ri-20) % KEC_N)];

        rho_out[2][0][ri] = rho_in[2][0][ABS((ri-3)  % KEC_N)];
        rho_out[2][1][ri] = rho_in[2][1][ABS((ri-10) % KEC_N)];
        rho_out[2][2][ri] = rho_in[2][2][ABS((ri-43) % KEC_N)];
        rho_out[2][3][ri] = rho_in[2][3][ABS((ri-25) % KEC_N)];
        rho_out[2][4][ri] = rho_in[2][4][ABS((ri-39) % KEC_N)];

        rho_out[3][0][ri] = rho_in[3][0][ABS((ri-41) % KEC_N)];
        rho_out[3][1][ri] = rho_in[3][1][ABS((ri-45) % KEC_N)];
        rho_out[3][2][ri] = rho_in[3][2][ABS((ri-15) % KEC_N)];
        rho_out[3][3][ri] = rho_in[3][3][ABS((ri-21) % KEC_N)];
        rho_out[3][4][ri] = rho_in[3][4][ABS((ri-8)  % KEC_N)];

        rho_out[4][0][ri] = rho_in[4][0][ABS((ri-18) % KEC_N)];
        rho_out[4][1][ri] = rho_in[4][1][ABS((ri-2)  % KEC_N)];
        rho_out[4][2][ri] = rho_in[4][2][ABS((ri-61) % KEC_N)];
        rho_out[4][3][ri] = rho_in[4][3][ABS((ri-56) % KEC_N)];
        rho_out[4][4][ri] = rho_in[4][4][ABS((ri-14) % KEC_N)];
    end
end

// Iota
generate
    for(y = 1; y <= 4; y++)
        for(x = 0; x <= 4; x++)
            for(i = 0; i <= KEC_N-1; i++)
                assign iota_out[y][x][i] = iota_in[y][x][i];
endgenerate

generate
    for(x = 1; x <= 4; x++)
        for(i = 0; i <= KEC_N-1; i++)
            assign iota_out[0][x][i] = iota_in[0][x][i];
endgenerate

//generate
//    for(i = 0; i <= KEC_N-1; i++)
//        assign iota_out[0][0][i] = iota_in[0][0][i] ^ Round_constant_signal[i];
//endgenerate
always_comb begin
    // 1. Default: copy the entire 64-bit lane from input to output
    iota_out[0][0] = iota_in[0][0];

    // 2. Override specific bits by XORing with the 8-bit constant bits
    // Mapping matches your VHDL: rc(0..3) -> bit 0,1,2,3; rc(4) -> bit 7; rc(5) -> bit 15, etc.
    iota_out[0][0][0]  = iota_in[0][0][0]  ^ Round_constant_signal[0];
    iota_out[0][0][1]  = iota_in[0][0][1]  ^ Round_constant_signal[1];
    iota_out[0][0][2]  = iota_in[0][0][2]  ^ Round_constant_signal[2];
    iota_out[0][0][3]  = iota_in[0][0][3]  ^ Round_constant_signal[3];
    
    iota_out[0][0][7]  = iota_in[0][0][7]  ^ Round_constant_signal[4];
    iota_out[0][0][15] = iota_in[0][0][15] ^ Round_constant_signal[5];
    iota_out[0][0][31] = iota_in[0][0][31] ^ Round_constant_signal[6];
    iota_out[0][0][63] = iota_in[0][0][63] ^ Round_constant_signal[7];
end


endmodule
