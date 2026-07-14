module cbd_eta (
    input  logic [31:0] d_i,    
    input  logic [31:0] j_i,    
    input  horcrux_pkg::horcrux_insn insn_i,
    output logic signed [31:0] cbd_o  // Ensure signed 8-bit output
);

    // ------------- Decode η with backward-compatible aliases -------------
    logic [2:0] eta;
    always_comb begin
        unique case (insn_i)
            horcrux_pkg::OP_CBD1: eta = 3'd1;
            horcrux_pkg::OP_CBD2: eta = 3'd2;
            horcrux_pkg::OP_CBD3: eta = 3'd3;
            horcrux_pkg::OP_CBD4: eta = 3'd4;
            default:               eta = 3'd0; // safe default for your tests
        endcase
    end

    logic [5:0] stride;
    logic [2:0] eta_local;
    logic [31:0] mask;
    logic [31:0] sh_a, sh_b;
    logic [31:0] a, b;
    logic signed [7:0] diff;
    logic [4:0] j_masked;

    always_comb begin
        unique case (insn_i)
            horcrux_pkg::OP_CBD1, horcrux_pkg::OP_CBD2, horcrux_pkg::OP_CBD3, horcrux_pkg::OP_CBD4: begin
                // Extract eta
                eta_local = eta[2:0];
                j_masked  = j_i[4:0];    // Mask to ensure we are within lane range
                
                stride    = {eta_local, 1'b0}; // Stride = 2*eta (shift left by 1)
                mask      = (32'h1 << eta_local) - 32'h1;  // Mask based on eta (2^eta - 1)
                
                // Calculate shifts for a and b
                sh_a = j_masked * stride;
                sh_b = sh_a + eta_local;
                
                // Extract lane values a and b
                a     = (d_i >> sh_a) & mask;
                b     = (d_i >> sh_b) & mask;
                
                // Perform signed subtraction
                diff  = $signed(a[7:0]) - $signed(b[7:0]);
                
                //cbd_o = {24'b0, diff}; 
                cbd_o = $signed(diff);
            end
            default: begin
                // Default case, ensure clean output for invalid instructions
                eta_local = 3'd0;
                j_masked  = 5'd0;
                stride    = 6'd0;
                mask      = 32'd0;
                sh_a      = 32'd0;
                sh_b      = 32'd0;
                a         = 32'd0;
                b         = 32'd0;
                diff      = 9'd0;
                cbd_o     = 32'd0; // Default zero
            end
        endcase
    end
endmodule
