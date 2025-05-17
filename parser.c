#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "parser.h"
#include "memory_register.h"

// Updated: opcode -> 4 bits
const char* getOpcodeBinary(const char* opcode) {
    if (strcmp(opcode, "ADD") == 0)   return "0000";
    if (strcmp(opcode, "SUB") == 0)   return "0001";
    if (strcmp(opcode, "MULI") == 0)   return "0010";
    if (strcmp(opcode, "ADDI") == 0)  return "0011";
    if (strcmp(opcode, "BNE") == 0)   return "0100";
    if (strcmp(opcode, "ANDI") == 0)   return "0101";
    if (strcmp(opcode, "ORI") == 0)    return "0110";
    if (strcmp(opcode, "J") == 0)  return "0111";
    if (strcmp(opcode, "SLL") == 0)  return "1000";
    if (strcmp(opcode, "SRL") == 0)   return "1001";
    if (strcmp(opcode, "LW") == 0)   return "1010";
    if (strcmp(opcode, "SW") == 0)    return "1011";
    return NULL;
}

void intToBinStr(char *out, int num, int bits) {
    for (int i = bits - 1; i >= 0; i--)
        out[bits - 1 - i] = ((num >> i) & 1) ? '1' : '0';
    out[bits] = '\0';
}

// Helper function to convert register string (e.g., "R1") to int
int reg_num(const char* reg) {
    if ((reg[0] == 'R' || reg[0] == 'r') && isdigit(reg[1]))
        return atoi(reg + 1);
    return 0;
}

int ReadFile(const char *filename, int32_t *memory, int *next_free_IA) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("Error opening %s\n", filename);
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (line[0] == '\n' || line[0] == '#' || strlen(line) <= 1)
            continue;

        line[strcspn(line, "\r\n")] = 0;  // remove newline

        char *tokens[5];
        int tokenCount = 0;
        // Tokenize (skip leading spaces/tabs)
        char *tok = strtok(line, " \t");
        while (tok && tokenCount < 5)
            tokens[tokenCount++] = tok, tok = strtok(NULL, " \t");
        // Debug print for tokens
        for (int i = 0; i < tokenCount; i++) {
            printf("Token[%d]: '%s'\n", i, tokens[i]);
        }

        // Get opcode binary and as int
        const char *opcodeBin = getOpcodeBinary(tokens[0]);
        if (!opcodeBin) {
            printf("Unknown opcode: %s\n", tokens[0]);
            continue;
        }
        int opcode = (int)strtol(opcodeBin, NULL, 2);

        int instr = 0;
        // R-format: ADD, SUB, SLL, SRL
        if (opcode == 0 || opcode == 1 || opcode == 8 || opcode == 9) {
            int r1 = reg_num(tokens[1]);
            int r2 = reg_num(tokens[2]);
            int r3 = reg_num(tokens[3]);
            int shamt = 0;
            if (opcode == 8 || opcode == 9) { // SLL, SRL: tokens[3] is shift amount
                r3 = 0;
                shamt = atoi(tokens[3]);
            }
            instr = (opcode << 28) | (r1 << 23) | (r2 << 18) | (r3 << 13) | (shamt & 0x1FFF);
        }
        // I-format: MULI, ADDI, BNE, ANDI, ORI, LW, SW
        else if ((opcode >= 2 && opcode <= 6) || opcode == 10 || opcode == 11) {
            int r1 = reg_num(tokens[1]);
            int r2 = reg_num(tokens[2]);
            int imm = atoi(tokens[3]);
            // Sign extend 18-bit immediate to 32 bits
            if (imm < 0) {
                imm = imm & 0x3FFFF;  // Keep only 18 bits
            }
            instr = (opcode << 28) | (r1 << 23) | (r2 << 18) | (imm & 0x3FFFF);
            printf("I-Format: opcode=%d r1=%d r2=%d imm=%d instr=0x%08X\n", opcode, r1, r2, imm, instr);
        }
        // J-format: J
        else if (opcode == 7) {
            int address = atoi(tokens[1]);
            instr = (opcode << 28) | (address & 0xFFFFFFF);
        }
        else {
            printf("Unknown or unsupported opcode: %s\n", tokens[0]);
            continue;
        }

        memory[*next_free_IA] = instr;
        printf("Loaded: %s -> 0x%08X at address %d\n", line, instr, *next_free_IA);
        (*next_free_IA)++;
    }

    fclose(fp);
    return 0;
}