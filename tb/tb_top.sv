// Copyright 2024 EPFL and Politecnico di Torino.
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1
//
// File: tb_top.sv
// Author: Michele Caon, Luigi Giuffrida, Valeria Piscopo
// Date: 09/01/2026
// Description: GR-HEEP testbench top-level module

module tb_top;
  // Include testbench utils
  `include "tb_util.svh"

  // PARAMETERS
  parameter string VCD_BASENAME = "waves-";
  parameter string VCD_DIR = "logs/";

  // Simulation clock (100MHz)
  const time SIM_CLK_PERIOD = 10ns;
  const time SIM_CLK_PHASE_HI = SIM_CLK_PERIOD / 2;
  const time SIM_CLK_PHASE_LO = SIM_CLK_PERIOD / 2;
  localparam int unsigned ClkFrequencykHz = 100_000;  // KHz

  // Timing delays
  const time         RESET_DEL = 1us;
  const int unsigned ResetWaitCycles = 50;

  // Watchdog heartbeat cycles
  localparam int unsigned WatchdogHeartbeatCycles = 100_000;

  // INTERNAL SIGNALS
  // ----------------
  // TB signals
  logic        sim_clk;
  logic        tb_rst_n;
  logic        sys_rst_n;

  // Wires to DUT
  wire         sim_clk_w;
  wire         tb_rst_nw;
  wire         boot_select;
  wire         exec_from_flash;
  wire         exit_valid;
  wire  [31:0] exit_value;

  // TB configuration
  typedef enum int {
    BOOT_MODE_JTAG  = 0,
    BOOT_MODE_FLASH = 1,
    BOOT_MODE_FORCE = 2
  } boot_mode_t;
  int          vcd_mode_opt;  // 0: no dump, 1: unconditional dump, 2: triggered by GPIO[0]
  int          boot_mode;  // 0: wait JTAG, 1: boot from flash, 2: load SRAM
  string       boot_mode_opt = "jtag";
  int          exec_from_flash_opt;
  string       firmware_file_opt;
  int          verbose;
  int unsigned max_cycles;

  // Cycle count
  int unsigned cycle_cnt_q;

  event rst_event;

  // VCD dump
  typedef enum logic [1:0] {
    IDLE,
    DUMP_INIT,
    WAIT,
    DUMP_OFF
  } fsm_state_t;
  enum int unsigned {
    VCD_MODE_OFF  = 0,  // no dump
    VCD_MODE_ON   = 1,  // unconditional dump
    VCD_MODE_TRIG = 2   // triggered dump
  } vcd_mode_e;
  fsm_state_t curr_state, next_state;
  logic vcd_trigger;
  bit   vcd_cnt_en;
  string vcd_filename_d, vcd_filename_q;
  int unsigned vcd_cnt;

  // ----------------
  // TB CONFIGURATION
  // ----------------

  // Parse command-line arguments
  // ----------------------------
  initial begin : cmd_args
    firmware_file_opt   = "";
    vcd_mode_opt        = 0;
    boot_mode           = 0;
    exec_from_flash_opt = 0;
    verbose             = 0;
    max_cycles          = 0;

    // Firmware file
    if ($value$plusargs("firmware=%s", firmware_file_opt)) begin
      $display("[CONFIG] Firmware file: %s", firmware_file_opt);
    end else begin
      $fatal("ERR! No firmware file specified");
    end

    // VCD dump
    $value$plusargs("vcd_mode=%d", vcd_mode_opt);
    $display("[CONFIG] VCD dump mode: %0d", vcd_mode_opt);

    // Boot select
    $value$plusargs("boot_mode=%s", boot_mode_opt);
    case (boot_mode_opt)
      "0", "jtag": begin
        $display("[CONFIG] Boot mode: wait JTAG");
        boot_mode = BOOT_MODE_JTAG;
      end
      "1", "flash": begin
        $display("[CONFIG] Boot mode: boot from flash");
        boot_mode = BOOT_MODE_FLASH;
      end
      "2", "force": begin
        $display("[CONFIG] Boot mode: force SRAM signals");
        boot_mode = BOOT_MODE_FORCE;
      end
      default: begin
        $error("ERR! Invalid boot mode. Defualting to JTAG");
        boot_mode = BOOT_MODE_JTAG;
      end
    endcase

    // Execute from flash
    if (boot_mode == BOOT_MODE_FLASH) begin
      $value$plusargs("exec_from_flash_opt=%d", exec_from_flash_opt);
      $display("[CONFIG] Execute from flash: %s", (exec_from_flash_opt ? "yes" : "no"));
    end

    // Verbose mode
    if ($test$plusargs("verbose")) begin
      $display("[CONFIG] Verbose mode enabled");
      verbose = 1;
    end

    // Max cycles
    if ($value$plusargs("max_cycles=%d", max_cycles)) begin
      $display("[CONFIG] Max cycles: %0d", max_cycles);
    end else begin
      $display("[CONFIG] Max cycles: unlimited");
    end
  end

  // Set timing format
  // -----------------
  initial begin : timing_format
    $timeformat(-9, 0, " ns", 9);
  end

  // -------------
  // LOAD FIRMWARE
  // -------------
  initial begin : load_firmware
    // Wait end of reset
    @(rst_event);
    @(posedge sim_clk);

    // Load firmware
    case (boot_mode)
      // Boot from JTAG
      // --------------
      BOOT_MODE_JTAG: begin
        if (verbose) $display("[%t] starting JTAG...", $time);
      end
      // Boot from SPI flash
      // -------------------
      BOOT_MODE_FLASH: begin
        if (verbose) $display("[%t] starting SPI flash...", $time);
      end
      // SRAM load
      // ---------
      BOOT_MODE_FORCE: begin
        $display("[%t] loading SRAM content...", $time);
        tb_loadHEX(firmware_file_opt);
        @(posedge sim_clk);
        $display("[%t] triggering boot loop exit...", $time);
        tb_set_exit_loop();
        $display("[%t] firmware loaded", $time);
      end
      default: begin
        $fatal("ERR! Invalid boot mode: %d", boot_mode);
      end
    endcase
  end

  // --------
  // VCD DUMP
  // --------
  // VCD dump FSM control signals
  assign vcd_trigger = u_tb_system.u_crheepto_top.gpio_0_io;

  // VCD dump FSM
  // ------------
  // FSM state progression
  always_comb begin : fsm_state_prog
    // Default values
    vcd_cnt_en     = 1'b0;
    vcd_filename_d = "";

    // State progression
    case (curr_state)
      IDLE: begin
        case (vcd_mode_opt)
          VCD_MODE_ON: next_state = DUMP_INIT;  // unconditional dump
          VCD_MODE_TRIG: begin  // triggered dump
            if (vcd_trigger) begin
              next_state = DUMP_INIT;
            end else begin
              next_state = IDLE;
            end
          end
          default:     next_state = IDLE;  // no dump
        endcase
      end
      DUMP_INIT: begin
        next_state     = WAIT;
        // Increment VCD sequence counter
        vcd_cnt_en     = 1'b1;
        // Create VCD file and start dumping
        vcd_filename_d = {VCD_DIR, VCD_BASENAME, $sformatf("%0d", vcd_cnt), ".vcd"};
        // NOTE: the following system tasks are specific to QuestaSim and not
        // part of the Verilog IEEE1364 standard, so may be not portable to
        // other simulators
        $fdumpfile(vcd_filename_d);
        $fdumpvars(0, u_tb_system.u_crheepto_top.u_core_v_mini_mcu.cpu_subsystem_i,
                      u_tb_system.u_crheepto_top.u_core_v_mini_mcu.memory_subsystem_i,
                      u_tb_system.u_crheepto_top.u_core_v_mini_mcu.system_bus_i,
                      u_tb_system.u_crheepto_top.coproc_wrapper_i, vcd_filename_d);
        $display("[%t] VCD file initialized: %s", $time, vcd_filename_d);
        $display("[%t] VCD dump ON", $time);
      end
      WAIT: begin
        case (vcd_mode_opt)
          VCD_MODE_ON: begin  // unconditional dump
            next_state = WAIT;
          end
          VCD_MODE_TRIG: begin  // triggered dump
            if (vcd_trigger) begin
              next_state = WAIT;
            end else begin
              next_state = DUMP_OFF;
            end
          end
          default: next_state = IDLE;  // no dump
        endcase
      end
      DUMP_OFF: begin
        next_state = IDLE;
        $fdumpoff(vcd_filename_q);
        $display("[%t] VCD dump OFF", $time);
      end
      default: next_state = IDLE;
    endcase
  end

  // FSM state register
  always_ff @(posedge sim_clk or negedge sys_rst_n) begin : fsm_state_reg
    if (!sys_rst_n) curr_state <= IDLE;
    else curr_state <= next_state;
  end

  // VCD sequence counter
  always_ff @(posedge sim_clk or negedge sys_rst_n) begin : vcd_cnt_reg
    if (!sys_rst_n) vcd_cnt <= 0;
    else if (vcd_cnt_en) vcd_cnt <= vcd_cnt + 1;
  end

  // VCD filename register
  always_ff @(posedge sim_clk) begin : vcd_filename_reg
    if (vcd_cnt_en) vcd_filename_q <= vcd_filename_d;
  end

  // --------------------
  // TESTBENCH COMPONENTS
  // --------------------

  // Clock & reset generator
  // -----------------------

  initial begin : sim_clk_gen
    sim_clk = 1'b1;
    forever begin
      #SIM_CLK_PHASE_HI sim_clk = 1'b0;
      #SIM_CLK_PHASE_LO sim_clk = 1'b1;
    end
  end
  initial begin : rst_gen
    tb_rst_n = 1'b1;

    // Wait a few cycles
    repeat (ResetWaitCycles) @(posedge sim_clk);

    tb_rst_n = 1'b0;

    // Wait a few cycles
    repeat (ResetWaitCycles) @(posedge sim_clk);

    // Release reset
    #RESET_DEL tb_rst_n = 1'b1;
    $display("[%t] Reset released", $time);

    #RESET_DEL -> rst_event;

  end

  // System clock and reset
  // ----------------------
  assign sys_rst_n = u_tb_system.u_crheepto_top.u_rstgen.rst_no;

  // TB Monitor
  // ----------
  // Watchdog counter
  always_ff @(posedge sim_clk or negedge tb_rst_n) begin : watchdog_cnt
    if (max_cycles != 0) begin
      if (~tb_rst_n) begin
        cycle_cnt_q <= 0;
      end else begin
        cycle_cnt_q <= cycle_cnt_q + 1;
        if (cycle_cnt_q >= max_cycles) begin
          $dumpoff;
          $fatal(2, "Simulation aborted due to maximum cycle limit");
        end else if (cycle_cnt_q % WatchdogHeartbeatCycles == 0) begin
          $display("[%t] Watchdog heartbeat: %0d cycles", $time, cycle_cnt_q);
        end
      end
    end
  end

  // Exit monitor
  always_ff @(posedge sim_clk or negedge sys_rst_n) begin : exit_monitor
    if (exit_valid) begin
      if (exit_value == 0) begin
        $display("[%t] TEST SUCCEEDED", $time);
      end else begin
        $display("[%t] TEST FAILED", $time);
      end
      $display("[%t] - return value: %0d", $time, $signed(exit_value));
      $dumpoff;
      if (exit_value == 0) begin
        $finish;
      end else begin
        $fatal(2, "[%t] TEST FAILED with value: %0d", $time, exit_value);
      end
    end
  end

  // Testbench system
  // ----------------
  // Set static onfiguration
  assign boot_select     = boot_mode == BOOT_MODE_FLASH;
  assign exec_from_flash = exec_from_flash_opt != 0;

  // Clock and reset
  assign sim_clk_w       = sim_clk;
  assign tb_rst_nw       = tb_rst_n;

  // Instantiate TB system
  tb_system #(
      .CLK_FREQ(ClkFrequencykHz)
  ) u_tb_system (
      .clk_i               (sim_clk_w),
      .rst_ni              (tb_rst_nw),
      .boot_select_i       (boot_select),
      .execute_from_flash_i(exec_from_flash),
      .exit_valid_o        (exit_valid),
      .exit_value_o        (exit_value)
  );
endmodule
