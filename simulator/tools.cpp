#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "main.h"

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
