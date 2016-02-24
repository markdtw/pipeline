#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "main.h"

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
