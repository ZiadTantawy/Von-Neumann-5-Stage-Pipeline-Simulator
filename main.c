#include <stdio.h>
#include <stdint.h>
#include "parser.h"
#include "memory_register.h"

// External functions and variables for memory and register file
extern int ReadFile(const char *filename, int32_t *memory, int *next_free_IA);
extern void printEntireMemory();
extern void printRegisters();
extern int32_t memory[2048]; // Main memory: 2048 words, 32 bits each
extern int next_Empty_IA;    // Next free instruction address
extern int32_t PC;           // Program Counter

int clockCycle = 1; // Global clock cycle counter

// Pipeline stage struct: holds all information for an instruction in a pipeline stage
// valid: is this stage active?
// instruction: the raw 32-bit instruction
// pc: the PC value for this instruction
// opcode, r1, r2, r3, shamt, imm, address: decoded fields
// alu_result, mem_data: results from ALU or memory
// write_reg: destination register
// stage_cycles: how many cycles spent in this stage
typedef struct {
    int valid;
    int instruction;
    int pc;
    int opcode, r1, r2, r3, shamt, imm, address;
    int alu_result, mem_data, write_reg;
    int stage_cycles;
} Pipeline_Stage;

// Instantiate pipeline registers for each stage
Pipeline_Stage IF = {0}, ID = {0}, EX = {0}, MEM = {0}, WB = {0};

// Sign-extend an 18-bit immediate to 32 bits (for I-format instructions)
int32_t sign_extend_18(int32_t imm) {
    if (imm & (1 << 17)) { // If sign bit (bit 17) is set
        imm |= ~((1 << 18) - 1); // Fill upper bits with 1s
    }
    return imm;
}

// Helper function for data forwarding (bypassing) in the pipeline
int get_forwarded_value(int reg, int orig_val) {
    // Forward from MEM stage if writing to this reg
    if (MEM.valid && (MEM.r1 == reg) && (MEM.opcode != 11) && (MEM.opcode != 7)) {
        if (MEM.opcode == 10) // LW
            return MEM.mem_data;
        else
            return MEM.alu_result;
    }
    // Forward from WB stage if writing to this reg
    if (WB.valid && (WB.r1 == reg) && (WB.opcode != 11) && (WB.opcode != 7)) {
        if (WB.opcode == 10) // LW
            return WB.mem_data;
        else
            return WB.alu_result;
    }
    // Otherwise, use register file
    return orig_val;
}

// Fetch stage: fetches the next instruction from memory using PC
void fetch() {
    // Fetch instruction at PC
    IF.instruction = memory[PC];
    IF.pc = PC;
    IF.valid = 1;
    IF.stage_cycles = 0;
    PC++; // Increment PC for next fetch
}

// Decode stage: decodes the instruction and extracts all fields
void decode() {
    // Extract fields from the 32-bit instruction
    ID.opcode = (ID.instruction >> 28) & 0xF;         // 4 bits
    ID.r1 = (ID.instruction >> 23) & 0x1F;            // 5 bits
    ID.r2 = (ID.instruction >> 18) & 0x1F;            // 5 bits
    ID.r3 = (ID.instruction >> 13) & 0x1F;            // 5 bits
    ID.shamt = ID.instruction & 0x1FFF;               // 13 bits
    
    // For I-format instructions, extract 18-bit immediate
    if ((ID.opcode >= 2 && ID.opcode <= 6) || ID.opcode == 10 || ID.opcode == 11) {
        ID.imm = ID.instruction & 0x3FFFF;            // 18 bits
        // Sign extend if needed
        if (ID.imm & (1 << 17)) {
            ID.imm |= ~((1 << 18) - 1);
        }
    } else {
        ID.imm = 0;
    }
    
    // For J-format instructions, extract 28-bit address
    if (ID.opcode == 7) {
        ID.address = ID.instruction & 0xFFFFFFF;      // 28 bits
    } else {
        ID.address = 0;
    }
    
    printf("[ID] Decoded: opcode=%d r1=%d r2=%d r3=%d shamt=%d imm=%d address=%d\n",
           ID.opcode, ID.r1, ID.r2, ID.r3, ID.shamt, ID.imm, ID.address);
}

