#ifndef CPU_H
#define CPU_H

#include <stdint.h>

// Pipeline functions
int32_t fetch(void);
void decode(int32_t instruction, int *opcode, int *r1, int *r2, int *r3, 
            int *shamt, int *imm, int *address);
void execute(int32_t instruction, int opcode, int r1, int r2, int r3,
             int shamt, int imm, int address);

#endif