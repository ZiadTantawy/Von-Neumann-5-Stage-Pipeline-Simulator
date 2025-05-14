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
    extern const int32_t R0;
    extern int32_t R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15;
    extern int32_t R16, R17, R18, R19, R20, R21, R22, R23, R24, R25, R26, R27, R28, R29, R30, R31;
    
    // Cast the const pointer to non-const to match the array type
    int32_t *registers[32] = {
        (int32_t*)&R0, &R1, &R2, &R3, &R4, &R5, &R6, &R7, &R8, &R9, &R10, &R11, &R12, &R13, &R14, &R15,
        &R16, &R17, &R18, &R19, &R20, &R21, &R22, &R23, &R24, &R25, &R26, &R27, &R28, &R29, &R30, &R31
    };

    switch(opcode){
        case 0:
            // ADD
            *registers[r1] = *registers[r2] + *registers[r3];
            break;
        case 1:
            // SUB
            *registers[r1] = *registers[r2] - *registers[r3];
            break;
        case 2:
            // MULI
            *registers[r1] = *registers[r2] * imm;
            break;
        case 3:
            // ADDI
            *registers[r1] = *registers[r2] + imm;
            break;
        case 4:
            // BNE
            if (*registers[r1] != *registers[r2]) {
                pc = pc + imm;
            }
            break;
        case 5:
            // ANDI
            *registers[r1] = *registers[r2] & imm;
            break;
        case 6:
            // ORI
            *registers[r1] = *registers[r2] | imm;
            break;
        case 7:
            // J
            pc = address;
            break;
        case 8:
            // SLL
            *registers[r1] = *registers[r2] << shamt;
            break;
        case 9:
            // SRL
            *registers[r1] = *registers[r2] >> shamt;
            break;
        case 10:
            // LW
            *registers[r1] = memory[*registers[r2] + imm];
            break;
        case 11:
            // SW
            memory[*registers[r2] + imm] = *registers[r1];
            break;
    }
    

    *((int32_t*)&R0) = 0;
}