// Execute stage: performs the ALU operation or branch calculation
void execute() {
    // Use data forwarding for register operands
    int val_r2 = get_forwarded_value(EX.r2, get_reg(EX.r2));
    int val_r3 = get_forwarded_value(EX.r3, get_reg(EX.r3));
    int val_r1 = get_forwarded_value(EX.r1, get_reg(EX.r1));
    
    // Sign extend the immediate value
    int32_t imm = EX.imm;
    if (imm & (1 << 17)) {  // If sign bit is set
        imm |= ~((1 << 18) - 1);  // Sign extend
    }
    
    printf("[EX] r1=%d val_r1=%d r2=%d val_r2=%d r3=%d val_r3=%d imm=%d\n", 
           EX.r1, val_r1, EX.r2, val_r2, EX.r3, val_r3, imm);
    
    switch(EX.opcode) {
        case 0: // ADD (R-format)
            EX.alu_result = val_r2 + val_r3;
            printf("[EX] ADD: %d + %d = %d\n", val_r2, val_r3, EX.alu_result);
            break;
        case 1: // SUB (R-format)
            EX.alu_result = val_r2 - val_r3;
            printf("[EX] SUB: %d - %d = %d\n", val_r2, val_r3, EX.alu_result);
            break;
        case 2: // MULI (I-format)
            EX.alu_result = val_r2 * imm;
            printf("[EX] MULI: %d * %d = %d\n", val_r2, imm, EX.alu_result);
            break;
        case 3: // ADDI (I-format)
            EX.alu_result = val_r2 + imm;
            printf("[EX] ADDI: %d + %d = %d\n", val_r2, imm, EX.alu_result);
            break;
        case 4: // BNE (I-format)
            if(val_r1 != val_r2) {
                PC = PC + imm;
                printf("[EX] BNE: %d != %d, PC = %d\n", val_r1, val_r2, PC);
            }
            break;
        case 5: // ANDI (I-format)
            EX.alu_result = val_r2 & imm;
            printf("[EX] ANDI: %d & %d = %d\n", val_r2, imm, EX.alu_result);
            break;
        case 6: // ORI (I-format)
            EX.alu_result = val_r2 | imm;
            printf("[EX] ORI: %d | %d = %d\n", val_r2, imm, EX.alu_result);
            break;
        case 7: // J (J-format)
            PC = EX.address;
            printf("[EX] J: PC = %d\n", PC);
            break;
        case 8: // SLL (R-format)
            EX.alu_result = val_r2 << EX.shamt;
            printf("[EX] SLL: %d << %d = %d\n", val_r2, EX.shamt, EX.alu_result);
            break;
        case 9: // SRL (R-format)
            EX.alu_result = (uint32_t)val_r2 >> EX.shamt;
            printf("[EX] SRL: %d >> %d = %d\n", val_r2, EX.shamt, EX.alu_result);
            break;
        case 10: // LW (I-format)
            EX.alu_result = val_r2 + imm;
            printf("[EX] LW: address = %d + %d = %d\n", val_r2, imm, EX.alu_result);
            break;
        case 11: // SW (I-format)
            EX.alu_result = val_r2 + imm;
            printf("[EX] SW: address = %d + %d = %d\n", val_r2, imm, EX.alu_result);
            break;
    }
}

// Memory stage: performs memory access for LW/SW
void memory_access() {
    if (MEM.opcode == 10) // LW: load from memory
        MEM.mem_data = memory[MEM.alu_result];
    if (MEM.opcode == 11) // SW: store to memory
        memory[MEM.alu_result] = get_reg(MEM.r1);
}

// Write Back stage: writes result to register file if needed
void write_back() {
    if (!WB.valid) return;
    
    printf("[WB] Writing back: opcode=%d r1=%d alu_result=%d mem_data=%d\n",
           WB.opcode, WB.r1, WB.alu_result, WB.mem_data);
    
    switch(WB.opcode) {
        // For all instructions that write to a register
        case 0: case 1: case 2: case 3: case 5: case 6: case 8: case 9:
            printf("[WB] Writing ALU result %d to R%d\n", WB.alu_result, WB.r1);
            set_reg(WB.r1, WB.alu_result); // Write ALU result to r1
            break;
        case 10: // LW: write loaded memory data to r1
            printf("[WB] Writing memory data %d to R%d (LW)\n", WB.mem_data, WB.r1);
            set_reg(WB.r1, WB.mem_data);
            break;
        // SW and J do not write to registers
        default:
            printf("[WB] No register write for opcode %d\n", WB.opcode);
            break;
    }
}

