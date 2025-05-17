#ifndef PIPELINE_H
#define PIPELINE_H

#include <stdint.h>
#include <stdio.h>

typedef enum {
    STAGE_IF, STAGE_ID, STAGE_EX, STAGE_MEM, STAGE_WB
} PipelineStage;

typedef struct {
    int32_t instruction;
    int32_t pc;
    PipelineStage stage;
    int cycles_in_stage;
    int is_bubble;
    
    // Decoded values
    int opcode;
    int r1, r2, r3;
    int shamt;
    int imm;
    int address;
} PipelineInstruction;

typedef struct {
    PipelineInstruction stages[5];
    int cycle_count;
    int instructions_completed;
    int memory_busy;
    int total_instructions;
    int branch_taken;
    int jump_occurred;
    int expected_cycles;
} Pipeline;

// Function prototypes
void pipeline_init(Pipeline *p, int total_instr);
void pipeline_cycle(Pipeline *p);
void print_pipeline_state(Pipeline *p);
void flush_pipeline(Pipeline *p);
int pipeline_should_continue(Pipeline *p);

#endif