void printState(int instruction, int oldPc) {
    printf("\n==== EXECUTION CYCLE %d ====\n", oldPc);
    
    // Print the instruction being processed
    printf("PC: %d\n", oldPc);
    printf("Instruction: %d (0x%08X)\n", instruction, instruction);
    
    // Print the decoded information
    printf("\n-- DECODE STAGE --\n");
    printf("Opcode: %d\n", opcode);
    
    // Print different fields based on instruction format
    if (opcode <= 1) {  // R-format (ADD, SUB)
        printf("Format: R-type\n");
        printf("r1: %d, r2: %d, r3: %d, shamt: %d\n", r1, r2, r3, shamt);
    } 
    else if (opcode >= 2 && opcode <= 6) {  // I-format (MULI, ADDI, BNE, ANDI, ORI)
        printf("Format: I-type\n");
        printf("r1: %d, r2: %d, immediate: %d\n", r1, r2, imm);
    }
    else if (opcode == 7) {  // J-format (J)
        printf("Format: J-type\n");
        printf("address: %d\n", address);
    }
    else if (opcode >= 8 && opcode <= 9) {  // Shift instructions (SLL, SRL)
        printf("Format: Shift-type\n");
        printf("r1: %d, r2: %d, shamt: %d\n", r1, r2, shamt);
    }
    else if (opcode >= 10 && opcode <= 11) {  // Memory instructions (LW, SW)
        printf("Format: Memory-type\n");
        printf("r1: %d, r2: %d, offset: %d\n", r1, r2, imm);
    }
    
    // Print the operation being executed
    printf("\n-- EXECUTE STAGE --\n");
    switch(opcode) {
        case 0: printf("Operation: ADD R%d, R%d, R%d\n", r1, r2, r3); break;
        case 1: printf("Operation: SUB R%d, R%d, R%d\n", r1, r2, r3); break;
        case 2: printf("Operation: MULI R%d, R%d, %d\n", r1, r2, imm); break;
        case 3: printf("Operation: ADDI R%d, R%d, %d\n", r1, r2, imm); break;
        case 4: printf("Operation: BNE R%d, R%d, %d\n", r1, r2, imm); break;
        case 5: printf("Operation: ANDI R%d, R%d, %d\n", r1, r2, imm); break;
        case 6: printf("Operation: ORI R%d, R%d, %d\n", r1, r2, imm); break;
        case 7: printf("Operation: J %d\n", address); break;
        case 8: printf("Operation: SLL R%d, R%d, %d\n", r1, r2, shamt); break;
        case 9: printf("Operation: SRL R%d, R%d, %d\n", r1, r2, shamt); break;
        case 10: printf("Operation: LW R%d, %d(R%d)\n", r1, imm, r2); break;
        case 11: printf("Operation: SW R%d, %d(R%d)\n", r1, imm, r2); break;
        default: printf("Unknown operation\n");
    }
    
    // Print register values
    printf("\n-- REGISTER VALUES --\n");
    extern const int32_t R0;
    extern int32_t R1, R2, R3, R4, R5, R6, R7, R8;
    extern int32_t R9, R10, R11, R12, R13, R14, R15, R16;
    extern int32_t R17, R18, R19, R20, R21, R22, R23, R24;
    extern int32_t R25, R26, R27, R28, R29, R30, R31;
    
    printf("R0=%d, R1=%d, R2=%d, R3=%d\n", R0, R1, R2, R3);
    printf("R4=%d, R5=%d, R6=%d, R7=%d\n", R4, R5, R6, R7);
    printf("R8=%d, R9=%d, R10=%d, R11=%d\n", R8, R9, R10, R11);
    printf("R12=%d, R13=%d, R14=%d, R15=%d\n", R12, R13, R14, R15);
    printf("R16=%d, R17=%d, R18=%d, R19=%d\n", R16, R17, R18, R19);
    printf("R20=%d, R21=%d, R22=%d, R23=%d\n", R20, R21, R22, R23);
    printf("R24=%d, R25=%d, R26=%d, R27=%d\n", R24, R25, R26, R27);
    printf("R28=%d, R29=%d, R30=%d, R31=%d\n", R28, R29, R30, R31);
    
    printf("\nNext PC: %d\n", pc);
    printf("============================\n\n");
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
        
        printf("\nStarting program execution...\n");
        
        // Execute the program
        while (pc < next_Empty_IA) {
            int oldPc = pc;  // Save PC before fetch increments it
            int instruction = fetch();
            decode(instruction);
            execute(instruction);
            
            // Print the execution state
            printState(instruction, oldPc);
            
            // Check if we need to stop (e.g., hit a special halt instruction or predefined limit)
            if (pc >= next_Empty_IA || pc < 0) {
                printf("Program execution complete.\n");
                break;
            }
        }
        
        printf("\nFinal register values:\n");
        
        extern const int32_t R0;
        extern int32_t R1, R2, R3, R4, R5, R6, R7, R8;
        extern int32_t R9, R10, R11, R12, R13, R14, R15, R16;
        extern int32_t R17, R18, R19, R20, R21, R22, R23, R24;
        extern int32_t R25, R26, R27, R28, R29, R30, R31;
        
        printf("R0=%d, R1=%d, R2=%d, R3=%d\n", R0, R1, R2, R3);
        printf("R4=%d, R5=%d, R6=%d, R7=%d\n", R4, R5, R6, R7);
        printf("R8=%d, R9=%d, R10=%d, R11=%d\n", R8, R9, R10, R11);
        printf("R12=%d, R13=%d, R14=%d, R15=%d\n", R12, R13, R14, R15);
        printf("R16=%d, R17=%d, R18=%d, R19=%d\n", R16, R17, R18, R19);
        printf("R20=%d, R21=%d, R22=%d, R23=%d\n", R20, R21, R22, R23);
        printf("R24=%d, R25=%d, R26=%d, R27=%d\n", R24, R25, R26, R27);
        printf("R28=%d, R29=%d, R30=%d, R31=%d\n", R28, R29, R30, R31);
        
        // Print final memory state if needed
        printf("\nFinal memory state:\n");
        printEntireMemory();
        
    } else {
        printf("Error parsing file\n");
        return 1;
    }

    return 0;
}


