#include <stdio.h>
#include <stdint.h>
#include "parser.h"
#include "memory_register.h"

// Pipeline stage representation
typedef struct {
    int valid;
    int32_t instruction;
    int pc;
    int opcode, r1, r2, r3, shamt, imm, address;
    int alu_result, mem_data;
    int stage_cycles;
} PipelineStage;

// Declare kol el stages
PipelineStage IF = {0}, ID = {0}, EX = {0}, MEM = {0}, WB = {0};

int clockCycle = 1;
int32_t PC = 0;

// Mapping opcode to mnemonic for better printing
const char* opcode_names[] = {
    "ADD", "SUB", "MULI", "ADDI", "BNE", "ANDI",
    "ORI", "J", "SLL", "SRL", "LW", "SW"
};

// Sign extend 18-bit immediate
int32_t sign_extend_18(int32_t imm) {
    if (imm & (1 << 17)) imm |= ~((1 << 18) - 1);
    return imm;
}

// Forward value from later pipeline stages if available -- handling hazards (preventing stalls)
int32_t get_forwarded_value(int reg, int32_t orig_val) {
    if (MEM.valid && MEM.r1 == reg && MEM.opcode != 11 && MEM.opcode != 7)
        return (MEM.opcode == 10) ? MEM.mem_data : MEM.alu_result;
    if (WB.valid && WB.r1 == reg && WB.opcode != 11 && WB.opcode != 7)
        return (WB.opcode == 10) ? WB.mem_data : WB.alu_result;
    return orig_val;
}

// Fetch kol instruction from memory
void fetch() {
    IF.instruction = memory[PC];
    IF.pc = PC;
    IF.valid = 1;
    PC++;
}

// Decode kol el instruction fields
void decode_instruction(PipelineStage* stage) {
    int inst = stage->instruction;
    stage->opcode = (inst >> 28) & 0xF;
    stage->r1 = (inst >> 23) & 0x1F;
    stage->r2 = (inst >> 18) & 0x1F;
    stage->r3 = (inst >> 13) & 0x1F;
    stage->shamt = inst & 0x1FFF;
    stage->imm = inst & 0x3FFFF;
    stage->address = inst & 0x0FFFFFFF;

    if ((stage->opcode >= 2 && stage->opcode <= 6) || stage->opcode == 10 || stage->opcode == 11)
        stage->imm = sign_extend_18(stage->imm);
}

// Perform ALU or control operation
void execute() {
    int32_t v1 = get_forwarded_value(EX.r1, get_reg(EX.r1));
    int32_t v2 = get_forwarded_value(EX.r2, get_reg(EX.r2));
    int32_t v3 = get_forwarded_value(EX.r3, get_reg(EX.r3));
    int32_t imm = EX.imm;
    int sh = EX.shamt;

    printf("[EX] Executing: opcode=%d (%s), r1=R%d, r2=R%d, r3=R%d, imm=%d, shamt=%d\n",
           EX.opcode, opcode_names[EX.opcode], EX.r1, EX.r2, EX.r3, imm, sh);

    switch (EX.opcode) {
        case 0: EX.alu_result = v2 + v3;
                printf("[EX] ADD: R%d = %d + %d = %d\n", EX.r1, v2, v3, EX.alu_result); break;
        case 1: EX.alu_result = v2 - v3;
                printf("[EX] SUB: R%d = %d - %d = %d\n", EX.r1, v2, v3, EX.alu_result); break;
        case 2: EX.alu_result = v2 * imm;
                printf("[EX] MULI: R%d = %d * %d = %d\n", EX.r1, v2, imm, EX.alu_result); break;
        case 3: EX.alu_result = v2 + imm;
                printf("[EX] ADDI: R%d = %d + %d = %d\n", EX.r1, v2, imm, EX.alu_result); break;
        case 4: 
            if (v1 != v2) {
                int old_pc = PC;
                PC = EX.pc + 1 + imm;
                IF.valid = 0;
                ID.valid = 0;
                printf("[EX] BNE: %d != %d → PC = %d + 1 + %d = %d (was %d)\n", v1, v2, EX.pc, imm, PC, old_pc);
            } else {
                printf("[EX] BNE: %d == %d → no branch\n", v1, v2);
            }
            break;
        case 5: EX.alu_result = v2 & imm;
                printf("[EX] ANDI: R%d = %d & %d = %d\n", EX.r1, v2, imm, EX.alu_result); break;
        case 6: EX.alu_result = v2 | imm;
                printf("[EX] ORI: R%d = %d | %d = %d\n", EX.r1, v2, imm, EX.alu_result); break;
        case 7: PC = EX.address;
                IF.valid = 0;
                ID.valid = 0;
                printf("[EX] JUMP: PC = %d\n", PC); break;
        case 8: EX.alu_result = v2 << sh;
                printf("[EX] SLL: R%d = %d << %d = %d\n", EX.r1, v2, sh, EX.alu_result); break;
        case 9: EX.alu_result = ((uint32_t)v2) >> sh;
                printf("[EX] SRL: R%d = %d >> %d = %d\n", EX.r1, v2, sh, EX.alu_result); break;
        case 10: EX.alu_result = v2 + imm;
                 printf("[EX] LW: Address = R%d (%d) + %d = %d\n", EX.r2, v2, imm, EX.alu_result); break;
        case 11: EX.alu_result = v2 + imm;
                 printf("[EX] SW: Address = R%d (%d) + %d = %d\n", EX.r2, v2, imm, EX.alu_result); break;
    }
}

