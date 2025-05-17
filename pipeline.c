#include "pipeline.h"
#include "memory_register.h"
#include "cpu.h"
#include <stdio.h>

void pipeline_init(Pipeline *p, int total_instr) {
    for (int i = 0; i < 5; i++) {
        p->stages[i].instruction = 0;
        p->stages[i].pc = 0;
        p->stages[i].stage = i;
        p->stages[i].cycles_in_stage = 0;
        p->stages[i].is_bubble = 0;
    }
    p->cycle_count = 0;
    p->instructions_completed = 0;
    p->memory_busy = 0;
    p->total_instructions = total_instr;
    p->branch_taken = 0;
    p->jump_occurred = 0;
    p->expected_cycles = 7 + ((total_instr - 1) * 2);
}

void flush_pipeline(Pipeline *p) {
    // Flush instructions after branch/jump
    for (int i = 0; i < 5; i++) {
        if (p->stages[i].stage == STAGE_IF || p->stages[i].stage == STAGE_ID) {
            p->stages[i].instruction = 0;
            p->stages[i].is_bubble = 1;
        }
    }
    p->branch_taken = 1;
}

int pipeline_should_continue(Pipeline *p) {
    // Continue if we haven't reached the expected cycles OR
    // there are still instructions in the pipeline
    if (p->cycle_count < p->expected_cycles) {
        return 1;
    }
    
    // Check if any stages still have active instructions
    for (int i = 0; i < 5; i++) {
        if (p->stages[i].instruction && !p->stages[i].is_bubble) {
            return 1;
        }
    }
    
    return 0;
}

void print_pipeline_state(Pipeline *p) {
    const char* stage_names[] = {"IF", "ID", "EX", "MEM", "WB"};
    const char* opcodes[] = {"ADD", "SUB", "MULI", "ADDI", "BNE", "ANDI", 
                            "ORI", "J", "SLL", "SRL", "LW", "SW"};
    
    for (int i = 0; i < 5; i++) {
        printf("%s: ", stage_names[i]);
        if (p->stages[i].instruction && !p->stages[i].is_bubble) {
            printf("PC=%d %s", p->stages[i].pc, opcodes[p->stages[i].opcode]);
            
            if (i == STAGE_ID || i == STAGE_EX) {
                printf(" (%d/2)", p->stages[i].cycles_in_stage + 1);
            }
        } else if (p->stages[i].is_bubble) {
            printf("BUBBLE");
        } else {
            printf("---");
        }
        printf("\n");
    }
}

void pipeline_cycle(Pipeline *p) {
    if (!pipeline_should_continue(p)) {
        return;
    }

    p->cycle_count++;
    printf("\n=== Cycle %d ===\n", p->cycle_count);

    // WB stage
    if (p->stages[STAGE_WB].instruction && !p->stages[STAGE_WB].is_bubble) {
        printf("WB: Completed instruction @ PC=%d\n", p->stages[STAGE_WB].pc);
        p->instructions_completed++;
        p->stages[STAGE_WB].instruction = 0;
    }

    // MEM -> WB
    if (p->stages[STAGE_MEM].instruction && !p->stages[STAGE_MEM].is_bubble) {
        p->stages[STAGE_WB] = p->stages[STAGE_MEM];
        p->stages[STAGE_MEM].instruction = 0;
        p->memory_busy = 0;
    }

    // EX -> MEM
    if (p->stages[STAGE_EX].instruction && !p->stages[STAGE_EX].is_bubble) {
        p->stages[STAGE_EX].cycles_in_stage++;
        
        if (p->stages[STAGE_EX].cycles_in_stage >= 2) {
            if (!p->memory_busy) {
                p->stages[STAGE_MEM] = p->stages[STAGE_EX];
                p->stages[STAGE_EX].instruction = 0;
                p->memory_busy = 1;
                
                execute(p->stages[STAGE_MEM].instruction,
                       p->stages[STAGE_MEM].opcode,
                       p->stages[STAGE_MEM].r1,
                       p->stages[STAGE_MEM].r2,
                       p->stages[STAGE_MEM].r3,
                       p->stages[STAGE_MEM].shamt,
                       p->stages[STAGE_MEM].imm,
                       p->stages[STAGE_MEM].address);
                
                if (p->stages[STAGE_MEM].opcode == 4 || p->stages[STAGE_MEM].opcode == 7) {
                    flush_pipeline(p);
                }
            }
        }
    }

    // ID -> EX
    if (p->stages[STAGE_ID].instruction && !p->stages[STAGE_ID].is_bubble) {
        p->stages[STAGE_ID].cycles_in_stage++;
        
        if (p->stages[STAGE_ID].cycles_in_stage >= 2) {
            p->stages[STAGE_EX] = p->stages[STAGE_ID];
            p->stages[STAGE_EX].cycles_in_stage = 0;
            p->stages[STAGE_ID].instruction = 0;
        }
    }

    // IF -> ID
    if (!p->branch_taken) {
        if (p->stages[STAGE_IF].instruction && !p->stages[STAGE_IF].is_bubble) {
            p->stages[STAGE_ID] = p->stages[STAGE_IF];
            p->stages[STAGE_ID].cycles_in_stage = 0;
            p->stages[STAGE_IF].instruction = 0;
        }
        
        if ((p->cycle_count % 2 == 1) && !p->memory_busy && 
            (p->instructions_completed < p->total_instructions)) {
            int32_t fetched = fetch();
            if (fetched != -1) {
                p->stages[STAGE_IF].instruction = fetched;
                p->stages[STAGE_IF].pc = reg_file.PC - 1;
                p->stages[STAGE_IF].is_bubble = 0;
                
                decode(p->stages[STAGE_IF].instruction,
                      &p->stages[STAGE_IF].opcode,
                      &p->stages[STAGE_IF].r1,
                      &p->stages[STAGE_IF].r2,
                      &p->stages[STAGE_IF].r3,
                      &p->stages[STAGE_IF].shamt,
                      &p->stages[STAGE_IF].imm,
                      &p->stages[STAGE_IF].address);
            }
        }
    } else {
        p->branch_taken = 0;
    }

    print_pipeline_state(p);
}