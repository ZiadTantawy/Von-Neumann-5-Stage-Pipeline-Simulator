#include "memory_register.h"

// Single definition of reg_file
RegisterFile reg_file = { .R = {0}, .PC = 0 };
int32_t memory[MEMORY_SIZE] = {0};
int next_Empty_IA = 0;
int next_Empty_DA = 0;

void printEntireMemory(void) {
    printf("\n========= MEMORY DUMP =========\n");
    printf("\nINSTRUCTION MEMORY (%d-%d):\n", INSTR_START, INSTR_END);
    for (int i = INSTR_START; i < next_Empty_IA && i <= INSTR_END; i++) {
        printf("Address %d: %d (0x%08X)\n", i, memory[i], memory[i]);
    }
    
    printf("\nDATA MEMORY (%d-%d):\n", DATA_START, DATA_END);
    for (int i = DATA_START; i < MEMORY_SIZE && i <= DATA_END; i++) {
        if (memory[i] != 0) {  // Only print non-zero data memory
            printf("Address %d: %d (0x%08X)\n", i, memory[i], memory[i]);
        }
    }
    printf("\n==============================\n");
}

void safeMemoryWrite(int address, int32_t value) {
    if (address >= 0 && address < MEMORY_SIZE) {
        memory[address] = value;
    } else {
        printf("ERROR: Memory write out of bounds: %d\n", address);
    }
}

int32_t safeMemoryRead(int address) {
    if (address >= 0 && address < MEMORY_SIZE) {
        return memory[address];
    }
    printf("ERROR: Memory read out of bounds: %d\n", address);
    return 0;
}