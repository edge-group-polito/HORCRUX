# Keccak-1600 RISC-V 32-bit Coprocessor Assembly
# Targeting Custom Instructions:
# LOAD:    0x6b, funct2=0x03, funct3=0x01  (overwrite register pair)
# START:   0x6b, funct2=0x00, funct3=0x01  (start keccak, no writeback)
# STORE:   0x3b, funct7=0x0F, funct3=0x07  (read keccak result word)
# XOR:     0x6b, funct2=0x01, funct3=0x01  (XOR into register pair)
# PERMUTE: 0x6b, funct2=0x02, funct3=0x01  (permute in-place with writeback)
# INIT:    0x3b, funct7=0x0E, funct3=0x07  (zero register file)
# READ3:   0x3b, funct7=0x0D, funct3=0x07  (read 3 bytes from register file)

.section .text
.global keccak_coproc_S
.global keccak_hw_init
.global keccak_hw_absorb_xor
.global keccak_hw_permute
.global keccak_hw_store_word
.global keccak_hw_read3
.align 4

# --- CUSTOM INSTRUCTION MACROS ---

.macro KECCAK_HW_LOAD rs_low, rs_high, rs_index
    .insn r 0x6b, 0x01, 0x03, x0, \rs_low, \rs_high, \rs_index
.endm

.macro KECCAK_HW_START rs_trigger
    .insn r 0x6b, 0x01, 0x00, x0, \rs_trigger, x0, x0
.endm

.macro KECCAK_HW_STORE rd, rs_index
    .insn r 0x3b, 0x7, 0xF, \rd, \rs_index, x0
.endm

.macro KECCAK_HW_XOR rs_low, rs_high, rs_index
    .insn r 0x6b, 0x01, 0x01, x0, \rs_low, \rs_high, \rs_index
.endm

.macro KECCAK_HW_PERMUTE rs_trigger
    .insn r 0x6b, 0x01, 0x02, x0, \rs_trigger, x0, x0
.endm

.macro KECCAK_HW_INIT_STATE
    .insn r 0x3b, 0x7, 0x0E, x0, x0, x0
.endm

.macro KECCAK_HW_READ3 rd, rs_byte_offset
    .insn r 0x3b, 0x7, 0x0D, \rd, \rs_byte_offset, x0
.endm

# --- THE FUNCTION ---
keccak_coproc_S:
    # a0: pointer to uint32_t state[50]

    # 1. PROLOGUE: Save Callee-saved registers (s0-s4) and Return Address
    addi    sp, sp, -24
    sw      ra, 20(sp)
    sw      s0, 16(sp)
    sw      s1, 12(sp)
    sw      s2, 8(sp)
    sw      s3, 4(sp)
    sw      s4, 0(sp)

    # 2. LOAD PHASE: Transfer memory state to Coprocessor
    # We process 10 words (5 lanes) per loop to satisfy the 5-register requirement
    # while providing word-pairs to the HW_LOAD instruction.
    
    li      t0, 0               # Lane index (0, 2, 4 ... 48)
    mv      t1, a0              # Moving pointer for state

load_10_words_loop:
    # Load first 5 words into s0-s4 as requested
    lw      s0, 0(t1)           # state[i]
    lw      s1, 4(t1)           # state[i+1]
    lw      s2, 8(t1)           # state[i+2]
    lw      s3, 12(t1)          # state[i+3]
    lw      s4, 16(t1)          # state[i+4]
    
    # Load next 5 words into temporary registers t2-t6
    lw      t2, 20(t1)          # state[i+5]
    lw      t3, 24(t1)          # state[i+6]
    lw      t4, 28(t1)          # state[i+7]
    lw      t5, 32(t1)          # state[i+8]
    lw      t6, 36(t1)          # state[i+9]

    # Now that the register file is definitely populated, fire the loads
    # HW_LOAD rs_low, rs_high, index
    KECCAK_HW_LOAD s0, s1, t0   # Lane index t0
    addi    t0, t0, 2
    
    KECCAK_HW_LOAD s2, s3, t0   # Lane index t0+2
    addi    t0, t0, 2
    
    KECCAK_HW_LOAD s4, t2, t0   # Lane index t0+4 (Uses s4 and t2)
    addi    t0, t0, 2
    
    KECCAK_HW_LOAD t3, t4, t0   # Lane index t0+6
    addi    t0, t0, 2
    
    KECCAK_HW_LOAD t5, t6, t0   # Lane index t0+8
    addi    t0, t0, 2

    # Increment memory pointer by 10 words (40 bytes)
    addi    t1, t1, 40
    li      t2, 50
    blt     t0, t2, load_10_words_loop

    # 3. START PHASE: Trigger Hardware
    li      t0, 1
    KECCAK_HW_START t0

    # 4. LATENCY PHASE: 24 NOPs
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop

    # 5. STORE PHASE: Retrieve resulting state[50]
    li      t0, 0               # Reset Word index
    mv      t1, a0              # FIX: Reset moving pointer to start of state

