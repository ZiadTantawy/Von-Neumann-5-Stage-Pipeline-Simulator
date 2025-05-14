#include <stdio.h>
#include <stdint.h>
#include "parser.h"
#include "memory_register.h"

// External functions from your other files
extern int ReadFile(const char *filename, int32_t *memory, int *next_free_IA);
extern void printEntireMemory();
extern int32_t memory[2048];
extern int next_Empty_IA;

// Binary int format in c is 0b00000000000000000000000000000000 (32 bits)
int opcode;
int r1;
int r2;
int r3;
int shamt;
int funct;
int imm;
int address;
int valueRS;
int valueRT;


int pc = 0;


int fetch()
{
        int instruction = memory[pc];
        pc++;
    return instruction;
}


void decode(int instruction)
{
    // Extract the 4-bit opcode (bits 31-28)
    opcode = (instruction >> 28) & 0xF;

    // R-Format: OPCODE (4), R1 (5), R2 (5), R3 (5), SHAMT (13)
    r1 = (instruction >> 23) & 0x1F;      // bits 27-23
    r2 = (instruction >> 18) & 0x1F;      // bits 22-18
    r3 = (instruction >> 13) & 0x1F;      // bits 17-13
    shamt = instruction & 0x1FFF;         // bits 12-0 (13 bits)

    // I-Format: OPCODE (4), R1 (5), R2 (5), IMMEDIATE (18)
    imm = instruction & 0x3FFFF;          // bits 17-0 (18 bits)

    // J-Format: OPCODE (4), ADDRESS (28)
    address = instruction & 0xFFFFFFF;    // bits 27-0 (28 bits)

}

void execute(int instruction)
{
    if (opcode == 0)
    {
        // R-Format: OPCODE (4), R1 (5), R2 (5), R3 (5), SHAMT (13)
    }
}





int main() {
    // Initialize memory if needed
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = 0;
    }
    
    // Set the path to your test file
    const char* testFile = "test.txt";
    
    printf("Reading assembly from: %s\n", testFile);
    
    // Parse the assembly file
    if (ReadFile(testFile, memory, &next_Empty_IA) == 0) {
        printf("Successfully parsed %d instructions\n", next_Empty_IA);
        
        // Print the memory contents after parsing
        printEntireMemory();
    } else {
        printf("Error parsing file\n");
        return 1;
    }
    
    while (pc != next_Empty_IA){
        int instruction = fetch();
        decode(instruction);
    }


    return 0;
}


