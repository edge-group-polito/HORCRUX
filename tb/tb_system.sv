// Copyright 2024 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: tb_system.sv
// Author: Michele Caon, Luigi Giuffrida, Valeria Piscopo
// Date: 09/01/2026
// Description: crheepto testbench system

module tb_system #(
    parameter int unsigned CLK_FREQ = 32'd100_000  // kHz
) (
    inout logic clk_i,
    inout logic rst_ni,

    // Static configuration
    inout logic boot_select_i,
    inout logic execute_from_flash_i,

    // Exit signals
    inout logic        exit_valid_o,
    inout logic [31:0] exit_value_o
);
  // Include testbench utils
  `include "tb_util.svh"

  // INTERNAL SIGNALS
  // ----------------
  // JTAG
  wire jtag_tck = '0;
  wire jtag_tms = '0;
  wire jtag_trst_n = '0;
  wire jtag_tdi = '0;
  wire jtag_tdo = '0;

  // UART
  wire                          crheepto_uart_tx;
  wire                          crheepto_uart_rx;

  // GPIO
  /* verilator lint_off UNUSED */
  /* verilator lint_off UNDRIVEN */
  wire                   [14:0] gpio;

  // SPI flash
  wire                          spi_flash_sck;
  wire                   [ 1:0] spi_flash_csb;
  wire                   [ 3:0] spi_flash_sd_io;

  // SPI
  wire                          spi_sck;
  wire                   [ 1:0] spi_csb;
  wire                   [ 3:0] spi_sd_io;

 // SPI Slave
  wire                          spi_slave_sck_io;
  wire                          spi_slave_cs_io;

  // GPIO
  wire                          clk_div;

  // I2C
  tri1                          i2c_sda_io;
  tri1                          i2c_scl_io;

  // I2S
  wire                          i2s_sck_io;
  wire                          i2s_ws_io;
  wire                          i2s_sd_io;

  // PAD MUX TEST
  wire                          test_pad_mux;

  // UART DPI emulator
  uartdpi #(
      .BAUD('d256000),
      .FREQ(CLK_FREQ * 1000),  // Hz
      .NAME("uart")
  ) u_uartdpi (
      .clk_i (clk_i),
      .rst_ni(rst_ni),
      .tx_o  (crheepto_uart_rx),
      .rx_i  (crheepto_uart_tx)
  );

  // SPI flash emulator
`ifndef VERILATOR
  spiflash u_flash_boot (
      .csb(spi_flash_csb[0]),
      .clk(spi_flash_sck),
      .io0(spi_flash_sd_io[0]),
      .io1(spi_flash_sd_io[1]),
      .io2(spi_flash_sd_io[2]),
      .io3(spi_flash_sd_io[3])
  );

  spiflash u_flash_device (
      .csb(spi_csb[0]),
      .clk(spi_sck),
      .io0(spi_sd_io[0]),
      .io1(spi_sd_io[1]),
      .io2(spi_sd_io[2]),
      .io3(spi_sd_io[3])
  );
`endif  /* VERILATOR */

  gpio_cnt #(
      .CntMax(32'd16)
  ) u_test_gpio (
      .clk_i (clk_i),
      .rst_ni(rst_ni),
      .gpio_i(gpio[13]),
      .gpio_o(gpio[14])
  );

  // I2s "microphone"/rx example
  i2s_microphone i2s_microphone_i (
      .rst_ni(rst_ni),
      .i2s_sck_i(i2s_sck_io),
      .i2s_ws_i(i2s_ws_io),
      .i2s_sd_o(i2s_sd_io)
  );

`ifdef USE_PG_PIN
  import UPF::*;

  supply1 VDD;
  supply0 VSS;

  initial begin
    $display("%t: All Power Supply ON (USE_PG_PIN)", $time);
    supply_on("VDD", 1.2);
    supply_on("VSS", 0);
  end

`endif

`ifdef POSTLAYOUT
`ifdef LOAD_SDF
  initial begin
    $sdf_annotate("../../../implementation/layout/artefacts_latest/export/crheepto.sdf.gz", tb_top.u_tb_system.u_crheepto_top,, "sdf.log", "TYPICAL",,);
  end
`endif
`endif

  // DUT
  // ---
  crheepto_top u_crheepto_top (
`ifdef USE_PG_PIN
      .VSS,
      .VDD,
`endif
      .clk_i               (clk_i),
      .rst_ni              (rst_ni),
      .boot_select_i       (boot_select_i),
      .execute_from_flash_i(execute_from_flash_i),
      .jtag_tck_i          (jtag_tck),
      .jtag_tms_i          (jtag_tms),
      .jtag_trst_ni        (jtag_trst_n),
      .jtag_tdi_i          (jtag_tdi),
      .jtag_tdo_o          (jtag_tdo),
      .uart_rx_i           (crheepto_uart_rx),
      .uart_tx_o           (crheepto_uart_tx),
      .exit_valid_o        (exit_valid_o),
      .exit_value_o        (exit_value_o[0]),
      .gpio_0_io           (gpio[0]),
      .gpio_1_io           (gpio[1]),
      .gpio_2_io           (gpio[2]),
      .gpio_3_io           (gpio[3]),
      .gpio_4_io           (gpio[4]),
      .spi_flash_sck_io    (spi_flash_sck),
      .spi_flash_cs_0_io   (spi_flash_csb[0]),
      .spi_flash_cs_1_io   (spi_flash_csb[1]),
      .spi_flash_sd_0_io   (spi_flash_sd_io[0]),
      .spi_flash_sd_1_io   (spi_flash_sd_io[1]),
      .spi_flash_sd_2_io   (spi_flash_sd_io[2]),
      .spi_flash_sd_3_io   (spi_flash_sd_io[3]),
      .spi_sck_io          (spi_sck),
      .spi_cs_0_io         (spi_csb[0]),
      .spi_cs_1_io         (spi_csb[1]),
      .spi_sd_0_io         (spi_sd_io[0]),
      .spi_sd_1_io         (spi_sd_io[1]),
      .spi_sd_2_io         (spi_sd_io[2]),
      .spi_sd_3_io         (spi_sd_io[3]),
      .spi_slave_sck_io    (spi_slave_sck_io),
      .spi_slave_cs_io     (spi_slave_cs_io),
      .spi_slave_miso_io   (),
      .spi_slave_mosi_io   (gpio[4]),
      .i2s_sck_io          (i2s_sck_io),
      .i2s_ws_io           (i2s_ws_io),
      .i2s_sd_io           (i2s_sd_io),
      .spi2_cs_0_io        (),
      .spi2_cs_1_io        (),
      .spi2_sck_io         (),
      .spi2_sd_0_io        (),
      .spi2_sd_1_io        (),
      .spi2_sd_2_io        (),
      .spi2_sd_3_io        (),
      .i2c_scl_io          (i2c_scl_io),
      .i2c_sda_io          (i2c_sda_io)
  );


  // Exit value
  assign exit_value_o[31:1] = u_crheepto_top.u_core_v_mini_mcu.exit_value_o[31:1];
endmodule
