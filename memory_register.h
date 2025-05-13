#ifndef MEMORY_REGISTER_H
#define MEMORY_REGISTER_H

#include <stdint.h>

// Constants
#define INSTR_START  0
#define INSTR_END    1023
#define DATA_START   1024
#define DATA_END     2047
#define MEMORY_SIZE  2048
#define INSTR_SIZE   32

// Global variables
extern int next_Empty_IA;
extern int next_Empty_DA;
extern int32_t memory[2048];
extern int32_t PC;
extern const int32_t R0;
extern int32_t R1, R2, R3, R4, R5, R6, R7, R8;
extern int32_t R9, R10, R11, R12, R13, R14, R15, R16;
extern int32_t R17, R18, R19, R20, R21, R22, R23, R24;
extern int32_t R25, R26, R27, R28, R29, R30, R31;

// Function prototypes
void printEntireMemory();
void printMemory(int start, int end);
void printRegisters();
void debugDump();

#endif /* MEMORY_REGISTER_H */