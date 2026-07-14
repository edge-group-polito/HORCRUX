// Copyright 2025 PoliTO - EDGE Group, @VLSI Lab                                       //
// Solderpad Hardware License, Version 2.1, see LICENSE.md for details.                //
// SPDX-License-Identifier: Apache-2.0 WITH SHL-2.1                                    //
//                                                                                      //
// Authors:      Alessandra Dolmeta - alessandra.dolmeta@polito.it                      //
//               Valeria Piscopo    - valeria.piscopo@polito.it                         //
// Design Name:  HORCRUX ISA Package                                                    //
// Language:     SystemVerilog                                                          //
// Date:         April 2026                                                            //
//                                                                                      //
// Description:  HORCRUX instruction set encoding, types, and instruction formats for   //
//               post-quantum cryptography acceleration (ML-KEM, ML-DSA, Falcon, HQC).  //
///////////////////////////////////////////////////////////////////////////////////////////

package horcrux_pkg;



parameter int unsigned NbInstr = 57; 


// Opcode of the new instruction
typedef enum logic [6:0] {
  none                = 7'b0000000,  // funct7=0x00: Reserved (no operation)
  OP_KSTART           = 7'b0000001,  // funct7=0x01: Start Keccak permutation
  OP_STORE            = 7'b0000010,  // funct7=0x02: Store result to register file
  OP_LOAD             = 7'b0000011,  // funct7=0x03: Load from register file
  OP_KABSORB          = 7'b0000100,  // funct7=0x04: Absorb (XOR) into Keccak state
  OP_KPERM            = 7'b0000101,  // funct7=0x05: In-place Keccak permutation
  OP_INIT             = 7'b0000110,  // funct7=0x06: Initialize/zero register file
  OP_KREAD3           = 7'b0101101,  // funct7=0x2D: Read 3 bytes from Keccak state
  OP_CBD1             = 7'b0001100,  // funct7=0x0C: CBD sampling n=256
  OP_CBD2             = 7'b0001101,  // funct7=0x0D: CBD sampling n=512
  OP_CBD3             = 7'b0001110,  // funct7=0x0E: CBD sampling n=768
  OP_CBD4             = 7'b0001111,  // funct7=0x0F: CBD sampling n=1024
  OP_BARRETT          = 7'b0010001,  // funct7=0x11: Barrett reduction for ML-KEM (Z₃₃₂₉)
  OP_BARRETT_HQC      = 7'b0011010,  // funct7=0x1A: Barrett reduction for HQC-1
  OP_BARRETT_HQC3     = 7'b0111010,  // funct7=0x3A: Barrett reduction for HQC-3
  OP_BARRETT_HQC5     = 7'b0111011,  // funct7=0x3B: Barrett reduction for HQC-5
  OP_BFNTTK           = 7'b0011011,  // funct7=0x1B: Cooley-Tukey NTT butterfly for ML-KEM
  OP_BFINTTK          = 7'b0011100,  // funct7=0x1C: Inverse NTT butterfly for ML-KEM
  OP_MQMULF           = 7'b0011101,  // funct7=0x1D: Montgomery multiply for Falcon
  OP_MQMULK           = 7'b0011110,  // funct7=0x1E: Montgomery multiply for ML-KEM
  OP_MQMULD           = 7'b0011111,  // funct7=0x1F: Montgomery multiply for ML-DSA
  OP_KARATS_1         = 7'b0100000,  // funct7=0x20: Karatsuba multiply step 1
  OP_KARATS_2         = 7'b0100001,  // funct7=0x21: Karatsuba multiply step 2
  OP_KARATS_3         = 7'b0100010,  // funct7=0x22: Karatsuba multiply step 3
  OP_GFMUL8           = 7'b0100011,  // funct7=0x23: GF(2⁸) multiplication mod 0x11D
  OP_BFNTTD           = 7'b0100100,  // funct7=0x24: Gentleman-Sande NTT butterfly for ML-DSA
  OP_BFNTTDH          = 7'b0100101,  // funct7=0x25: NTT butterfly with high word output
  OP_BFINTTD          = 7'b0100110,  // funct7=0x26: Inverse NTT butterfly for ML-DSA
  OP_BFINTTDH         = 7'b0100111,  // funct7=0x27: Inverse NTT butterfly with high output
  OP_KARATS_4         = 7'b0101000,  // funct7=0x28: Karatsuba multiply step 4
  OP_RED32            = 7'b0101001,  // funct7=0x29: 32-bit modular reduction
  OP_CL_128F          = 7'b0101010,  // funct7=0x2A: SPHINCS+-128f chain lengths
  OP_CL_192F          = 7'b0101011,  // funct7=0x2B: SPHINCS+-192f chain lengths
  OP_CL_256F          = 7'b0101100,  // funct7=0x2C: SPHINCS+-256f chain lengths
  OP_BFNTTF           = 7'b0101110,  // funct7=0x2E: NTT butterfly for Falcon
  OP_BFINTTF          = 7'b0101111,  // funct7=0x2F: Inverse NTT butterfly for Falcon
  OP_THASH1           = 7'b0110000,  // funct7=0x30: SPHINCS+ thash for 1-block input
  OP_THASH2           = 7'b0110001,  // funct7=0x31: SPHINCS+ thash for 2-block input
  OP_PRF_ADDR         = 7'b0110010,  // funct7=0x32: SPHINCS+ PRF with address (FIPS 205)
  OP_MODP_MONTYMUL    = 7'b0110011,  // funct7=0x33: Montgomery multiply mod P
  OP_FPR_LOAD_SE      = 7'b0110100,  // funct7=0x34: Falcon FPR load sign-extended
  OP_FPR_EXEC         = 7'b0110101,  // funct7=0x35: Falcon FPR execute operation
  OP_FPR_RDHI         = 7'b0110110,  // funct7=0x36: Falcon FPR read high word
  OP_FPR_NORM64_EXEC  = 7'b0110111,  // funct7=0x37: Falcon FPR normalize 64-bit
  OP_FPR_NORM64_RDHI  = 7'b0111000,  // funct7=0x38: Falcon FPR normalize read high
  OP_FPR_NORM64_RDE   = 7'b0111001,  // funct7=0x39: Falcon FPR normalize read extended
  OP_GF_REDUCE        = 7'b1111000,  // funct7=0x78: GF(2⁸) reduce mod 0x11D
  OP_COMPARE_U32      = 7'b1111001,  // funct7=0x79: Constant-time u32 comparison
  OP_REJ_UNIFORM      = 7'b1001000,  // funct7=0x48: Rejection sampling mod Q for matrix A
  OP_REJ_ETA2         = 7'b1001001,  // funct7=0x49: Nibble rejection for eta=2 ([-2, +2])
  OP_REJ_ETA4         = 7'b1001010,  // funct7=0x4A: Nibble rejection for eta=4 ([-4, +4])
  OP_UNPACK_Z         = 7'b1001011,  // funct7=0x4B: Unpack gamma1-range coefficients
  OP_THASH1_192       = 7'b0111100,  // funct7=0x3C: SPHINCS+-192 thash for 1-block input
  OP_THASH2_192       = 7'b0111101,  // funct7=0x3D: SPHINCS+-192 thash for 2-block input
  OP_THASH1_256       = 7'b0111110,  // funct7=0x3E: SPHINCS+-256 thash for 1-block input
  OP_THASH2_256       = 7'b0111111,  // funct7=0x3F: SPHINCS+-256 thash for 2-block input
  OP_PRF_192          = 7'b1000000,  // funct7=0x40: SPHINCS+-192 PRF with address
  OP_PRF_256          = 7'b1000001   // funct7=0x41: SPHINCS+-256 PRF with address
} horcrux_insn;

  typedef enum logic [6:0] {
    ILLEGAL         = 7'b0000000,
    KECCAK          = 7'b0000001,
    REDUCTION       = 7'b0000010,
    GF              = 7'b0000100,
    LOAD_STORE      = 7'b0000101,
    MULTIPLIER_TREE = 7'b0000110,
    CBD             = 7'b0000111,
    CHAIN_LENGTHS   = 7'b0001000,
    THASH           = 7'b0001001,
    SAMPLER         = 7'b0001010 
  } opcode_t;


  typedef struct packed {
    logic accept;
    logic writeback;
    logic [2:0] register_read;
    horcrux_insn insn;
    logic done;
  } issue_resp_t;

  typedef struct packed {
    logic [31:0] instr;
    logic [31:0] mask;
    issue_resp_t resp;
    opcode_t     opcode;
  } copro_issue_resp_t;


  parameter copro_issue_resp_t CoproInstr[NbInstr] = '{

      '{
          instr: 32'b00000_11_00000_00000_001_00000_1101011,  // LOAD
          mask: 32'b00000_11_00000_00000_111_00000_1111111,
          resp : '{
              accept : 1'b1,
              writeback : 1'b0,
              register_read : {1'b1, 1'b1, 1'b1},
              insn: OP_LOAD,
              done : 1'b1
          },
          opcode : LOAD_STORE
      },
      //KECCAK instructions
      '{
          instr: 32'b00000_00_00000_00000_001_00000_1101011,  // KECCAK opcode - start
          mask: 32'b00000_11_00000_00000_111_00000_1111111,
          resp : '{
              accept : 1'b1,
              writeback : 1'b0,
              register_read : {1'b1, 1'b1, 1'b1},
              insn: OP_KSTART,
              done : 1'b1
          },
          opcode : KECCAK
      },
      '{
        instr: 32'b001111_00000_00000_111_00000_0111011,  // STORE
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp
          : '{
              accept : 1'b1,
              writeback : 1'b1,
              register_read : {1'b1, 1'b1, 1'b0},
              insn: OP_STORE,
              done : 1'b1
          },
        opcode : LOAD_STORE
    },

      // OP_KABSORB: XOR data into register file (funct2=01, funct3=001, opcode=0x6b)
      '{
          instr: 32'b00000_01_00000_00000_001_00000_1101011,
          mask:  32'b00000_11_00000_00000_111_00000_1111111,
          resp : '{
              accept : 1'b1,
              writeback : 1'b0,
              register_read : {1'b1, 1'b1, 1'b1},
              insn: OP_KABSORB,
              done : 1'b1
          },
          opcode : KECCAK
      },

      // OP_KPERM: in-place permutation with writeback (funct2=10, funct3=001, opcode=0x6b)
      '{
          instr: 32'b00000_10_00000_00000_001_00000_1101011,
          mask:  32'b00000_11_00000_00000_111_00000_1111111,
          resp : '{
              accept : 1'b1,
              writeback : 1'b0,
              register_read : {1'b1, 1'b1, 1'b1},
              insn: OP_KPERM,
              done : 1'b1
          },
          opcode : KECCAK
      },

      // OP_INIT: zero the register file (funct7=0x0E, funct3=111, opcode=0x3b)
      '{
          instr: 32'b0001110_00000_00000_111_00000_0111011,
          mask:  32'b1111111_00000_00000_111_00000_1111111,
          resp : '{
              accept : 1'b1,
              writeback : 1'b0,
              register_read : {1'b0, 1'b0, 1'b0},
              insn: OP_INIT,
              done : 1'b1
          },
          opcode : KECCAK
      },


    //CBD instructions
    '{ 
        instr: 32'b0000000_00000_00000_111_00000_0111011,  //
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,
            register_read : {1'b1, 1'b1, 1'b0},
            insn: OP_CBD1,
            done : 1'b1
        },
        opcode : CBD
    }, 
    // CBD2 - RESTORED
    '{ 
        instr: 32'b0000001_00000_00000_111_00000_0111011,  //
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,
            register_read : {1'b1, 1'b1, 1'b0},
            insn: OP_CBD2,
            done : 1'b1
        },
        opcode : CBD
    }, 
    '{ 
        instr: 32'b0000010_00000_00000_111_00000_0111011,  // 
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,           
            register_read : {1'b1, 1'b1, 1'b0},
            insn: OP_CBD3,
            done : 1'b1
        },
        opcode : CBD
    }, 
    '{ 
        instr: 32'b0000011_00000_00000_111_00000_0111011,  // 
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,           
            register_read : {1'b1, 1'b1, 1'b0},
            insn: OP_CBD4,
            done : 1'b1
        },
        opcode : CBD
    },

    '{ 
        instr: 32'b0000101_00000_00000_111_00000_0111011,  // BARRETT KYBER
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,           
            register_read : {1'b1, 1'b1, 1'b0},
            insn: OP_BARRETT,
            done : 1'b1
        },
        opcode : REDUCTION
    },

      '{ 
        instr: 32'b1011000_00000_00000_111_00000_0111011,  // BARRETT HQC (funct7=0x58)
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
          accept : 1'b1,
          writeback : 1'b1,
          register_read : {1'b0, 1'b0, 1'b1},
          insn: OP_BARRETT_HQC,
          done : 1'b1
        },
        opcode : REDUCTION
      },

      '{ 
        instr: 32'b1011001_00000_00000_111_00000_0111011,  // BARRETT HQC-3 (funct7=0x59)
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
          accept : 1'b1,
          writeback : 1'b1,
          register_read : {1'b0, 1'b0, 1'b1},
          insn: OP_BARRETT_HQC3,
          done : 1'b1
        },
        opcode : REDUCTION
      },

      '{ 
        instr: 32'b1011010_00000_00000_111_00000_0111011,  // BARRETT HQC-5 (funct7=0x5A)
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
          accept : 1'b1,
          writeback : 1'b1,
          register_read : {1'b0, 1'b0, 1'b1},
          insn: OP_BARRETT_HQC5,
          done : 1'b1
        },
        opcode : REDUCTION
      },

    //MULTIPLIER TREE instructions
    '{
          instr: 32'b00000_00_00000_00000_111_00000_1101011,  // OP_BFNTTK
          mask:  32'b00000_11_00000_00000_111_00000_1111111,
          resp : '{
              accept : 1'b1,
              writeback : 1'b1,
              register_read : {1'b1, 1'b1, 1'b1},
              insn: OP_BFNTTK,
              done : 1'b1
          },
          opcode : MULTIPLIER_TREE
      },
      '{
          instr: 32'b00000_01_00000_00000_111_00000_1101011,  // OP_BFINTTK
          mask:  32'b00000_11_00000_00000_111_00000_1111111,
          resp : '{
              accept : 1'b1,
              writeback : 1'b1,
              register_read : {1'b1, 1'b1, 1'b1},
              insn: OP_BFINTTK,
              done : 1'b1
          },
          opcode : MULTIPLIER_TREE
      },
      '{
          instr: 32'b00000_10_00000_00000_111_00000_1101011,  // OP_BFNTTD
          mask:  32'b00000_11_00000_00000_111_00000_1111111,
          resp : '{
              accept : 1'b1,
              writeback : 1'b1,
              register_read : {1'b1, 1'b1, 1'b1},
              insn: OP_BFNTTD,
              done : 1'b1
          },
          opcode : MULTIPLIER_TREE
      },
      '{
          instr: 32'b00000_11_00000_00000_111_00000_1101011,  // OP_BFINTTD
          mask:  32'b00000_11_00000_00000_111_00000_1111111,
          resp : '{
              accept : 1'b1,
              writeback : 1'b1,
              register_read : {1'b1, 1'b1, 1'b1},
              insn: OP_BFINTTD,
              done : 1'b1
          },
          opcode : MULTIPLIER_TREE
      },
    '{
        instr: 32'b00000_00_00000_00000_110_00000_1101011,  // OP_BFNTTF
        mask:  32'b00000_11_00000_00000_111_00000_1111111,
        resp : '{
        accept : 1'b1,
        writeback : 1'b1,
        register_read : {1'b1, 1'b1, 1'b1},
        insn: OP_BFNTTF,
        done : 1'b1
        },
        opcode : MULTIPLIER_TREE
    },
    '{
        instr: 32'b00000_01_00000_00000_110_00000_1101011,  // OP_BFINTTF
        mask:  32'b00000_11_00000_00000_111_00000_1111111,
        resp : '{
        accept : 1'b1,
        writeback : 1'b1,
        register_read : {1'b1, 1'b1, 1'b1},
        insn: OP_BFINTTF,
        done : 1'b1
        },
        opcode : MULTIPLIER_TREE
    },
      '{ 
        instr: 32'b0010000_00000_00000_111_00000_0111011,  // OP_MQMULF
        mask:  32'b1111111_00000_00000_111_00000_1111111, 
        resp
          : '{
              accept : 1'b1,    
              writeback : 1'b1,       
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_MQMULF,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE
    },
    '{ 
        instr: 32'b0010001_00000_00000_111_00000_0111011,  // OP_MQMULK
        mask:  32'b1111111_00000_00000_111_00000_1111111, 
        resp
          : '{
              accept : 1'b1,    
              writeback : 1'b1,       
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_MQMULK,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE
    },
    '{ 
        instr: 32'b0010010_00000_00000_111_00000_0111011,  // OP_MQMULD
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp
          : '{
              accept : 1'b1,    
              writeback : 1'b1,       
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_MQMULD,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE
    },
    '{ 
        instr: 32'b0010011_00000_00000_111_00000_0111011,  // OP_KARATS_1
        mask:  32'b1111111_00000_00000_111_00000_1111111, 
        resp
          : '{
              accept : 1'b1,    
              writeback : 1'b1,       
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_KARATS_1,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE
    },
    '{ 
        instr: 32'b0010100_00000_00000_111_00000_0111011,  // OP_KARATS_2
        mask:  32'b1111111_00000_00000_111_00000_1111111, 
        resp
          : '{
              accept : 1'b1,  
              writeback : 1'b1,     
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_KARATS_2,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE
    },
    '{ 
        instr: 32'b0010101_00000_00000_111_00000_0111011,  // OP_KARATS_3
        mask:  32'b1111111_00000_00000_111_00000_1111111, 
        resp
          : '{
              accept : 1'b1,    
              writeback : 1'b1,       
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_KARATS_3,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE
    },
    '{  
        instr: 32'b0010110_00000_00000_111_00000_0111011,  // OP_GFMUL8
        mask:  32'b1111111_00000_00000_111_00000_1111111, 
        resp
          : '{
              accept : 1'b1,    
              writeback : 1'b1,       
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_GFMUL8,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE
    },
    '{
        instr: 32'b0010111_00000_00000_111_00000_0111011,  // OP_BFNTTDH
        mask:  32'b1111111_00000_00000_111_00000_1111111, 
        resp
          : '{
              accept : 1'b1,    
              writeback : 1'b1,       
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_BFNTTDH,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE   
    },
    '{
        instr: 32'b0011000_00000_00000_111_00000_0111011,  // OP_BFINTTDH
        mask:  32'b1111111_00000_00000_111_00000_1111111, 
        resp
          : '{
              accept : 1'b1,    
              writeback : 1'b1,       
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_BFINTTDH,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE   
    },
    '{ 
        instr: 32'b0011001_00000_00000_111_00000_0111011,  // OP_KARATS_4
        mask:  32'b1111111_00000_00000_111_00000_1111111, 
        resp
          : '{
              accept : 1'b1,    
              writeback : 1'b1,       
              register_read : {1'b1, 1'b1, 1'b0},    
              insn: OP_KARATS_4,
              done : 1'b1 
          },  
        opcode : MULTIPLIER_TREE   
    },
        '{
            instr: 32'b0011010_00000_00000_111_00000_0111011,  // OP_RED32
          mask:  32'b1111111_00000_00000_111_00000_1111111,
          resp
            : '{
                accept : 1'b1,
                writeback : 1'b1,
                register_read : {1'b1, 1'b1, 1'b0},
                insn: OP_RED32,
                done : 1'b1
            },
          opcode : MULTIPLIER_TREE
      },

        '{
              instr: 32'b00000_10_00000_00000_110_00000_1101011,  // OP_MODP_MONTYMUL
              mask:  32'b00000_11_00000_00000_111_00000_1111111,
              resp
            : '{
              accept : 1'b1,
              writeback : 1'b1,
              register_read : {1'b1, 1'b1, 1'b1},
              insn: OP_MODP_MONTYMUL,
              done : 1'b1
            },
              opcode : MULTIPLIER_TREE
            },

        '{
            instr: 32'b0110011_00000_00000_111_00000_0111011,  // OP_FPR_LOAD_SE
            mask:  32'b1111111_00000_00000_111_00000_1111111,
            resp
              : '{
                  accept : 1'b1,
              writeback : 1'b1,
                  register_read : {1'b1, 1'b1, 1'b0},
                  insn: OP_FPR_LOAD_SE,
                  done : 1'b1
              },
            opcode : MULTIPLIER_TREE
        },

        '{
            instr: 32'b0110100_00000_00000_111_00000_0111011,  // OP_FPR_EXEC
            mask:  32'b1111111_00000_00000_111_00000_1111111,
            resp
              : '{
                  accept : 1'b1,
                  writeback : 1'b1,
                  register_read : {1'b1, 1'b1, 1'b0},
                  insn: OP_FPR_EXEC,
                  done : 1'b1
              },
            opcode : MULTIPLIER_TREE
        },

        '{
            instr: 32'b0110101_00000_00000_111_00000_0111011,  // OP_FPR_RDHI
            mask:  32'b1111111_00000_00000_111_00000_1111111,
            resp
              : '{
                  accept : 1'b1,
                  writeback : 1'b1,
                  register_read : {1'b0, 1'b0, 1'b0},
                  insn: OP_FPR_RDHI,
                  done : 1'b1
              },
            opcode : MULTIPLIER_TREE
        },

          '{
            instr: 32'b00000_11_00000_00000_110_00000_1101011,  // OP_FPR_NORM64_EXEC
            mask:  32'b00000_11_00000_00000_111_00000_1111111,
            resp
              : '{
                accept : 1'b1,
                writeback : 1'b1,
                register_read : {1'b1, 1'b1, 1'b1},
                insn: OP_FPR_NORM64_EXEC,
                done : 1'b1
              },
            opcode : MULTIPLIER_TREE
          },

          '{
            instr: 32'b0110110_00000_00000_111_00000_0111011,  // OP_FPR_NORM64_RDHI
            mask:  32'b1111111_00000_00000_111_00000_1111111,
            resp
              : '{
                accept : 1'b1,
                writeback : 1'b1,
                register_read : {1'b0, 1'b0, 1'b0},
                insn: OP_FPR_NORM64_RDHI,
                done : 1'b1
              },
            opcode : MULTIPLIER_TREE
          },

          '{
            instr: 32'b0110111_00000_00000_111_00000_0111011,  // OP_FPR_NORM64_RDE
            mask:  32'b1111111_00000_00000_111_00000_1111111,
            resp
              : '{
                accept : 1'b1,
                writeback : 1'b1,
                register_read : {1'b0, 1'b0, 1'b0},
                insn: OP_FPR_NORM64_RDE,
                done : 1'b1
              },
            opcode : MULTIPLIER_TREE
          },

    //CHAIN_LENGTHS instructions (SPHINCS+ WOTS+)
    '{
        instr: 32'b0011011_00000_00000_111_00000_0111011,  // OP_CL_128F (funct7=0x1B)
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b0, 1'b0},
            insn: OP_CL_128F,
            done : 1'b1
        },
        opcode : CHAIN_LENGTHS
    },
    '{
        instr: 32'b0011100_00000_00000_111_00000_0111011,  // OP_CL_192F (funct7=0x1C)
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b0, 1'b0},
            insn: OP_CL_192F,
            done : 1'b1
        },
        opcode : CHAIN_LENGTHS
    },
    '{
        instr: 32'b0011101_00000_00000_111_00000_0111011,  // OP_CL_256F (funct7=0x1D)
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b0, 1'b0},
            insn: OP_CL_256F,
            done : 1'b1
        },
        opcode : CHAIN_LENGTHS
    },

    // OP_KREAD3: read 3 consecutive bytes from register file (funct7=0x0D, funct3=111, opcode=0x3b)
    // Input: rs1 = byte offset into Keccak register file
    // Output: rd = {8'b0, byte2, byte1, byte0} (24 bits, zero-extended)
    '{
        instr: 32'b0001101_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,
            register_read : {1'b1, 1'b0, 1'b0},
            insn: OP_KREAD3,
            done : 1'b1
        },
        opcode : LOAD_STORE
    },

    // OP_THASH1: SPHINCS+-128 thash for 1-block input (funct7=0x30, funct3=111, opcode=0x3b)
    // rs2 value: 0=robust, non-zero=simple
    // Precondition: reg[0:3]=pub_seed, reg[4:11]=addr, reg[12:15]=input
    // Postcondition: reg[0:3]=output hash
    '{
        instr: 32'b0110000_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b1, 1'b0},
            insn: OP_THASH1,
            done : 1'b0
        },
        opcode : THASH
    },

    // OP_THASH2: SPHINCS+-128 thash for 2-block input (funct7=0x31, funct3=111, opcode=0x3b)
    // rs2 value: 0=robust, non-zero=simple
    // Precondition: reg[0:3]=pub_seed, reg[4:11]=addr, reg[12:19]=input (32 bytes)
    // Postcondition: reg[0:3]=output hash
    '{
        instr: 32'b0110001_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b1, 1'b0},
            insn: OP_THASH2,
            done : 1'b0
        },
        opcode : THASH
    },

    // OP_PRF_ADDR: SPHINCS+-128 PRF with address (funct7=0x32, funct3=111, opcode=0x3b)
    // FIPS 205: PRF = SHAKE256(pub_seed || addr || sk_seed)
    // Precondition: reg[0:3]=pub_seed, reg[4:11]=addr, reg[12:15]=sk_seed
    // Postcondition: reg[0:3]=PRF output (16 bytes)
    '{
        instr: 32'b0110010_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b0, 1'b0},
            insn: OP_PRF_ADDR,
            done : 1'b0
        },
        opcode : THASH
    },

    // OP_THASH1_192: SPHINCS+-192 thash for 1-block input (funct7=0x3C)
    // rs2 value: 0=robust, non-zero=simple
    // Precondition: reg[0:5]=pub_seed, reg[6:13]=addr, reg[14:19]=input
    // Postcondition: reg[0:5]=output hash (24 bytes)
    '{
        instr: 32'b0111100_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b1, 1'b0},
            insn: OP_THASH1_192,
            done : 1'b0
        },
        opcode : THASH
    },

    // OP_THASH2_192: SPHINCS+-192 thash for 2-block input (funct7=0x3D)
    // rs2 value: 0=robust, non-zero=simple
    // Precondition: reg[0:5]=pub_seed, reg[6:13]=addr, reg[14:25]=input (48 bytes)
    // Postcondition: reg[0:5]=output hash (24 bytes)
    '{
        instr: 32'b0111101_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b1, 1'b0},
            insn: OP_THASH2_192,
            done : 1'b0
        },
        opcode : THASH
    },

    // OP_THASH1_256: SPHINCS+-256 thash for 1-block input (funct7=0x3E)
    // rs2 value: 0=robust, non-zero=simple
    // Precondition: reg[0:7]=pub_seed, reg[8:15]=addr, reg[16:23]=input
    // Postcondition: reg[0:7]=output hash (32 bytes)
    '{
        instr: 32'b0111110_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b1, 1'b0},
            insn: OP_THASH1_256,
            done : 1'b0
        },
        opcode : THASH
    },

    // OP_THASH2_256: SPHINCS+-256 thash for 2-block input (funct7=0x3F)
    // rs2 value: 0=robust, non-zero=simple
    // Precondition: reg[0:7]=pub_seed, reg[8:15]=addr, reg[16:31]=input (64 bytes)
    // Postcondition: reg[0:7]=output hash (32 bytes)
    '{
        instr: 32'b0111111_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b1, 1'b0},
            insn: OP_THASH2_256,
            done : 1'b0
        },
        opcode : THASH
    },

    // OP_PRF_192: SPHINCS+-192 PRF with address (funct7=0x40)
    // Precondition: reg[0:5]=pub_seed, reg[6:13]=addr, reg[14:19]=sk_seed
    // Postcondition: reg[0:5]=PRF output (24 bytes)
    '{
        instr: 32'b1000000_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b0, 1'b0},
            insn: OP_PRF_192,
            done : 1'b0
        },
        opcode : THASH
    },

    // OP_PRF_256: SPHINCS+-256 PRF with address (funct7=0x41)
    // Precondition: reg[0:7]=pub_seed, reg[8:15]=addr, reg[16:23]=sk_seed
    // Postcondition: reg[0:7]=PRF output (32 bytes)
    '{
        instr: 32'b1000001_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b0,
            register_read : {1'b0, 1'b0, 1'b0},
            insn: OP_PRF_256,
            done : 1'b0
        },
        opcode : THASH
    },

    // OP_GF_REDUCE: Full GF(2^8) reduction mod 0x11D (funct7=0x78, funct3=111, opcode=0x3b)
    // rd = gf_reduce(rs1) - reduces 16-bit input to 8-bit result
    '{
        instr: 32'b1111000_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,
            register_read : {1'b0, 1'b0, 1'b1},
            insn: OP_GF_REDUCE,
            done : 1'b1
        },
        opcode : MULTIPLIER_TREE
    },

    // OP_COMPARE_U32: Constant-time comparison (funct7=0x79, funct3=111, opcode=0x3b)
    // rd = (rs1 == rs2) ? 1 : 0 using: 1 ^ (((v1-v2) | (v2-v1)) >> 31)
    '{
        instr: 32'b1111001_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,
            register_read : {1'b0, 1'b1, 1'b1},
            insn: OP_COMPARE_U32,
            done : 1'b1
        },
        opcode : MULTIPLIER_TREE
    },

    //==========================================================================
    // ML-DSA SAMPLER INSTRUCTIONS
    //==========================================================================

    // OP_REJ_UNIFORM: Rejection sampling for matrix A (funct7=0x48, funct3=111, opcode=0x3b)
    // Input:  rs1 = {8'bxx, byte2, byte1, byte0} (24 bits from SHAKE stream)
    // Output: rd  = {valid[31], 8'b0, coeff[22:0]}
    //         valid=1 if coeff < Q (8380417), else 0
    '{
        instr: 32'b1001000_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,
            register_read : {1'b0, 1'b0, 1'b1},  // reads rs1 only
            insn: OP_REJ_UNIFORM,
            done : 1'b1
        },
        opcode : SAMPLER
    },

    // OP_REJ_ETA2: Nibble rejection for eta=2 (funct7=0x49, funct3=111, opcode=0x3b)
    // Input:  rs1 = byte with two 4-bit nibbles
    //         rs2[0] = nibble selector (0=low, 1=high)
    // Output: rd = {valid[31], sign_extended_coeff[30:0]} in [-2, +2]
    '{
        instr: 32'b1001001_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,
            register_read : {1'b0, 1'b1, 1'b1},  // reads rs1 and rs2
            insn: OP_REJ_ETA2,
            done : 1'b1
        },
        opcode : SAMPLER
    },

    // OP_REJ_ETA4: Nibble rejection for eta=4 (funct7=0x4A, funct3=111, opcode=0x3b)
    // Input:  rs1 = byte with two 4-bit nibbles
    //         rs2[0] = nibble selector (0=low, 1=high)
    // Output: rd = {valid[31], sign_extended_coeff[30:0]} in [-4, +4]
    '{
        instr: 32'b1001010_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,
            register_read : {1'b0, 1'b1, 1'b1},  // reads rs1 and rs2
            insn: OP_REJ_ETA4,
            done : 1'b1
        },
        opcode : SAMPLER
    },

    // OP_UNPACK_Z: Gamma1-range coefficient unpacking (funct7=0x4B, funct3=111, opcode=0x3b)
    // Input:  rs1 = 32-bit packed data
    //         rs2[1:0] = extraction selector (0-3)
    // Output: rd = signed 32-bit coefficient in [-(GAMMA1-1), GAMMA1]
    '{
        instr: 32'b1001011_00000_00000_111_00000_0111011,
        mask:  32'b1111111_00000_00000_111_00000_1111111,
        resp : '{
            accept : 1'b1,
            writeback : 1'b1,
            register_read : {1'b0, 1'b1, 1'b1},  // reads rs1 and rs2
            insn: OP_UNPACK_Z,
            done : 1'b1
        },
        opcode : SAMPLER
    }

  };



  // funct3 for horcrux_R intruction
  typedef enum logic [2:0] {
    OP_R_R0 = 3'b000,  //0
    OP_R_R1 = 3'b001,  //1
    OP_R_R2 = 3'b010,  //2
    OP_R_R3 = 3'b011,  //3
    OP_R_R4 = 3'b100,  //4
    OP_R_R5 = 3'b101,  //5
    OP_R_R6 = 3'b110,  //6
    OP_R_R7 = 3'b111   //7
  } horcrux_r_op;

  // funct2 for horcrux_R4 intruction
  typedef enum logic [1:0] {
    OP_R4_R0 = 2'b00,  //1
    OP_R4_R1 = 2'b01,  //1
    OP_R4_R2 = 2'b10,  //2
    OP_R4_R3 = 2'b11   //3
  } horcrux_r4_op;

  // Mode of operation of horcrux
  typedef enum logic [6:0] {
    FUNCT7_0  = 7'b0000000,  //0
    FUNCT7_1  = 7'b0000001,  //1
    FUNCT7_2  = 7'b0000010,  //2
    FUNCT7_3  = 7'b0000011,  //3
    FUNCT7_4  = 7'b0000100,  //4
    FUNCT7_5  = 7'b0000101,  //5
    FUNCT7_6  = 7'b0000110,  //6
    FUNCT7_7  = 7'b0000111,  //7
    FUNCT7_8  = 7'b0001000,  //8
    FUNCT7_9  = 7'b0001001,  //9
    FUNCT7_10 = 7'b0001010,  //10
    FUNCT7_11 = 7'b0001011,  //11
    FUNCT7_12 = 7'b0001100,  //12
    FUNCT7_13 = 7'b0001101,  //13
    FUNCT7_14 = 7'b0001110,  //14
    FUNCT7_15 = 7'b0001111,  //15
    FUNCT7_16 = 7'b0010000,  //16
    FUNCT7_17 = 7'b0010001,  //17
    FUNCT7_18 = 7'b0010010,  //18
    FUNCT7_19 = 7'b0010011,  //19
    FUNCT7_20 = 7'b0010100,  //20
    FUNCT7_21 = 7'b0010101,  //21
    FUNCT7_22 = 7'b0010110,  //22
    FUNCT7_23 = 7'b0010111,  //23
    FUNCT7_24 = 7'b0011000,  //24
    FUNCT7_25 = 7'b0011001,  //25
    FUNCT7_26 = 7'b0011010,  //26
    FUNCT7_27 = 7'b0011011,  //27
    FUNCT7_28 = 7'b0011100,  //28
    FUNCT7_29 = 7'b0011101,  //29
    FUNCT7_30 = 7'b0011110,  //30
    FUNCT7_31 = 7'b0011111,  //31
    FUNCT7_32 = 7'b0100000,  //32
    FUNCT7_33 = 7'b0100001,  //33
    FUNCT7_34 = 7'b0100010,  //34
    FUNCT7_35 = 7'b0100011,  //35
    FUNCT7_36 = 7'b0100100,  //36
    FUNCT7_37 = 7'b0100101,  //37
    FUNCT7_38 = 7'b0100110,  //38
    FUNCT7_39 = 7'b0100111,  //39
    FUNCT7_40 = 7'b0101000,  //40
    FUNCT7_41 = 7'b0101001,  //41
    FUNCT7_42 = 7'b0101010,  //42
    FUNCT7_43 = 7'b0101011,  //43
    FUNCT7_44 = 7'b0101100,  //44
    FUNCT7_45 = 7'b0101101   //45
  } horcrux_funct7;

  // funct3 for horcrux_I intruction
  typedef enum logic [2:0] {
    FUNCT3_1 = 3'b001,
    FUNCT3_2 = 3'b010,
    FUNCT3_3 = 3'b011,
    FUNCT3_4 = 3'b100,
    FUNCT3_5 = 3'b101,
    FUNCT3_6 = 3'b110,
    FUNCT3_7 = 3'b111
  } horcrux_funct3;

  // New instruction definition
  typedef union packed {
    struct packed {
      horcrux_funct7 funct7;  // 31:25
      logic [4:0]  rs2;     // 24:20
      logic [4:0]  rs1;     // 19:15
      horcrux_r_op   funct3;  // 14:12
      logic [4:0]  rd;      // 11:7
      logic [6:0]  opcode;  // 6:0
    } as_horcrux_R;
    struct packed {
      logic [4:0]  rs3;     // 31:27
      horcrux_r4_op  funct2;  // 26:25
      logic [4:0]  rs2;     // 24:20
      logic [4:0]  rs1;     // 19:15
      horcrux_funct3 funct3;  // 14:12
      logic [4:0]  rd;      // 11:7
      logic [6:0]  opcode;  // 6:0
    } as_horcrux_R4;
    logic [31:0] raw;
  } instruction_u;

  // Opcode of the new instruction
  typedef enum logic [6:0] {
    horcrux_R  = 7'b0111011,  //3b
    horcrux_R4 = 7'b1101011   //6b
  } horcrux_op;

  // State of the FSM of the controller
  typedef enum logic [2:0] {
    RESET_S,
    WAIT_S,
    LOAD_S,
    KECCAK_S,
    PROCESS_S,
    COMPUTE_S,
    STORE_S,
    DONE_S
  } controller_state_t;


  typedef struct packed {logic [2:0] mode_horcrux;} mode_t;

  typedef struct packed {logic [6:0] funct7_horcrux;} funct7_t;

  typedef struct packed {logic [1:0] funct2_horcrux;} funct2_t;

  typedef struct packed {logic [11:0] immediate_horcrux;} immediate_t;


  typedef struct packed {
    logic [31:0] rs1;
    logic [31:0] rs2;
    logic [31:0] rs3;
  } in_t;

  typedef struct packed {
    logic [31:0] rd1;
  } out_t;


endpackage