store_10_words_loop:
    # --- Step A: Retrieve 10 words from Coprocessor into registers ---
    # Using multiple registers avoids "Write-After-Write" stalls
    KECCAK_HW_STORE t2, t0
    addi    t0, t0, 1
    KECCAK_HW_STORE t3, t0
    addi    t0, t0, 1
    KECCAK_HW_STORE t4, t0
    addi    t0, t0, 1
    KECCAK_HW_STORE t5, t0
    addi    t0, t0, 1
    KECCAK_HW_STORE t6, t0
    addi    t0, t0, 1
    
    KECCAK_HW_STORE s0, t0
    addi    t0, t0, 1
    KECCAK_HW_STORE s1, t0
    addi    t0, t0, 1
    KECCAK_HW_STORE s2, t0
    addi    t0, t0, 1
    KECCAK_HW_STORE s3, t0
    addi    t0, t0, 1
    KECCAK_HW_STORE s4, t0
    addi    t0, t0, 1

    # --- Step B: Store the 10 registers to memory ---
    # Using fixed offsets is much faster than recalculating addresses
    sw      t2, 0(t1)
    sw      t3, 4(t1)
    sw      t4, 8(t1)
    sw      t5, 12(t1)
    sw      t6, 16(t1)
    sw      s0, 20(t1)
    sw      s1, 24(t1)
    sw      s2, 28(t1)
    sw      s3, 32(t1)
    sw      s4, 36(t1)

    # Increment memory pointer by 40 bytes
    addi    t1, t1, 40
    
    # Check loop condition (5 iterations total for 50 words)
    li      t2, 50
    blt     t0, t2, store_10_words_loop

    
    # 6. EPILOGUE: Restore registers
    lw      s4, 0(sp)
    lw      s3, 4(sp)
    lw      s2, 8(sp)
    lw      s1, 12(sp)
    lw      s0, 16(sp)
    lw      ra, 20(sp)
    addi    sp, sp, 24
    ret


# ============================================================================
# NEW FUNCTIONS: HW-managed Keccak state (absorb + permute in-place)
# ============================================================================

# --- keccak_hw_init: Zero the HW register file ---
# void keccak_hw_init(void);
keccak_hw_init:
    KECCAK_HW_INIT_STATE
    ret

# --- keccak_hw_absorb_xor: XOR a word pair into HW register file ---
# void keccak_hw_absorb_xor(uint32_t lo, uint32_t hi, uint32_t index);
# a0 = low word, a1 = high word, a2 = register index
keccak_hw_absorb_xor:
    KECCAK_HW_XOR a0, a1, a2
    ret

# --- keccak_hw_permute: In-place permutation (result stays in register file) ---
# void keccak_hw_permute(void);
keccak_hw_permute:
    li      t0, 1
    KECCAK_HW_PERMUTE t0
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    ret

# --- keccak_hw_store_word: Read one word from keccak result ---
# uint32_t keccak_hw_store_word(uint32_t index);
# a0 = index, returns result in a0
keccak_hw_store_word:
    KECCAK_HW_STORE a0, a0
    ret

# --- keccak_hw_read3: Read 3 consecutive bytes from register file ---
# uint32_t keccak_hw_read3(uint32_t byte_offset);
# a0 = byte offset, returns {0, byte2, byte1, byte0} in a0
keccak_hw_read3:
    KECCAK_HW_READ3 a0, a0
    ret