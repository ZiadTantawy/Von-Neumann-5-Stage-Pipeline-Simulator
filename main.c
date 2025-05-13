#include <stdio.h>
#include <stdint.h>
#include "parser.h"
#include "memory_register.h"

// External functions from your other files
extern int ReadFile(const char *filename, int32_t *memory, int *next_free_IA);
extern void printEntireMemory();
extern int32_t memory[2048];
extern int next_Empty_IA;

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
    
    return 0;
}