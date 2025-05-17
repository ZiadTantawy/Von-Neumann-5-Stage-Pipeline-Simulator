#include <stdio.h>
#include <stdint.h>
#include "parser.h"
#include "memory_register.h"

// External functions from your other files
extern int ReadFile(const char *filename, int32_t *memory, int *next_free_IA);
extern void printEntireMemory();
extern int32_t memory[2048];
extern int next_Empty_IA;
extern int32_t PC;
int clockCycle = 1;

extern const int32_t R0;
extern int32_t R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15;
extern int32_t R16, R17, R18, R19, R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R30, R31;
    
// Cast the const pointer to non-const to match the array type
int32_t *registers[32] = {
    (int32_t*)&R0, &R1, &R2, &R3, &R4, &R5, &R6, &R7, &R8, &R9, &R10, &R11, &R12, &R13, &R14, &R15,
    &R16, &R17, &R18, &R19, &R20, &R21, &R22, &R23, &R24, &R25, &R26, &R27, &R28, &R29, &R30, &R31
};

typedef struct{
    int valid;
    int instruction;
    int pc;

    int opcode;
    int r1,r2,r3;
    int shamt;
    int imm;
    int address;

    int alu_result;
    int mem_data;
    int write_reg;

    int stage_cycles;
} Pipeline_Stage;

Pipeline_Stage IF, ID, EX, MEM, WB;

// Binary int format in c is 0b00000000000000000000000000000000 (32 bits)

void fetch()
{
    if (!IF.valid) {
        IF.instruction = memory[PC];
        IF.pc = PC;
        IF.valid = 1;
        IF.stage_cycles = 0;
        PC += 4;  // Increment PC after fetching
    }
}

int32_t sign_extend_18(int32_t imm) {
    if (imm & (1 << 17)) { // Check if bit 17 is 1 (negative number)
        imm |= ~((1 << 18) - 1); // Fill upper bits with 1s
    }
    return imm;
}

void decode()
{
    if (!ID.valid) return; // Only decode if there's a valid instruction

    // Extract fields
    ID.opcode  = (ID.instruction >> 28) & 0xF;
    ID.r1      = (ID.instruction >> 23) & 0x1F;
    ID.r2      = (ID.instruction >> 18) & 0x1F;
    ID.r3      = (ID.instruction >> 13) & 0x1F;
    ID.shamt   = ID.instruction & 0x1FFF;
    ID.imm     = sign_extend_18(ID.instruction & 0x3FFFF);
    ID.address = ID.instruction & 0xFFFFFFF;

    // Stay in ID for 2 cycles
    if (ID.stage_cycles == 1) {
        ID.valid = 0;
        ID.stage_cycles = 0;
    } else {
        ID.stage_cycles++;
    }
}

void execute()
{
    switch(EX.opcode){
        case 0:
            // ADD
            EX.alu_result = *registers[EX.r2] + *registers[EX.r3];
            EX.write_reg = EX.r1;
            break;
        case 1:
            // SUB
            EX.alu_result = *registers[EX.r2] - *registers[EX.r3];
            EX.write_reg = EX.r1;
            break;
        case 2:
            // MULI
            EX.alu_result = *registers[EX.r2] * EX.imm;
            EX.write_reg = EX.r1;
            break;
        case 3:
            // ADDI
            EX.alu_result = *registers[EX.r2] + EX.imm;
            EX.write_reg = EX.r1;
            break;
        case 4:
            // BNE
            if (*registers[EX.r1] != *registers[EX.r2]) {
                PC = PC + EX.imm;
            }
            break;
        case 5:
            // ANDI
            EX.alu_result = *registers[EX.r2] & EX.imm;
            EX.write_reg = EX.r1;
            break;
        case 6:
            // ORI
            EX.alu_result = *registers[EX.r2] | EX.imm;
            EX.write_reg = EX.r1;
            break;
        case 7:
            // J
            PC = (PC & 0xF0000000) | (EX.address & 0x0FFFFFFF);
            break;
        case 8:
            // SLL
            EX.alu_result = *registers[EX.r2] << EX.shamt;
            EX.write_reg = EX.r1;
            break;
        case 9:
            // SRL
            EX.alu_result = *registers[EX.r2] >> EX.shamt;
            EX.write_reg = EX.r1;
            break;
        case 10:
            // LW
            EX.alu_result = *registers[EX.r2] + EX.imm;
            EX.mem_data = memory[EX.alu_result];
            EX.write_reg = EX.r1;
            break;
        case 11:
            // SW
            EX.alu_result = *registers[EX.r2] + EX.imm;
            EX.mem_data = *registers[EX.r1];
            break;
    }

    *((int32_t*)&R0) = 0;
    if (EX.stage_cycles == 1) {
        EX.valid = 0;
        EX.stage_cycles = 0;
    } else {
        EX.stage_cycles++;
    }
}

