#ifndef MEMORY_REGISTER_H
#define MEMORY_REGISTER_H

#include <stdint.h>
#include <stdio.h>  // Added for printf

#define MEMORY_SIZE 2048
#define INSTR_START 0
#define INSTR_END 1023
#define DATA_START 1024
#define DATA_END 2047

typedef struct {
    int32_t R[32];  // R0-R31
    int32_t PC;
} RegisterFile;

// Declaration only (no definition)
extern RegisterFile reg_file;
extern int32_t memory[MEMORY_SIZE];
extern int next_Empty_IA;
extern int next_Empty_DA;

void printEntireMemory(void);
void safeMemoryWrite(int address, int32_t value);
int32_t safeMemoryRead(int address);

#endif