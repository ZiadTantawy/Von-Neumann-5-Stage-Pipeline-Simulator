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
const int32_t R0 = 0;
int32_t R1 = 0;
int32_t R2 = 0;
int32_t R3 = 0;
int32_t R4 = 0;
int32_t R5 = 0;
int32_t R6 = 0;
int32_t R7 = 0;
int32_t R8 = 0;
int32_t R9 = 0;
int32_t R10 = 0;
int32_t R11 = 0;
int32_t R12 = 0;
int32_t R13 = 0;
int32_t R14 = 0;
int32_t R15 = 0;
int32_t R16 = 0;
int32_t R17 = 0;
int32_t R18 = 0;
int32_t R19 = 0;
int32_t R20 = 0;
int32_t R21 = 0;
int32_t R22 = 0;
int32_t R23 = 0;
int32_t R24 = 0;
int32_t R25 = 0;
int32_t R26 = 0;
int32_t R27 = 0;
int32_t R28 = 0;
int32_t R29 = 0;
int32_t R30 = 0;
int32_t R31 = 0;
int32_t PC = 0;

void printEntireMemory() {
   /* printf("\n========= MEMORY DUMP =========\n");
    
    // Print instruction memory region
    printf("\nINSTRUCTION MEMORY (%d-%d):\n", INSTR_START, INSTR_END);
    for (int i = INSTR_START; i < next_Empty_IA; i++) {
        printf("0x%04X: 0x%08X\n", i, memory[i]);
    }
    
    // Print data memory region (if used)
    if (next_Empty_DA > DATA_START) {
        printf("\nDATA MEMORY (%d-%d):\n", DATA_START, DATA_END);
        for (int i = DATA_START; i < next_Empty_DA; i++) {
            printf("0x%04X: 0x%08X\n", i, memory[i]);
        }
    }
    
    printf("\n==============================\n");*/
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