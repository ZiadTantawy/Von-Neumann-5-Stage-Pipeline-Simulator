#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

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
        // Tokenize
        char *tok = strtok(line, " ");
        while (tok && tokenCount < 5)
            tokens[tokenCount++] = tok, tok = strtok(NULL, " ");

        // Get opcode binary
        const char *opcodeBin = getOpcodeBinary(tokens[0]);
        if (!opcodeBin) {
            printf("Unknown opcode: %s\n", tokens[0]);
            continue;
        }

        char binInstr[33] = {0}; // Instruction string
        strcat(binInstr, opcodeBin); // Add opcode (4 bits)

        // Handle instruction types by number of operands
        if (strcmp(tokens[0], "ADD") == 0 || strcmp(tokens[0], "SUB") == 0 || strcmp(tokens[0], "MUL") == 0) {
            // R-type: opcode + r1 + r2 + r3 + 13 shamt zeros
            for (int i = 1; i <= 3; i++) {
                char regBin[6];
                int reg = atoi(tokens[i] + 1);
                intToBinStr(regBin, reg, 5);
                strcat(binInstr, regBin);
            }
            strcat(binInstr, "0000000000000"); // 13-bit unused
        } 
        else if (strcmp(tokens[0], "ADDI") == 0 || strcmp(tokens[0], "MULI") == 0 || strcmp(tokens[0], "MOVI") == 0) {
            // I-type: opcode + r1 + r2 + imm (18)
            for (int i = 1; i <= 2; i++) {
                char regBin[6];
                int reg = atoi(tokens[i] + 1);
                intToBinStr(regBin, reg, 5);
                strcat(binInstr, regBin);
            }
            char immBin[19];
            int imm = atoi(tokens[3]);
            if (imm < 0) imm = (1 << 18) + imm; // handle negative imm
            intToBinStr(immBin, imm, 18);
            strcat(binInstr, immBin);
        } 
        else if (strcmp(tokens[0], "JMP") == 0) {
            // J-type: opcode + 28-bit address
            char addrBin[29];
            int addr = atoi(tokens[1]);
            intToBinStr(addrBin, addr, 28);
            strcat(binInstr, addrBin);
        } 
        else {
            printf("Unsupported instruction: %s\n", tokens[0]);
            continue;
        }

        // Convert binary string to signed int
        int32_t instrValue = (int32_t)strtol(binInstr, NULL, 2);
        memory[*next_free_IA] = instrValue;
        (*next_free_IA)++;
    }

    fclose(fp);
    return 0;
}