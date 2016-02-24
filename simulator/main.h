/*
 * main.h
 */
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
// A instruction structure
typedef struct Instruction{
    int opcode, rs, rt, rd, JSc, funct, INST;
    int rs_regvalue, rt_regvalue;
    int EXEbuff, MEMbuff;
    bool stall, flush, forwarding;
    char pipeline_fwd[50];
    char operation[5];
    short c;
}Instruction;
// halt buffer and check for halt
extern int halt_buff;
extern bool halt_error;
// print part of snapshot
void printsnapshot(FILE *snapshot, int cycle, int *reg, int pc); 
// print pipeline instructions
void printpipeline(FILE *snapshot, Instruction ID, Instruction IF, Instruction EXE, Instruction MEM, Instruction WB);
// convert char to int
int chartoint(char *memory, int start, int wordsize);
// check error: if write to reg 0
int checkWrite0(Instruction inst);
// check error: if memory address overflow
bool MemAddrOverflow(Instruction inst, int exebuff);
// check error: if memory miss alignment happens
bool MemMissAlign(Instruction inst, int exebuff);
// check error: if number overflow
bool checkNumOverflow(Instruction inst);
// name all the instructions
void naming(Instruction *inst);
// implement R-type instructions
int RTypeInstruction(Instruction code, int *reg);
// implement I-type instructions
int ITypeInstruction(Instruction code, int *reg);
// do pipeline forwarding
void forwarding(Instruction *inst, Instruction pre, int type, int stage);
// pipeline Instruction Fetch stage
void IFetch(Instruction *IF, Instruction ID, char *i_memory, int *pcounter);
// pipeline Instruction Decode stage
void IDecode(Instruction *inst, Instruction preMEM, Instruction preEXE, Instruction preID, int *reg, int *pcounter, int *tmppc);
// pipeline Execute stage
int EXEcute(FILE *error_dump, Instruction *inst, Instruction preMEM, Instruction preEXE, Instruction preID, int *reg, int cycle);
// pipeline Memory read/write stage
int MEMory(FILE *error_dump, Instruction *inst, int exebuff, int *reg, char *d_memory, int cycle);
// pipeline Write back stage
void WBack(FILE *error_dump, Instruction *inst, int exebuff, int membuff, int *reg, int cycle);
// load image to program
void loadimage(const char *file, char load[], int *firstfour); 
