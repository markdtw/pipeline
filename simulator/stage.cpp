#include<cstdio>
#include<cstdlib>
#include<cstring>
#include "main.h"

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
