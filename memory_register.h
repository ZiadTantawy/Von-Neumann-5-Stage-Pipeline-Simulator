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
extern int32_t registers[32];
extern int32_t PC;

// Function prototypes
void printEntireMemory();
void printRegisters();
int32_t get_reg(int idx);
void set_reg(int idx, int32_t value);

#endif /* MEMORY_REGISTER_H */