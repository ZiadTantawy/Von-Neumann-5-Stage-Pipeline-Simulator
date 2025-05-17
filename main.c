#include <stdio.h>
#include <stdint.h>
#include "memory_register.h"
#include "parser.h"
#include "pipeline.h"
#include "cpu.h"

int32_t fetch(void) {
    if (reg_file.PC >= next_Empty_IA) return -1;
    int32_t instruction = memory[reg_file.PC];
    reg_file.PC++;
    return instruction;
}

void decode(int32_t instruction, int *opcode, int *r1, int *r2, int *r3, 
            int *shamt, int *imm, int *address) {
    *opcode = (instruction >> 28) & 0xF;
    *r1 = (instruction >> 23) & 0x1F;
    *r2 = (instruction >> 18) & 0x1F;
    *r3 = (instruction >> 13) & 0x1F;
    *shamt = instruction & 0x1FFF;
    *imm = instruction & 0x3FFFF;
    *address = instruction & 0xFFFFFFF;
    
    if ((*imm & 0x20000) != 0) {
        *imm |= 0xFFFC0000;
    }
}

void execute(int32_t instruction, int opcode, int r1, int r2, int r3,
             int shamt, int imm, int address) {
    if (r1 < 0 || r1 > 31 || r2 < 0 || r2 > 31 || r3 < 0 || r3 > 31) {
        printf("Error: Invalid register number\n");
        return;
    }

    printf("Executing: opcode=%d, r1=%d, r2=%d, r3=%d, imm=%d, addr=%d\n",
           opcode, r1, r2, r3, imm, address);

    switch(opcode) {
        case 0: reg_file.R[r1] = reg_file.R[r2] + reg_file.R[r3]; break;
        case 1: reg_file.R[r1] = reg_file.R[r2] - reg_file.R[r3]; break;
        case 2: reg_file.R[r1] = reg_file.R[r2] * imm; break;
        case 3: reg_file.R[r1] = reg_file.R[r2] + imm; break;
        case 4: if (reg_file.R[r1] != reg_file.R[r2]) reg_file.PC += imm - 1; break;
        case 5: reg_file.R[r1] = reg_file.R[r2] & imm; break;
        case 6: reg_file.R[r1] = reg_file.R[r2] | imm; break;
        case 7: reg_file.PC = (reg_file.PC & 0xF0000000) | (address & 0x0FFFFFFF); break;
        case 8: reg_file.R[r1] = reg_file.R[r2] << shamt; break;
        case 9: reg_file.R[r1] = (uint32_t)reg_file.R[r2] >> shamt; break;
        case 10: if (reg_file.R[r2]+imm >= 0 && reg_file.R[r2]+imm < MEMORY_SIZE)
                    reg_file.R[r1] = memory[reg_file.R[r2]+imm]; break;
        case 11: if (reg_file.R[r2]+imm >= 0 && reg_file.R[r2]+imm < MEMORY_SIZE)
                    memory[reg_file.R[r2]+imm] = reg_file.R[r1]; break;
        default: printf("Unknown opcode: %d\n", opcode);
    }
    reg_file.R[0] = 0;
}

void print_register_state() {
    printf("\nRegister State:\n");
    for (int i = 0; i < 32; i++) {
        if (reg_file.R[i] != 0 || i == 0) {
            printf("R%d = %d\n", i, reg_file.R[i]);
        }
    }
    printf("PC = %d\n", reg_file.PC);
}

int pipeline_has_active_instructions(Pipeline *p) {
    for (int i = 0; i < 5; i++) {
        if (p->stages[i].instruction && !p->stages[i].is_bubble) {
            return 1;
        }
    }
    return 0;
}

int main() {
    // Initialize memory and registers
    for (int i = 0; i < MEMORY_SIZE; i++) memory[i] = 0;
    reg_file.PC = 0;
    for (int i = 0; i < 32; i++) reg_file.R[i] = 0;

    const char* testFile = "test.txt";
    
    printf("Loading program from %s\n", testFile);
    if (ReadFile(testFile, memory, &next_Empty_IA) == 0) {
        printf("Successfully loaded %d instructions\n", next_Empty_IA);
        printEntireMemory();
        
        Pipeline pipeline;
        pipeline_init(&pipeline, next_Empty_IA);
        
        printf("Expected cycles: %d\n", pipeline.expected_cycles);
        
        printf("\nStarting pipeline execution...\n");
        
        // Main execution loop
        while (pipeline_should_continue(&pipeline)) {
            pipeline_cycle(&pipeline);
        }
        
        printf("\nExecution completed in %d cycles\n", pipeline.cycle_count);
        print_register_state();
        printEntireMemory();
    } else {
        printf("Failed to load program\n");
    }
    
    return 0;
}