// Print the state of the pipeline and registers for debugging
void printState() {
    printf("\n==== CLOCK CYCLE %d ====\n", clockCycle);
    printf("PC: %d\n", PC);
    if (IF.valid)
        printf("IF: Instruction 0x%08X at PC %d\n", IF.instruction, IF.pc);
    else
        printf("IF: -\n");
    if (ID.valid)
        printf("ID: Opcode %d, r1 %d, r2 %d, r3 %d, shamt %d, imm %d, address %d\n", ID.opcode, ID.r1, ID.r2, ID.r3, ID.shamt, ID.imm, ID.address);
    else
        printf("ID: -\n");
    if (EX.valid)
        printf("EX: ALU Result %d\n", EX.alu_result);
    else
        printf("EX: -\n");
    if (MEM.valid)
        printf("MEM: Mem Data %d\n", MEM.mem_data);
    else
        printf("MEM: -\n");
    if (WB.valid)
        printf("WB: Write Back Value %d\n", WB.alu_result);
    else
        printf("WB: -\n");
    printf("Registers: R1=%d R2=%d R3=%d R4=%d ... R31=%d\n", get_reg(1), get_reg(2), get_reg(3), get_reg(4), get_reg(31));
    printf("============================\n");
}

// Pipeline control: advances instructions through the pipeline according to timing rules
void pipeline_control() {
    // 1. Write Back: if WB stage has been active for 1 cycle, perform write back and clear
    if (WB.valid && WB.stage_cycles == 1) {
        write_back();
        WB.valid = 0;
    }
    // 2. Memory: if MEM.valid and MEM.stage_cycles == 1, perform memory access BEFORE copying to WB
    if (MEM.valid && MEM.stage_cycles == 1) {
        memory_access(); // Perform memory access first
        WB = MEM;
        WB.valid = 1;
        WB.stage_cycles = 0;
        MEM.valid = 0;
    }
    // 3. Execute: if EX.valid and EX.stage_cycles == 2, perform execute BEFORE copying to MEM
    if (EX.valid && EX.stage_cycles == 2) {
        execute(); // Perform ALU operation first
        MEM = EX;
        MEM.valid = 1;
        MEM.stage_cycles = 0;
        EX.valid = 0;
    }
    // 4. Decode: if ID stage has been active for 2 cycles, move to EX
    if (ID.valid && ID.stage_cycles == 2) {
        EX = ID;
        EX.valid = 1;
        EX.stage_cycles = 0;
        decode();  // Decode the instruction in EX stage
        ID.valid = 0;
    }
    // 5. Fetch: only on odd cycles and if MEM is not active, fetch new instruction
    if ((clockCycle % 2 != 0) && !MEM.valid && PC < next_Empty_IA) {
        fetch();
    }
    // 6. Move pipeline registers forward (increment stage_cycles for all valid stages)
    if (IF.valid) {
        ID = IF;
        ID.valid = 1;
        ID.stage_cycles = 0;
        IF.valid = 0;
        decode();  // Decode as soon as instruction enters ID
    }
    // Increment stage cycles for all valid stages
    if (ID.valid) ID.stage_cycles++;
    if (EX.valid) EX.stage_cycles++;
    if (MEM.valid) MEM.stage_cycles++;
    if (WB.valid) WB.stage_cycles++;
    // Debug print for pipeline stages
    printf("[Pipeline] IF=%d ID=%d EX=%d MEM=%d WB=%d\n", IF.valid, ID.valid, EX.valid, MEM.valid, WB.valid);
}

// Main function: loads program, runs pipeline, prints state each cycle
int main() {
    // Initialize memory to zero
    for (int i = 0; i < 2048; i++) memory[i] = 0;
    PC = 0; // Start PC at 0
    clockCycle = 1; // Start clock at 1
    // Load program from file into memory
    const char* testFile = "test.txt";
    if (ReadFile(testFile, memory, &next_Empty_IA) != 0) {
        printf("Error loading program\n");
        return 1;
    }
    // Initialize pipeline stages to inactive
    IF.valid = ID.valid = EX.valid = MEM.valid = WB.valid = 0;
    IF.stage_cycles = ID.stage_cycles = EX.stage_cycles = MEM.stage_cycles = WB.stage_cycles = 0;
    printf("\nStarting program execution...\n");
    // Main pipeline loop: run until all instructions have exited the pipeline
    while (PC < next_Empty_IA || IF.valid || ID.valid || EX.valid || MEM.valid || WB.valid) {
        printState();      // Print state for debugging (moved before pipeline_control)
        pipeline_control(); // Advance pipeline
        clockCycle++;      // Next clock cycle
    }
    printf("Program finished.\n");
    return 0;
}


