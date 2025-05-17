#include <stdio.h>
#include <stdint.h>
#include "memory_register.h"

#define INSTR_START  0
#define INSTR_END    1023
#define DATA_START   1024
#define DATA_END     2047
#define MEMORY_SIZE  2048
#define INSTR_SIZE   32

int next_Empty_IA = 0;
int next_Empty_DA = 0;

int32_t memory[2048];
int32_t registers[32] = {0};  // All registers initialized to 0
int32_t PC = 0;

// Helper function to read a register value
int32_t get_reg(int idx) {
    if (idx == 0) return 0;  // R0 is always 0
    return registers[idx];
}

// Helper function to write a value to a register
void set_reg(int idx, int32_t value) {
    if (idx == 0) return;  // R0 cannot be written to
    registers[idx] = value;
    printf("[set_reg] R%d = %d\n", idx, value);
}

void printEntireMemory() {
    printf("\n========= MEMORY DUMP =========\n");
    
    // Print instruction memory region
    printf("\nINSTRUCTION MEMORY (%d-%d):\n", INSTR_START, INSTR_END);
    for (int i = INSTR_START; i < next_Empty_IA; i++) {
        printf("Address %d: %d\n", i, memory[i]);
    }
    
    // Print data memory region (if used)
    if (next_Empty_DA > DATA_START) {
        printf("\nDATA MEMORY (%d-%d):\n", DATA_START, DATA_END);
        for (int i = DATA_START; i < next_Empty_DA; i++) {
            printf("Address %d: %d\n", i, memory[i]);
        }
    }
    
    printf("\n==============================\n");
}

void printRegisters() {
    printf("\n========= REGISTER DUMP =========\n");
    for (int i = 0; i < 32; i++) {
        printf("R%d = %d\n", i, get_reg(i));
    }
    printf("PC = %d\n", PC);
    printf("==============================\n");
}