void mem_access(){
    if(MEM.opcode == 10){
        MEM.mem_data = memory[MEM.alu_result];
    }
    else if(MEM.opcode == 11){
        memory[MEM.alu_result] = MEM.mem_data;
    }
}

void write_back(){
    if((WB.opcode >= 0 && WB.opcode <= 3) || (WB.opcode >= 5 && WB.opcode <= 6) || (WB.opcode >= 8 && WB.opcode <= 9)){
        *registers[WB.write_reg] = WB.alu_result;
    }
    if(WB.opcode == 10){
        *registers[WB.write_reg] = WB.mem_data;
    }
}

void pipeline_control(){
    // Data hazard detection (RAW - Read After Write)
    if (EX.valid && ID.valid) {
        // Check if EX stage is writing to a register that ID stage needs to read
        if (EX.write_reg != 0 && // Don't stall for x0
            ((EX.write_reg == ID.r1) || (EX.write_reg == ID.r2) || (EX.write_reg == ID.r3))) {
            // Stall the pipeline
            IF.valid = 0;
            return;
        }
    }

    if (MEM.valid && ID.valid) {
        // Check if MEM stage is writing to a register that ID stage needs to read
        if (MEM.write_reg != 0 && // Don't stall for x0
            ((MEM.write_reg == ID.r1) || (MEM.write_reg == ID.r2) || (MEM.write_reg == ID.r3))) {
            // Stall the pipeline
            IF.valid = 0;
            return;
        }
    }

    // Control hazard detection (for branches and jumps)
    if (EX.valid && (EX.opcode == 4 || EX.opcode == 7)) { // BNE or J
        // Flush the pipeline
        IF.valid = 0;
        ID.valid = 0;
    }

    // Advance pipeline stages
    if (WB.valid) {
        WB.valid = 0;
        WB.stage_cycles = 0;
    }

    if (MEM.valid) {
        MEM.valid = 0;
        MEM.stage_cycles = 0;
    }

    if (EX.valid) {
        EX.valid = 0;
        EX.stage_cycles = 0;
    }

    if (ID.valid) {
        ID.valid = 0;
        ID.stage_cycles = 0;
    }

    if (IF.valid) {
        IF.valid = 0;
        IF.stage_cycles = 0;
    }
}

int main(){
    // Initialize memory and registers
    if (ReadFile("test.txt", memory, &next_Empty_IA) != 0) {
        printf("Error loading program\n");
        return 1;
    }

    // Initialize pipeline stages
    IF.valid = 0;
    IF.stage_cycles = 0;
    ID.valid = 0;
    ID.stage_cycles = 0;
    EX.valid = 0;
    EX.stage_cycles = 0;
    MEM.valid = 0;
    MEM.stage_cycles = 0;
    WB.valid = 0;
    WB.stage_cycles = 0;

    // Set initial PC
    PC = 0;
    clockCycle = 1;

    printf("Program loaded successfully. Starting execution...\n");
    printf("Number of instructions: %d\n", next_Empty_IA);

    // Run pipeline
    while (1) {
        printf("\nCycle %d:\n", clockCycle);
        
        // Run pipeline stages in reverse order
        write_back();
        mem_access();
        execute();
        decode();
        fetch();
        
        // Handle pipeline control
        pipeline_control();

        // Print pipeline state
        printf("IF: %s (PC: %d)\n", IF.valid ? "Valid" : "Invalid", IF.pc);
        printf("ID: %s (Opcode: %d)\n", ID.valid ? "Valid" : "Invalid", ID.opcode);
        printf("EX: %s (Opcode: %d)\n", EX.valid ? "Valid" : "Invalid", EX.opcode);
        printf("MEM: %s (Opcode: %d)\n", MEM.valid ? "Valid" : "Invalid", MEM.opcode);
        printf("WB: %s (Opcode: %d)\n", WB.valid ? "Valid" : "Invalid", WB.opcode);

        // Print register values
        printf("\nRegister Values:\n");
        for (int i = 0; i < 32; i++) {
            printf("x%d: %d\n", i, *registers[i]);
        }

        // Check if program is complete
        if (!IF.valid && !ID.valid && !EX.valid && !MEM.valid && !WB.valid) {
            printf("\nAll instructions completed.\n");
            break;
        }

        clockCycle++;
        
        // Safety check to prevent infinite loops
        if (clockCycle > 100) {
            printf("\nMaximum cycles reached. Stopping execution.\n");
            break;
        }
    }

    printf("\nProgram completed in %d cycles\n", clockCycle);
    return 0;
}