// Perform memory read/write 
void memory_access() {
    if (MEM.opcode == 10) {
        MEM.mem_data = memory[MEM.alu_result];
        printf("[MEM] Loaded value %d from memory[%d]\n", MEM.mem_data, MEM.alu_result);
    }
    if (MEM.opcode == 11) {
        int32_t data = get_reg(MEM.r1);
        memory[MEM.alu_result] = data;
        printf("[MEM] Stored value %d into memory[%d]\n", data, MEM.alu_result);
    }
}

// Write result back to register file
void write_back() {
    if (!WB.valid) return;

    switch (WB.opcode) {
        case 0: case 1: case 2: case 3:
        case 5: case 6: case 8: case 9:
            set_reg(WB.r1, WB.alu_result);
            printf("[WB] R%d = %d (ALU result)\n", WB.r1, WB.alu_result);
            break;
        case 10:
            set_reg(WB.r1, WB.mem_data);
            printf("[WB] R%d = %d (Memory loaded)\n", WB.r1, WB.mem_data);
            break;
        default:
            printf("[WB] No write-back for opcode %d (%s)\n", WB.opcode, opcode_names[WB.opcode]);
    }
}

// Stage-by-stage status printer display kol el stages
void print_pipeline() {
    printf("\n=== Cycle %d ===\n", clockCycle);
    printf("PC = %d\n", PC);

    if (IF.valid) printf("IF: Fetched 0x%08X (%s) @ PC=%d\n", IF.instruction, opcode_names[(IF.instruction >> 28) & 0xF], IF.pc);
    else printf("IF: -\n");

    if (ID.valid) printf("ID: opcode=%d (%s) r1=R%d r2=R%d r3=R%d imm=%d\n",
                         ID.opcode, opcode_names[ID.opcode], ID.r1, ID.r2, ID.r3, ID.imm);
    else printf("ID: -\n");

    if (EX.valid) printf("EX: Ready to execute opcode %d (%s)\n", EX.opcode, opcode_names[EX.opcode]);
    else printf("EX: -\n");

    if (MEM.valid) printf("MEM: Processing opcode %d (%s)\n", MEM.opcode, opcode_names[MEM.opcode]);
    else printf("MEM: -\n");

    if (WB.valid) printf("WB: Writing result of opcode %d (%s)\n", WB.opcode, opcode_names[WB.opcode]);
    else printf("WB: -\n");
}

// Progress pipeline forward by one cycle
void advance_pipeline() {
    if (WB.valid && WB.stage_cycles >= 1) {
        write_back();
        WB.valid = 0;
    }

    if (MEM.valid && MEM.stage_cycles >= 1) {
        memory_access();
        WB = MEM;
        WB.valid = 1;
        WB.stage_cycles = 0;
        MEM.valid = 0;
    }

    if (EX.valid && EX.stage_cycles >= 2) {
        execute();
        MEM = EX;
        MEM.valid = 1;
        MEM.stage_cycles = 0;
        EX.valid = 0;
    }

    if (ID.valid && ID.stage_cycles >= 2) {
        EX = ID;
        EX.valid = 1;
        EX.stage_cycles = 0;
        ID.valid = 0;
    }

    if (IF.valid) {
        ID = IF;
        ID.valid = 1;
        ID.stage_cycles = 0;
        IF.valid = 0;
        decode_instruction(&ID);
    }

    if ((clockCycle - 1) % 2 == 0 && PC < next_Empty_IA) {
        fetch();
    }

    if (ID.valid) ID.stage_cycles++;
    if (EX.valid) EX.stage_cycles++;
    if (MEM.valid) MEM.stage_cycles++;
    if (WB.valid) WB.stage_cycles++;
}

int main() {
    for (int i = 0; i < MEMORY_SIZE; i++) memory[i] = 0;
    PC = 0;
    clockCycle = 1;

    const char* file = "test.txt";
    if (ReadFile(file, memory, &next_Empty_IA) != 0) {
        printf("Failed to read %s\n", file);
        return 1;
    }

    printf("\nStarting simulation...\n");

    int lastPrintedCycle = 0;
    while (PC < next_Empty_IA || IF.valid || ID.valid || EX.valid || MEM.valid || WB.valid) {
        advance_pipeline();
        if (PC < next_Empty_IA || IF.valid || ID.valid || EX.valid || MEM.valid || WB.valid) {
            print_pipeline();
            lastPrintedCycle = clockCycle;
        }
        clockCycle++;
    }
    printf("\nExecution complete in %d cycles\n", lastPrintedCycle);
    printEntireMemory();
    printf("\n========= REGISTER FILE =========\n");
    for (int i = 0; i < 32; i++) {
        printf("R%-2d = %d\n", i, get_reg(i));
    }
    printf("===============================\n");
    return 0;
}
