#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "main.h"
int halt_buff;
bool halt_error;

int main () {
    char i_memory[1024], d_memory[1024];
    int cycle=0, pcounter=0, reg[32];

    memset(i_memory, 0, sizeof(i_memory));
    memset(d_memory, 0, sizeof(d_memory));
    memset(reg, 0, sizeof(reg));
    halt_buff = 5;

    FILE *snapshot, *error_dump;
    snapshot = fopen("../snapshot.rpt", "w");
    error_dump = fopen("../error_dump.rpt", "w");

    loadimage("../testcase/iimage.bin", i_memory, &pcounter);
    loadimage("../testcase/dimage.bin", d_memory, &reg[29]);

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
