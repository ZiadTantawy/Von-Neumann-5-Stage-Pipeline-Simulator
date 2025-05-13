#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>

// Function prototypes
const char* getOpcodeBinary(const char* opcode);
void intToBinStr(char *out, int num, int bits);
int ReadFile(const char *filename, int32_t *memory, int *next_free_IA);

#endif /* PARSER_H */