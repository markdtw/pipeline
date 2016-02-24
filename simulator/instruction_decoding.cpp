#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "main.h"

void naming(Instruction *inst) {
    if (inst->opcode==0x00) {
        if (inst->funct==0x20) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "ADD", 3);
        } else if (inst->funct==0x22) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "SUB", 3);
        } else if (inst->funct==0x24) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "AND", 3);
        } else if (inst->funct==0x25) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "OR", 2);
        } else if (inst->funct==0x26) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "XOR", 3);
        } else if (inst->funct==0x27) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "NOR", 3);
        } else if (inst->funct==0x28) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "NAND", 4);
        } else if (inst->funct==0x2A) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "SLT", 3);
        } else if (inst->funct==0x00) {
            if (inst->rt==0 && inst->rd==0 && inst->c==0) {
                memset(inst->operation, 0, sizeof(inst->operation));
                strncpy(inst->operation, "NOP", 3);
            } else {
                memset(inst->operation, 0, sizeof(inst->operation));
                strncpy(inst->operation, "SLL", 3);
            }
        } else if (inst->funct==0x02) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "SRL", 3);
        } else if (inst->funct==0x03) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "SRA", 3);
        } else if (inst->funct==0x08) {
            memset(inst->operation, 0, sizeof(inst->operation));
            strncpy(inst->operation, "JR", 2);
        }
    } else if (inst->opcode==0x08) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "ADDI", 4);
    } else if (inst->opcode==0x23) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "LW", 2);
    } else if (inst->opcode==0x21) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "LH", 2);
    } else if (inst->opcode==0x25) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "LHU", 3);
    } else if (inst->opcode==0x20) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "LB", 2);
    } else if (inst->opcode==0x24) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "LBU", 3);
    } else if (inst->opcode==0x2B) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "SW", 2);
    } else if (inst->opcode==0x29) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "SH", 2);
    } else if (inst->opcode==0x28) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "SB", 2);
    } else if (inst->opcode==0x0F) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "LUI", 3);
    } else if (inst->opcode==0x0C) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "ANDI", 4);
    } else if (inst->opcode==0x0D) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "ORI", 3);
    } else if (inst->opcode==0x0E) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "NORI", 4);
    } else if (inst->opcode==0x0A) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "SLTI", 4);
    } else if (inst->opcode==0x04) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "BEQ", 3);
    } else if (inst->opcode==0x05) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "BNE", 3);
    } else if (inst->opcode==0x02) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "J", 1);
    } else if (inst->opcode==0x03) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "JAL", 3);
    } else if (inst->opcode==0x3F) {
        memset(inst->operation, 0, sizeof(inst->operation));
        strncpy(inst->operation, "HALT", 4);
    }
}

int RTypeInstruction(Instruction code, int *reg) {
    int ALU_value = 0;
    if (code.funct==0x20) {
        // add
        ALU_value = code.rs_regvalue + code.rt_regvalue;
    } else if (code.funct==0x22) {
        // sub
        ALU_value = code.rs_regvalue - code.rt_regvalue;
    } else if (code.funct==0x24) {
        // and
        ALU_value = code.rs_regvalue & code.rt_regvalue;
    } else if (code.funct==0x25) {
        // or
        ALU_value = code.rs_regvalue | code.rt_regvalue;
    } else if (code.funct==0x26) {
        // xor
        ALU_value = code.rs_regvalue ^ code.rt_regvalue;
    } else if (code.funct==0x27) {
        // nor
        ALU_value = ~(code.rs_regvalue | code.rt_regvalue);
    } else if (code.funct==0x28) {
        // nand
        ALU_value = ~(code.rs_regvalue & code.rt_regvalue);
    } else if (code.funct==0x2A) {
        // slt
        ALU_value = code.rs_regvalue < code.rt_regvalue;
    } else if (code.funct==0x00) {
        // sll
        ALU_value = (unsigned int)code.rt_regvalue << code.c;
    } else if (code.funct==0x02) {
        // srl
        ALU_value = (unsigned int)code.rt_regvalue >> code.c;
    } else if (code.funct==0x03) {
        // sra
        ALU_value = code.rt_regvalue >> code.c;
    } else ALU_value = 0;
    return ALU_value;
}

int ITypeInstruction(Instruction code, int *reg) {
    int ALU_value = 0;
    if (code.opcode==0x08 || code.opcode==0x23 || code.opcode==0x21 || code.opcode==0x25 ||
            code.opcode==0x20 || code.opcode==0x24 || code.opcode==0x2B || code.opcode==0x29 || code.opcode==0x28) {
        // addi, lw, lh, lhu, lb, lbu, sw, sh, sb
        ALU_value = code.rs_regvalue + code.c;
    } else if (code.opcode==0x0F) {
        // lui
        ALU_value = code.c<<16;
    } else if (code.opcode==0x0C) {
        // andi
        ALU_value = (code.rs_regvalue & (unsigned short) code.c);
    } else if (code.opcode==0x0D) {
        // ori
        ALU_value = (code.rs_regvalue | (unsigned short) code.c);
    } else if (code.opcode==0x0E) {
        // nori
        ALU_value = ~(code.rs_regvalue | (unsigned short)code.c);
    } else if (code.opcode==0x0A) {
        // slti
        ALU_value = code.rs_regvalue < code.c;
    } else ALU_value = 0;
    return ALU_value;
}
