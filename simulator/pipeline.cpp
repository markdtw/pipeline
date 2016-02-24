#include<cstdio>
#include<cstdlib>
#include<cstring>
#define FORW_RS 1
#define FORW_RT 2
#define FORW_L_RS 3
#define FORW_L_RT 4
#define FORW_S_RS 5
#define FORW_S_RT 6
#define PREEX_RS 1
#define PREEX_RT 2
#define PREDM_RS 3
#define PREDM_RT 4
typedef struct Instruction{
    int opcode, rs, rt, rd, JSc, funct, INST;
    int rs_regvalue, rt_regvalue;
    int EXEbuff, MEMbuff;
    bool stall, flush, forwarding;
    char pipeline_fwd[50];
    char operation[5];
    short c;
}Instruction;
int halt_buff;
bool halt_error;
void printsnapshot(FILE *snapshot, int cycle, int *reg, int pc) {
    fprintf(snapshot, "cycle %d\n", cycle);
    for(int i=0; i<32; i++) {
        fprintf(snapshot, "$%02d: 0x%08X\n", i, reg[i]);
    }
    fprintf(snapshot, "PC: 0x%08X\n", pc);
}
void printpipeline(FILE *snapshot, Instruction ID, Instruction IF, Instruction EXE, Instruction MEM, Instruction WB) {
    fprintf(snapshot, "IF: 0x%08X", IF.INST);
    if (ID.stall) {
        fprintf(snapshot, " to_be_stalled");
    }
    if (ID.flush) {
        fprintf(snapshot, " to_be_flushed");
    }
    fprintf(snapshot, "\n");
    if (ID.INST==0) {
        fprintf(snapshot, "ID: NOP\n");
    } else {
        fprintf(snapshot, "ID: %s", ID.operation);
        if (ID.stall) {
            fprintf(snapshot, " to_be_stalled");
        }
        if (ID.forwarding && (ID.opcode==0x04 || ID.opcode==0x05 || (ID.opcode==0x00 && ID.funct==0x08))) {
            fprintf(snapshot, "%s", ID.pipeline_fwd);
        }
        fprintf(snapshot, "\n");
    }

    if (EXE.INST==0) {
        fprintf(snapshot, "EX: NOP\n");
    } else {
        fprintf(snapshot, "EX: %s", EXE.operation);
        if (EXE.forwarding && EXE.opcode!=0x04 && EXE.opcode!=0x05 && EXE.funct!=0x08) {
            fprintf(snapshot, "%s", EXE.pipeline_fwd);
        }
        fprintf(snapshot, "\n");
    }

    if (MEM.INST==0) {
        fprintf(snapshot, "DM: NOP\n");
    } else {
        fprintf(snapshot, "DM: %s\n", MEM.operation);
    }
    if (WB.INST==0) {
        fprintf(snapshot, "WB: NOP\n");
    } else {
        fprintf(snapshot, "WB: %s\n", WB.operation);
    }
    fprintf(snapshot, "\n\n");
}
int chartoint(char *memory, int start, int wordsize) {
    if (wordsize == 4) {
        return ((memory[start]<<24)&0xFF000000) | ((memory[start+1]<<16)&0x00FF0000) | ((memory[start+2]<<8)&0x0000FF00) | (memory[start+3]&0x000000FF);
    } else {
        return ((memory[start]<<8)&0x0000FF00) | (memory[start+1]&0x000000FF);
    }
}
int checkWrite0(Instruction inst) {
    if (inst.opcode==0x00) {
        /* check R-type */
        if (inst.funct!=0x08) {
            if (inst.funct==0x00) {
                /* check sll */
                if (inst.rd==0 && (inst.rt!=0 || inst.c!=0)) {
                    return 0;
                }
            } else return inst.rd;
        }
    } else if (inst.opcode!=0x02 && inst.opcode!=0x03 && inst.opcode!=0x3F) {
        /* check I-type */
        if (inst.opcode!=0x2B && inst.opcode!=0x29 && inst.opcode!=0x28 && inst.opcode!=0x04 && inst.opcode!=0x05) {
            return inst.rt;
        }
    }
    return 1;
}
bool MemAddrOverflow(Instruction inst, int exebuff) {
    if (inst.opcode==0x20 || inst.opcode==0x24 || inst.opcode==0x28) {
        if (exebuff > 1023 || exebuff < 0) {
            return true;
        }
    } else if (inst.opcode==0x23 || inst.opcode==0x2B) {
        if (exebuff > 1023 || exebuff+3 > 1023 || exebuff < 0) {
            return true;
        }
    } else if (inst.opcode==0x21 || inst.opcode==0x25 || inst.opcode==0x29) {
        if (exebuff > 1023 || exebuff+1 > 1023 || exebuff < 0) {
            return true;
        }
    }
    return false;
}
bool MemMissAlign(Instruction inst, int exebuff) {
    if (inst.opcode==0x23 || inst.opcode==0x2B) {
        if ((exebuff)%4 != 0) {
            return true;
        }
    } else if (inst.opcode==0x21 || inst.opcode==0x25 || inst.opcode==0x29) {
        if ((exebuff)%2 != 0) {
            return true;
        }
    }
    return false;
}
bool checkNumOverflow(Instruction inst) {
    int sum=0, augend=0, addend=0;
    if (inst.opcode==0x00) {
        /* check R-type */
        if (inst.funct==0x20) {
            augend = inst.rs_regvalue;
            addend = inst.rt_regvalue;
        } else if (inst.funct==0x22) {
            augend = inst.rs_regvalue;
            addend = inst.rt_regvalue * (-1);
        }
        /* check I-type */
    } else if (inst.opcode==0x08 || inst.opcode==0x23 || inst.opcode==0x21 || inst.opcode==0x25 ||
            inst.opcode==0x20 || inst.opcode==0x24 || inst.opcode==0x2B || inst.opcode==0x29 || inst.opcode==0x28) {
        augend = inst.rs_regvalue;
        addend = inst.c;
    }
    sum = augend + addend;
    if (augend>0 && addend>0 && sum<=0) return true;
    if (augend<0 && addend<0 && sum>=0) return true;
    return false;
}
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
void forwarding(Instruction *inst, Instruction pre, int type, int stage) {
    inst->forwarding = true;
    switch (type) {
        case FORW_RS:
            // forwarding RS
            inst->rs_regvalue = pre.EXEbuff;
            break;
        case FORW_RT:
            // forwarding RT
            inst->rt_regvalue = pre.EXEbuff;
            break;
        case FORW_L_RS:
            // forwarding load RS
            inst->rs_regvalue = pre.MEMbuff;
            break;
        case FORW_L_RT:
            // forwarding load RT
            inst->rt_regvalue = pre.MEMbuff;
            break;
        case FORW_S_RS:
            // forwarding store RS
            inst->rs_regvalue = pre.rt_regvalue;
            break;
        case FORW_S_RT:
            // forwarding store RT
            inst->rt_regvalue = pre.rt_regvalue;
            break;
        default:
            printf("forwarding type WRONG~~\n");
            break;
    }
    char strbuff[10];
    int strbufflen;
    switch (stage) {
        case 1:
            // preEXE rs
            strcat(inst->pipeline_fwd, " fwd_EX-DM_rs_$");
            strbufflen = sprintf(strbuff, "%d", inst->rs);
            strncat(inst->pipeline_fwd, strbuff, strbufflen);
            break;
        case 2:
            // preEXE rt
            strcat(inst->pipeline_fwd, " fwd_EX-DM_rt_$");
            strbufflen = sprintf(strbuff, "%d", inst->rt);
            strncat(inst->pipeline_fwd, strbuff, strbufflen);
            break;
        case 3:
            // preMEM rs
            strcat(inst->pipeline_fwd, " fwd_DM-WB_rs_$");
            strbufflen = sprintf(strbuff, "%d", inst->rs);
            strncat(inst->pipeline_fwd, strbuff, strbufflen);
            break;
        case 4:
            // preMEM rt
            strcat(inst->pipeline_fwd, " fwd_DM-WB_rt_$");
            strbufflen = sprintf(strbuff, "%d", inst->rt);
            strncat(inst->pipeline_fwd, strbuff, strbufflen);
            break;
        default:
            printf("forwarding string WRONG~~\n");
            break;
    }

}
void IFetch(Instruction *IF, Instruction ID, char *i_memory, int *pcounter) {
    IF->INST = chartoint(i_memory, *pcounter, 4);
    if (!ID.stall) {
        *pcounter+=4;
    }
    if (((IF->INST>>26)&0x0000003F)==0x3F) {
        if (!ID.stall) {
            halt_buff -= 1;
        }
    } else {
        if (halt_buff>0)
            halt_buff = 5;
    }
}
void IDecode(Instruction *inst, Instruction preMEM, Instruction preEXE, Instruction preID, int *reg, int *pcounter, int *tmppc) {

    inst->opcode = (inst->INST>>26) & 0x0000003F;
    if (inst->opcode==0x00) {
        /* R-type */
        inst->funct = inst->INST & 0x0000003F;
        if (inst->funct!=0x00 && inst->funct!=0x02 && inst->funct!=0x03) {
            inst->rs = (inst->INST>>21) & 0x0000001F;
        }
        if (inst->funct!=0x08) {
            inst->rt = (inst->INST>>16) & 0x0000001F;
            inst->rd = (inst->INST>>11) & 0x0000001F;
            inst->c = (inst->INST>>6) & 0x0000001F;
        }
        inst->rs_regvalue = reg[inst->rs];
        inst->rt_regvalue = reg[inst->rt];
    } else if (inst->opcode==0x02 || inst->opcode==0x03) {
        /* J-type */
        inst->JSc = inst->INST & 0x03FFFFFF;
        if (inst->opcode==0x03)
            inst->rs = 31;
        /* J control after stall detection */
    } else if (inst->opcode==0x3F) {
        /* S-type HALT*/
        printf("HALT\n");
    } else {
        /* I-type */
        inst->c = inst->INST & 0x0000FFFF;
        inst->rs = (inst->INST>>21) & 0x0000001F;
        inst->rt = (inst->INST>>16) & 0x0000001F;
        inst->rs_regvalue = reg[inst->rs];
        inst->rt_regvalue = reg[inst->rt];
        /* beq, bne after stall detection */
    }
    naming(inst);

    /* R-type instruction, check preID for STALL */
    if (inst->opcode==0x00 && inst->funct!=0x08) {
        if (preID.opcode==0x23 || preID.opcode==0x21 || preID.opcode==0x25 || preID.opcode==0x20 || preID.opcode==0x24) {
            /* load series ($rt) */
            if (inst->rs==preID.rt && inst->rs!=-1 && inst->rs!=0) {
                inst->stall = true;
            }
            if (inst->rt==preID.rt && inst->rt!=-1 && inst->rt!=0) {
                inst->stall = true;
            }
        }
        /* I-type no store FOR STALL */
    } else if (inst->opcode==0x08 || inst->opcode==0x23 || inst->opcode==0x21 || inst->opcode==0x25 || inst->opcode==0x20 ||
            inst->opcode==0x24 || inst->opcode==0x0C || inst->opcode==0x0D || inst->opcode==0x0E || inst->opcode==0x0A) {
        if (preID.opcode==0x23 || preID.opcode==0x21 || preID.opcode==0x25 || preID.opcode==0x20 || preID.opcode==0x24) {
            /* load series ($rt) */
            if (inst->rs==preID.rt && inst->rs!=-1 && inst->rs!=0) {
                inst->stall = true;
            }
        }
    } else if (inst->opcode==0x2B || inst->opcode==0x29 || inst->opcode==0x28) {
        if (preID.opcode==0x23 || preID.opcode==0x21 || preID.opcode==0x25 || preID.opcode==0x20 || preID.opcode==0x24) {
            /* load series ($rs) */
            if (inst->rs==preID.rt && inst->rs!=-1 && inst->rs!=0) {
                inst->stall = true;
            }
            /* load series ($rt) */
            if (inst->rt==preID.rt && inst->rt!=-1 && inst->rt!=0) {
                inst->stall = true;
            }
        }
    }

    /* BEQ, BNE, JR for STALL */
    if (inst->opcode==0x04 || inst->opcode==0x05 || (inst->opcode==0x00 && inst->funct==0x08)) {
        /* check preID */
        if (preID.opcode==0x00 && preID.funct!=0x08) {
            /* R-type ($rd) */
            if (inst->rs==preID.rd && inst->rs!=-1 && inst->rs!=0) {
                inst->stall = true;
            }
            if (inst->rt==preID.rd && inst->rt!=-1 && inst->rt!=0) {
                inst->stall = true;
            }
        } else if (preID.opcode!=0x2B && preID.opcode!=0x29 && preID.opcode!=0x28 && preID.opcode!=0x04 && preID.opcode!=0x05 && preID.opcode!=0x02 && preID.opcode!=0x03 && preID.opcode!=0x3F) {
            /* I-type ($rt) */
            if (inst->rs==preID.rt && inst->rs!=-1 && inst->rs!=0) {
                inst->stall = true;
            }
            if (inst->rt==preID.rt && inst->rt!=-1 && inst->rt!=0) {
                inst->stall = true;
            }
        }
        /* check preEXE */
        if (preEXE.opcode==0x23 || preEXE.opcode==0x21 || preEXE.opcode==0x25 || preEXE.opcode==0x20 || preEXE.opcode==0x24) {
            /* all Load word series ($rt) */
            if (inst->rs==preEXE.rt && inst->rs!=-1 && inst->rs!=0) {
                inst->stall = true;
            }
            if (inst->rt==preEXE.rt && inst->rt!=-1 && inst->rt!=0) {
                inst->stall = true;
            }
        }
    }

    bool exeforwed_rs = false;
    bool exeforwed_rt = false;

    if (!inst->stall) {
        /* BEQ, BNE, JR for FORWARDING */
        if (inst->opcode==0x04 || inst->opcode==0x05 || (inst->opcode==0x00 && inst->funct==0x08)) {
            /* check R-type ($rs first) ($rd) */
            if (inst->rs==preEXE.rd && inst->rs!=-1 && inst->rs!=0) {
                /* check preEXE */
                if (preEXE.opcode==0x00 && preEXE.opcode!=0x08) {
                    forwarding(inst, preEXE, FORW_RS, PREEX_RS);
                    exeforwed_rs = true;
                }
            }
            /* check I-type NO LOAD ($rs first) ($rt) */
            if (inst->rs==preEXE.rt && inst->rs!=-1 && inst->rs!=0) {
                /* check preEXE */
                if (preEXE.opcode==0x08 ||
                        preEXE.opcode==0x0F || preEXE.opcode==0x0C || preEXE.opcode==0x0D || preEXE.opcode==0x0E || preEXE.opcode==0x0A) {
                    forwarding(inst, preEXE, FORW_RS, PREEX_RS);
                    exeforwed_rs = true;
                }
            }
        }
        if (inst->opcode==0x04 || inst->opcode==0x05) {
            /* check R-type ($rt second) ($rt) */
            if (inst->rt==preEXE.rd && inst->rt!=-1 && inst->rt!=0) {
                /* check preEXE */
                if (preEXE.opcode==0x00 && preEXE.opcode!=0x08) {
                    forwarding(inst, preEXE, FORW_RT, PREEX_RT);
                    exeforwed_rt = true;
                }
            }
            /* check I-type ($rt second) ($rt) */
            if (inst->rt==preEXE.rt && inst->rt!=-1 && inst->rt!=0) {
                /* check preEXE */
                if (preEXE.opcode==0x08 ||
                        preEXE.opcode==0x0F || preEXE.opcode==0x0C || preEXE.opcode==0x0D || preEXE.opcode==0x0E || preEXE.opcode==0x0A) {
                    forwarding(inst, preEXE, FORW_RT, PREEX_RT);
                    exeforwed_rt = true;
                }
            }
        }

        if (inst->opcode==0x04 || inst->opcode==0x05 || (inst->opcode==0x00 && inst->funct==0x08)) {
            exeforwed_rs = false;
            exeforwed_rt = false;

            /* Jal forwarding */
            if (preEXE.opcode==0x03 && preEXE.INST!=0) {
                if (inst->rs==31) {
                    forwarding(inst, preEXE, FORW_RS, PREEX_RS);
                    exeforwed_rs = true;
                }
            }
            if (preEXE.opcode==0x03  && preEXE.INST!=0) {
                if (inst->rt==31) {
                    forwarding(inst, preEXE, FORW_RT, PREEX_RT);
                    exeforwed_rt = true;
                }
            }
        }
    }

    /* check for FLUSH */
    if (!inst->stall) {
        /* Handle J */
        if (inst->opcode==0x00 && inst->funct==0x08) {
            /* Handle Jr */
            *tmppc = inst->rs_regvalue;
            inst->flush = true;
        }
        if (inst->opcode==0x02) {
            *tmppc = (*pcounter & 0xF0000000) | ((inst->JSc<<2) & 0x0FFFFFFC);
            inst->flush = true;
        } else if (inst->opcode==0x03) {
            inst->EXEbuff = *pcounter;
            *tmppc = (*pcounter & 0xF0000000) | ((inst->JSc<<2) & 0x0FFFFFFC);
            inst->flush = true;
        } else if (inst->opcode==0x04) {
            /* Handle beq, bne */
            // beq
            if (inst->rs_regvalue == inst->rt_regvalue) {
                *tmppc = *pcounter + inst->c*4;
                inst->flush = true;
            }
        } else if (inst->opcode==0x05) {
            // bne
            if (inst->rs_regvalue != inst->rt_regvalue) {
                *tmppc = *pcounter + inst->c*4;
                inst->flush = true;
            }
        }
    }
}
int EXEcute(FILE *error_dump, Instruction *inst, Instruction preMEM, Instruction preEXE, Instruction preID, int *reg, int cycle) {

    bool exeforwed_rs = false;
    bool exeforwed_rt = false;

    if (inst->opcode==0x00) {
        /* check inst R-type forwarding */
        /* check preEXE ($rs first)*/
        if (inst->rs==preEXE.rd && inst->rs!=-1 && inst->rs!=0 && inst->funct!=0x00 && inst->funct!=0x02 && inst->funct!=0x03) {
            if (preEXE.opcode==0x00 && preEXE.funct!=0x08 && preEXE.INST!=0) {
                /* check R-type ($rs) ($rd) */
                forwarding(inst, preEXE, FORW_RS, PREEX_RS);
                exeforwed_rs = true;
            }
        }
        if (inst->rs==preEXE.rt && inst->rs!=-1 && inst->rs!=0 && inst->funct!=0x00 && inst->funct!=0x02 && inst->funct!=0x03) {
            if (preEXE.opcode==0x08 || preEXE.opcode==0x0F || preEXE.opcode==0x0C || preEXE.opcode==0x0D || preEXE.opcode==0x0E || preEXE.opcode==0x0A) {
                /* check I-type NO LOAD without STORE ($rs) ($rt) */
                forwarding(inst, preEXE, FORW_RS, PREEX_RS);
                exeforwed_rs = true;
            }
        }
        if (!exeforwed_rs) {
            /* check preMEM ($rs first) */
            if (inst->rs==preMEM.rd && inst->rs!=-1 && inst->rs!=0 && inst->funct!=0x00 && inst->funct!=0x02 && inst->funct!=0x03) {
                if (preMEM.opcode==0x00 && preMEM.funct!=0x08 && preMEM.INST!=0) {
                    /* check R-type ($rs) ($rd) */
                    forwarding(inst, preMEM, FORW_RS, PREDM_RS);
                }
            }
            if (inst->rs==preMEM.rt && inst->rs!=-1 && inst->rs!=0 && inst->funct!=0x00 && inst->funct!=0x02 && inst->funct!=0x03) {
                if (preMEM.opcode==0x08 || preMEM.opcode==0x0F || preMEM.opcode==0x0C || preMEM.opcode==0x0D || preMEM.opcode==0x0E || preMEM.opcode==0x0A) {
                    /* check I-type without LOAD ($rs) ($rt) */
                    forwarding(inst, preMEM, FORW_RS, PREDM_RS);
                } else if (preMEM.opcode==0x23 || preMEM.opcode==0x21 || preMEM.opcode==0x25 || preMEM.opcode==0x20 || preMEM.opcode==0x24) {
                    /* I-type LOAD ($rs) ($rt) */
                    forwarding(inst, preMEM, FORW_L_RS, PREDM_RS);
                }
            }
            if (inst->rs==preMEM.rs && inst->rs!=-1 && inst->rs!=0 && inst->funct!=0x00 && inst->funct!=0x02 && inst->funct!=0x03) {
                if (preMEM.opcode==0x03 && preMEM.INST!=0) {
                    /* J-type JAL ($rs) ($rs) */
                    forwarding(inst, preMEM, FORW_RS, PREDM_RS);
                }
            }
        }

        /* check preEXE ($rt second) */
        if (inst->rt==preEXE.rd && inst->rt!=-1 && inst->rt!=0) {
            if (preEXE.opcode==0x00 && preEXE.funct!=0x08 && preEXE.INST!=0) {
                /* R-type ($rt) */
                forwarding(inst, preEXE, FORW_RT, PREEX_RT);
                exeforwed_rt = true;
            }
        }
        if (inst->rt==preEXE.rt && inst->rt!=-1 && inst->rt!=0) {
            if (preEXE.opcode==0x08 || preEXE.opcode==0x0F || preEXE.opcode==0x0C || preEXE.opcode==0x0D || preEXE.opcode==0x0E || preEXE.opcode==0x0A) {
                /* I-type NO LOAD without STORE ($rt) */
                forwarding(inst, preEXE, FORW_RT, PREEX_RT);
                exeforwed_rt = true;
            }
        }
        if (!exeforwed_rt) {
            /* check preMEM ($rt) */
            if (inst->rt==preMEM.rd && inst->rt!=-1 && inst->rt!=0) {
                if (preMEM.opcode==0x00 && preMEM.funct!=0x08 && preMEM.INST!=0) {
                    /* check R-type ($rd) */
                    forwarding(inst, preMEM, FORW_RT, PREDM_RT);
                }
            }
            if (inst->rt==preMEM.rt && inst->rt!=-1 && inst->rt!=0) {
                if (preMEM.opcode==0x08 || preMEM.opcode==0x0F || preMEM.opcode==0x0C || preMEM.opcode==0x0D || preMEM.opcode==0x0E || preMEM.opcode==0x0A) {
                    /* I-type without load ($rt) */
                    forwarding(inst, preMEM, FORW_RT, PREDM_RT);
                } else if (preMEM.opcode==0x23 || preMEM.opcode==0x21 || preMEM.opcode==0x25 || preMEM.opcode==0x20 || preMEM.opcode==0x24) {
                    /* I-type LOAD */
                    forwarding(inst, preMEM, FORW_L_RT, PREDM_RT);
                }
            }
            if (inst->rt==preMEM.rs && inst->rt!=-1 && inst->rt!=0) {
                if (preMEM.opcode==0x03 && preMEM.INST!=0) {
                    /* J-type JAL ($rs) ($rs) */
                    forwarding(inst, preMEM, FORW_RT, PREDM_RT);
                }
            }
        }
    }
    /* check inst I-type forwarding without STORE's rt */
    else if (inst->opcode==0x08 || inst->opcode==0x23 || inst->opcode==0x21 || inst->opcode==0x25 || inst->opcode==0x20 ||
            inst->opcode==0x24 || inst->opcode==0x0C || inst->opcode==0x0D || inst->opcode==0x0E || inst->opcode==0x0A ||
            inst->opcode==0x2B || inst->opcode==0x29 || inst->opcode==0x28) {
        /* check preEXE */
        if (preEXE.opcode==0x00 && preEXE.funct!=0x08 && preEXE.INST!=0) {
            /* check R-type ($rd) */
            if (inst->rs==preEXE.rd && inst->rs!=-1 && inst->rs!=0) {
                forwarding(inst, preEXE, FORW_RS, PREEX_RS);
                exeforwed_rs = true;
            }
        } else if (preEXE.opcode==0x08 || preEXE.opcode==0x0F || preEXE.opcode==0x0C || preEXE.opcode==0x0D || preEXE.opcode==0x0E || preEXE.opcode==0x0A) {
            /* I-type NO LOAD without STORE ($rt) */
            if (inst->rs==preEXE.rt && inst->rs!=-1 && inst->rs!=0) {
                forwarding(inst, preEXE, FORW_RS, PREEX_RS);
                exeforwed_rs = true;
            }
        }
        if (!exeforwed_rs) {
            /* check preMEM */
            if (preMEM.opcode==0x00 && preMEM.funct!=0x08 && preMEM.INST!=0) {
                /* check R-type ($rd) */
                if (inst->rs==preMEM.rd && inst->rs!=-1 && inst->rs!=0) {
                    forwarding(inst, preMEM, FORW_RS, PREDM_RS);
                }
            } else if (preMEM.opcode==0x08 || preMEM.opcode==0x0F || preMEM.opcode==0x0C || preMEM.opcode==0x0D || preMEM.opcode==0x0E || preMEM.opcode==0x0A) {
                /* I-type without load ($rt) */
                if (inst->rs==preMEM.rt && inst->rs!=-1 && inst->rs!=0) {
                    forwarding(inst, preMEM, FORW_RS, PREDM_RS);
                }
            } else if (preMEM.opcode==0x23 || preMEM.opcode==0x21 || preMEM.opcode==0x25 || preMEM.opcode==0x20 || preMEM.opcode==0x24) {
                /* I-type LOAD ($rt) */
                if (inst->rs==preMEM.rt && inst->rs!=-1 && inst->rs!=0) {
                    forwarding(inst, preMEM, FORW_L_RS, PREDM_RS);
                }
            } else if (preMEM.opcode==0x03) {
                /* Jal */
                if (inst->rs==preMEM.rs && inst->rs!=-1 && inst->rs!=0) {
                    forwarding(inst, preMEM, FORW_RS, PREDM_RS);
                }
            }
        }
    }
    // check STORE's rt
    if (inst->opcode==0x2B || inst->opcode==0x29 || inst->opcode==0x28) {
        if (preEXE.opcode==0x00 && preEXE.funct!=0x08 && preEXE.INST!=0) {
            // check R-type ($rd)
            if (inst->rt==preEXE.rd && inst->rt!=-1 && inst->rt!=0) {
                forwarding(inst, preEXE, FORW_RT, PREEX_RT);
                exeforwed_rt = true;
            }
        } else if (preEXE.opcode==0x08 || preEXE.opcode==0x0F || preEXE.opcode==0x0C || preEXE.opcode==0x0D || preEXE.opcode==0x0E || preEXE.opcode==0x0A) {
            // I-type NO LOAD without STORE ($rt)
            if (inst->rt==preEXE.rt && inst->rt!=-1 && inst->rt!=0) {
                forwarding(inst, preEXE, FORW_RT, PREEX_RT);
                exeforwed_rt = true;
            }
        }
        if (!exeforwed_rt) {
            // check preMEM
            if (preMEM.opcode==0x00 && preMEM.funct!=0x08 && preMEM.INST!=0) {
                // check R-type ($rd)
                if (inst->rt==preMEM.rd && inst->rt!=-1 && inst->rt!=0) {
                    forwarding(inst, preMEM, FORW_RT, PREDM_RT);
                }
            } else if (preMEM.opcode==0x08 || preMEM.opcode==0x0F || preMEM.opcode==0x0C || preMEM.opcode==0x0D || preMEM.opcode==0x0E || preMEM.opcode==0x0A) {
                // I-type without load ($rt) and JAL
                if (inst->rt==preMEM.rt && inst->rt!=-1 && inst->rt!=0) {
                    forwarding(inst, preMEM, FORW_RT, PREDM_RT);
                }
            } else if (preMEM.opcode==0x23 || preMEM.opcode==0x21 || preMEM.opcode==0x25 || preMEM.opcode==0x20 || preMEM.opcode==0x24) {
                // I-type LOAD ($rt)
                if (inst->rt==preMEM.rt && inst->rt!=-1 && inst->rt!=0) {
                    forwarding(inst, preMEM, FORW_L_RT, PREDM_RT);
                }
            } else if (preMEM.opcode==0x03 && preMEM.INST!=0) {
                // Jal

                if (inst->rt==preMEM.rs && inst->rt!=-1 && inst->rt!=0) {
                    forwarding(inst, preMEM, FORW_RT, PREDM_RT);
                }

            }
        }
    }


    if (inst->opcode==0x00) {
        /* R-type */
        inst->EXEbuff =  RTypeInstruction(*inst, reg);
    } else if (inst->opcode==0x02 || inst->opcode==0x03) {
        /* J-type */
        inst->EXEbuff = inst->opcode==0x02? 0: inst->EXEbuff;
    } else if (inst->opcode==0x3F) {
        /* S-type */
        inst->EXEbuff = 0;
    } else {
        /* I-type */
        inst->EXEbuff = ITypeInstruction(*inst, reg);
    }
    if (checkNumOverflow(*inst)) {
        fprintf(error_dump, "In cycle %d: Number Overflow\n", cycle);
    }
    return inst->EXEbuff;
}
int MEMory(FILE *error_dump, Instruction *inst, int exebuff, int *reg, char *d_memory, int cycle) {
    if (MemAddrOverflow(*inst, exebuff)) {
        fprintf(error_dump, "In cycle %d: Address Overflow\n", cycle);
        halt_error = true;
    }
    if (MemMissAlign(*inst, exebuff)) {
        fprintf(error_dump, "In cycle %d: Misalignment Error\n", cycle);
        halt_error = true;
    }

    int MEM_value = 0;

    if (!halt_error) {
        if (inst->opcode==0x23) {
            // lw
            MEM_value = chartoint(d_memory, exebuff, 4);
        } else if (inst->opcode==0x21) {
            // lh
            MEM_value = (short) chartoint(d_memory, exebuff, 2);
        } else if (inst->opcode==0x25) {
            // lhu
            MEM_value = (unsigned short) chartoint(d_memory, exebuff, 2);
        } else if (inst->opcode==0x20) {
            // lb
            if (((d_memory[exebuff]>>7)&0x01)==1) {
                MEM_value = d_memory[exebuff] | 0xFFFFFF00;
            } else {
                MEM_value = d_memory[exebuff] & 0x000000FF;
            }
        } else if (inst->opcode==0x24) {
            // lbu
            MEM_value = d_memory[exebuff] & 0x000000FF;
        } else if (inst->opcode==0x2B) {
            // sw
            d_memory[exebuff] = (inst->rt_regvalue>>24) & 0x000000FF;
            d_memory[exebuff+1] = (inst->rt_regvalue>>16) & 0x000000FF;
            d_memory[exebuff+2] = (inst->rt_regvalue>>8) & 0x000000FF;
            d_memory[exebuff+3] = inst->rt_regvalue & 0x000000FF;
        } else if (inst->opcode==0x29) {
            // sh
            d_memory[exebuff] = (inst->rt_regvalue>>8) & 0x000000FF;
            d_memory[exebuff+1] = inst->rt_regvalue & 0x000000FF;
        } else if (inst->opcode==0x28) {
            // sb
            d_memory[exebuff] = inst->rt_regvalue & 0x000000FF;
        }
    }
    return MEM_value;
}
void WBack(FILE *error_dump, Instruction *inst, int exebuff, int membuff, int *reg, int cycle) {
    if (checkWrite0(*inst)==0) {
        fprintf(error_dump, "In cycle %d: Write $0 Error\n", cycle);
    }
    if (inst->opcode==0x00 && inst->funct!=0x08) {
        reg[inst->rd] = exebuff;
    } else if (inst->opcode==0x08 || inst->opcode==0x0F || inst->opcode==0x0C || inst->opcode==0x0D || inst->opcode==0x0E || inst->opcode==0x0A) {
        // addi, lui, andi, ori, nori, slti
        reg[inst->rt] = exebuff;
    } else if (inst->opcode==0x23 || inst->opcode==0x21 || inst->opcode==0x25 || inst->opcode==0x20 || inst->opcode==0x24) {
        // lw, lh, lhu, lb, lbu
        reg[inst->rt] = membuff;
    } else if (inst->opcode==0x03) {
        // jal
        reg[31] = inst->EXEbuff;
    }
    reg[0] = 0;
}
void loadimage(const char *file, char load[], int *firstfour) {

    FILE *image;
    if ( (image = fopen(file, "rb")) == NULL ) exit(1);
    char tmp[4];

    /* The first four byte: iimage-> initial value of PC
     *                      dimage-> initial value of $sp */
    for (int i=0; i<4; i++) fread(&tmp[i], sizeof(char), 1, image);
    *firstfour = ((tmp[0]<<24)&0xFF000000) | ((tmp[1]<<16)&0x00FF0000) | ((tmp[2]<<8)&0x0000FF00) | (tmp[3]&0x000000FF);

    /* The next four byte: iimage-> number of words to be loaded into I memory
     *                     dimage-> number of words to be loaded into D memory */
    for (int i=0; i<4; i++) fread(&tmp[i], sizeof(char), 1, image);
    int numwords = ((tmp[0]<<24)&0xFF000000) | ((tmp[1]<<16)&0x00FF0000) | ((tmp[2]<<8)&0x0000FF00) | (tmp[3]&0x000000FF);

    /* load program text/data into memory */
    if (strcmp(file, "dimage.bin") == 0) {
        for (int i=0; i<numwords*4; i++) fread(&load[i], sizeof(char), 1, image);
    } else {
        for (int i=*firstfour; i<numwords*4+*firstfour; i++) fread(&load[i], sizeof(char), 1, image);
    }
    fclose(image);
}
int main () {
    char i_memory[1024], d_memory[1024];
    int cycle=0, pcounter=0, reg[32];

    memset(i_memory, 0, sizeof(i_memory));
    memset(d_memory, 0, sizeof(d_memory));
    memset(reg, 0, sizeof(reg));
    halt_buff = 5;

    FILE *snapshot, *error_dump;
    snapshot = fopen("snapshot.rpt", "w");
    error_dump = fopen("error_dump.rpt", "w");

    loadimage("iimage.bin", i_memory, &pcounter);
    loadimage("dimage.bin", d_memory, &reg[29]);

    Instruction IF, ID, EXE, MEM, WB;
    Instruction preID, preEXE, preMEM;

    halt_error = false;

    IF.flush = IF.forwarding = IF.stall = false;
    IF.c = 0;
    IF.rd = IF.rs = IF.rt = -1;
    IF.funct = IF.INST = IF.JSc = IF.opcode = IF.EXEbuff = IF.MEMbuff = IF.rs_regvalue = IF.rt_regvalue = 0;
    ID.INST = EXE.INST = MEM.INST = WB.INST = 0;
    ID.stall = EXE.stall = MEM.stall = WB.stall = false;
    ID.flush = EXE.flush = MEM.flush = WB.flush = false;
    ID.forwarding = EXE.forwarding = MEM.forwarding = WB.forwarding = false;
    memset(IF.operation, 0, sizeof(IF.operation));
    memset(IF.pipeline_fwd, 0, sizeof(IF.pipeline_fwd));

    int tmppc=0;
    bool to_stall = false;

    do {
        printsnapshot(snapshot, cycle, reg, pcounter);
        cycle++;
        if (WB.INST!=0) {
            WBack(error_dump, &WB, WB.EXEbuff, WB.MEMbuff, reg, cycle);
        }
        if (MEM.INST!=0) {
            MEM.MEMbuff = MEMory(error_dump, &MEM, MEM.EXEbuff, reg, d_memory, cycle);
        }
        if (EXE.INST!=0) {
            EXE.EXEbuff = EXEcute(error_dump, &EXE, preMEM, preEXE, preID, reg, cycle);
        }
        if (ID.INST!=0) {
            IDecode(&ID, preMEM, preEXE, preID, reg, &pcounter, &tmppc);
        }
        if (!ID.stall) {
            to_stall = false;
        }
        if (!to_stall) {
            /* ID for pcounter if STALL */
            IFetch(&IF, ID, i_memory, &pcounter);
        }
        if (ID.flush) {
            pcounter = tmppc;
        }

        //if (cycle>30 && cycle<40)printf("cycle %d    ID: %s INST=0x%08X rs=%d rt=%d rd=%d reg[rs]=%x reg[rt]=%x c=%x \n", cycle-1, ID.operation, ID.INST, ID.rs, ID.rt, ID.rd, ID.rs_regvalue, ID.rt_regvalue, ID.c);
        //if (cycle>55 && cycle<68)printf("cycle %d stall=%d halt_buff=%d\n", cycle-1, ID.stall, halt_buff);
        if (cycle>130 && cycle<140)printf("cycle %d    EX: %s INST=0x%08X rs=%d rt=%d rd=%d reg[rs]=%x reg[rt]=%x c=%x \n", cycle-1, EXE.operation, EXE.INST, EXE.rs, EXE.rt, EXE.rd, EXE.rs_regvalue, EXE.rt_regvalue, EXE.c);
        //if (cycle>175 && cycle<185)printf("cycle %d    DM: %s INST=0x%08X rs=%d rt=%d rd=%d reg[rs]=%x reg[rt]=%x c=%x \n", cycle-1, MEM.operation, MEM.INST, MEM.rs, MEM.rt, MEM.rd, MEM.rs_regvalue, MEM.rt_regvalue, MEM.c);
        //if (cycle>175 && cycle<185)printf("cycle %d    WB: %s INST=0x%08X rs=%d rt=%d rd=%d reg[rs]=%x reg[rt]=%x c=%x \n", cycle-1, WB.operation, WB.INST, WB.rs, WB.rt, WB.rd, WB.rs_regvalue, WB.rt_regvalue, WB.c);
        if (cycle>130 && cycle<140)printf("cycle %d preID: %s INST=0x%08X rs=%d rt=%d rd=%d opcode=%x exebuff=%d\n", cycle-1, preID.operation, preID.INST, preID.rs, preID.rt, preID.rd, preID.opcode, preID.EXEbuff);
        if (cycle>130 && cycle<140)printf("cycle %d preEX: %s INST=0x%08X rs=%d rt=%d rd=%d opcode=%x exebuff=%d\n", cycle-1, preEXE.operation, preEXE.INST, preEXE.rs, preEXE.rt, preEXE.rd, preEXE.opcode, preMEM.EXEbuff);
        if (cycle>130 && cycle<140)printf("cycle %d preDM: %s INST=0x%08X rs=%d rt=%d rd=%d opcode=%x exebuff=%d\n", cycle-1, preMEM.operation, preMEM.INST, preMEM.rs, preMEM.rt, preMEM.rd, preMEM.opcode, preMEM.EXEbuff);
        printpipeline(snapshot, ID, IF, EXE, MEM, WB);

        /* record previous stage */
        preMEM = MEM;
        preEXE = EXE;
        preID = ID;

        /* passing through next cycle */
        WB = MEM;
        MEM = EXE;
        if (!ID.stall) {
            EXE = ID;
        } else {
            to_stall = true;
            preID.INST = preID.opcode = preID.rs = preID.rt = preID.rd = 0;
            EXE.INST = EXE.opcode = EXE.rs = EXE.rt = EXE.rd = 0;
        }
        if (!ID.stall && !ID.flush) {
            ID = IF;
        }
        if (ID.flush) {
            halt_buff = 5;
            ID.INST = 0;
            ID.flush = false;
        }
        memset(IF.operation, 0, sizeof(IF.operation));
        memset(IF.pipeline_fwd, 0, sizeof(IF.pipeline_fwd));
        if (!ID.stall) {
            IF.flush = IF.forwarding = IF.stall = false;
            IF.c = 0;
            IF.rd = IF.rs = IF.rt = -1;
            IF.funct = IF.INST = IF.JSc = IF.opcode = IF.EXEbuff = IF.MEMbuff = IF.rs_regvalue = IF.rt_regvalue = 0;
        }
        ID.flush = ID.stall = false;
    } while (halt_buff>0 && !halt_error);

    fclose(snapshot);
    fclose(error_dump);

    return 0;
}
