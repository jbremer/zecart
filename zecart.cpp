#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "pin.H"
#include "zecart.h"

#ifdef TARGET_IA32

const char *g_reg_names[] = {
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi",
};

REG g_reg_names_order[8] = {
    REG_EAX, REG_ECX, REG_EDX, REG_EBX, REG_ESP, REG_EBP, REG_ESI, REG_EDI,
};

ADDRINT g_reg_values[8];

#else

const char *g_reg_names[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

REG g_reg_names_order[16] = {
    REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
    REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15
};

ADDRINT g_reg_values[16];

#endif

uint32_t g_reg_index[REG_LAST];


int only_main_module = 0;

void execute_instruction(ADDRINT addr, const char *insns)
{
    printf("0x%lx: %s  |  ", addr, insns);
}

void dump_read_memory(void *ptr, ADDRINT size)
{
    switch (size) {
    case 1:
        printf("reads %d 0x%02x from 0x%08x, ", *(unsigned char *) ptr, *(unsigned char *) ptr, ptr);
        break;

    case 2:
        printf("reads %d 0x%04x from 0x%08x, ", *(unsigned short *) ptr, *(unsigned short *) ptr, ptr);
        break;

    case 4:
        printf("reads %u 0x%08x from 0x%08x, ", *(unsigned int *) ptr, *(unsigned int *) ptr, ptr);
        break;

    case 8:
        printf("reads %lu 0x%08lx from 0x%08x, ", *(ADDRINT *) ptr, *(ADDRINT *) ptr, ptr);
        break;

    default:
        printf("reads INVALID SIZE %d from 0x%08x, ", size, ptr);
        break;
    }
}

void *dump_addr;
uint32_t dump_size;

void dump_written_memory_before(void *ptr, uint32_t size)
{
    dump_addr = ptr;
    dump_size = size;
}

void dump_written_memory_after()
{
    void *ptr = dump_addr;
    uint32_t size = dump_size;

    switch (size) {
    case 1:
        printf("writes %d 0x%02x to 0x%08x, ", *(unsigned char *) ptr, *(unsigned char *) ptr, ptr);
        break;

    case 2:
        printf("writes %d 0x%04x to 0x%08x, ", *(unsigned short *) ptr, *(unsigned short *) ptr, ptr);
        break;

    case 4:
        printf("writes %u 0x%08x to 0x%08x, ", *(unsigned int *) ptr, *(unsigned int *) ptr, ptr);
        break;

    case 8:
        printf("writes %lu 0x%08lx to 0x%08x, ", *(ADDRINT *) ptr, *(ADDRINT *) ptr, ptr);
        break;

    default:
        printf("writes INVALID SIZE %d to 0x%08x, ", size, ptr);
        break;
    }
}



void dump_reg_before(uint32_t reg_index, ADDRINT value)
{
    g_reg_values[reg_index] = value;
}

void dump_reg_rw_after(uint32_t reg_index, ADDRINT value)
{
    printf("%s RW 0x%08x 0x%08x, ", g_reg_names[reg_index], g_reg_values[reg_index], value);
}

void dump_reg_r_after(uint32_t reg_index)
{
    printf("%s R 0x%08x, ", g_reg_names[reg_index], g_reg_values[reg_index]);
}

void dump_reg_w_after(uint32_t reg_index, ADDRINT value)
{
    printf("%s W 0x%08x, ", g_reg_names[reg_index], value);
}

void print_newline()
{
    printf("\n");
}

int inside = 0;

void insns(INS ins, void *v)
{
    IMG img = IMG_FindByAddress(INS_Address(ins));
    int main_module = IMG_Valid(img) ? IMG_IsMainExecutable(img) : 0;

    if(only_main_module != 0 && main_module == 0) {
        return;
    }

    if(INS_Address(ins) == 0x401fc0) {
        inside = 1;
    }

    //if(inside == 0) return;

    if(INS_Address(ins) == 0x40206a) {
        inside = 0;
    }

    if(INS_Mnemonic(ins) != "XOR") {
        //return;
    }

    // dump the instruction
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &execute_instruction, IARG_INST_PTR, IARG_PTR, _strdup(INS_Disassemble(ins).c_str()), IARG_END);

    // reads memory (1)
    if(INS_IsMemoryRead(ins) != 0) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &dump_read_memory, IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_END);
    }

    // reads memory (2)
    if(INS_HasMemoryRead2(ins) != 0) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &dump_read_memory, IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_END);
    }

    IPOINT after = IPOINT_AFTER;
    if(INS_IsCall(ins) != 0) {
        after = IPOINT_TAKEN_BRANCH;
    }
    else if(INS_IsSyscall(ins) != 0) {
        return;
    }
    else if(INS_HasFallThrough(ins) == 0 && (INS_IsBranch(ins) != 0 || INS_IsRet(ins) != 0)) {
        after = IPOINT_TAKEN_BRANCH;
    }

    // dump written memory
    if(INS_IsMemoryWrite(ins) != 0) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &dump_written_memory_before, IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE, IARG_END);
        INS_InsertCall(ins, after, (AFUNPTR) &dump_written_memory_after, IARG_END);
    }

    // dump all affected registers
    for (UINT32 i = 0; i < INS_OperandCount(ins); i++) {
        if(INS_OperandIsMemory(ins, i) != 0) {
            if(INS_OperandMemoryBaseReg(ins, i) != REG_INVALID()) {
                REG base_reg = INS_OperandMemoryBaseReg(ins, i);

                if(g_reg_index[base_reg] != 0) {
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &dump_reg_before, IARG_UINT32, g_reg_index[base_reg]-1, IARG_REG_VALUE, INS_OperandMemoryBaseReg(ins, i), IARG_END);
                    INS_InsertCall(ins, after, (AFUNPTR) &dump_reg_r_after, IARG_UINT32, g_reg_index[base_reg]-1, IARG_END);
                }
            }
            if(INS_OperandMemoryIndexReg(ins, i) != REG_INVALID()) {
                REG index_reg = INS_OperandMemoryIndexReg(ins, i);

                if(g_reg_index[index_reg] != 0) {
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &dump_reg_before, IARG_UINT32, g_reg_index[index_reg]-1, IARG_REG_VALUE, INS_OperandMemoryIndexReg(ins, i), IARG_END);
                    INS_InsertCall(ins, after, (AFUNPTR) &dump_reg_r_after, IARG_UINT32, g_reg_index[index_reg]-1, IARG_END);
                }
            }
        }
        if(INS_OperandIsReg(ins, i) != 0) {
            REG reg_index = REG_FullRegName(INS_OperandReg(ins, i));

            if(INS_OperandReadAndWritten(ins, i) != 0) {
                if(g_reg_index[reg_index] != 0) {
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &dump_reg_before, IARG_UINT32, g_reg_index[reg_index]-1, IARG_REG_VALUE, reg_index, IARG_END);
                    INS_InsertCall(ins, after, (AFUNPTR) &dump_reg_rw_after, IARG_UINT32, g_reg_index[reg_index]-1, IARG_REG_VALUE, reg_index, IARG_END);
                }
            }
            else if(INS_OperandRead(ins, i) != 0) {
                if(g_reg_index[reg_index] != 0) {
                    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &dump_reg_before, IARG_UINT32, g_reg_index[reg_index]-1, IARG_REG_VALUE, reg_index, IARG_END);
                    INS_InsertCall(ins, after, (AFUNPTR) &dump_reg_r_after, IARG_UINT32, g_reg_index[reg_index]-1, IARG_END);
                }
            }
            else if(INS_OperandWritten(ins, i) != 0) {
                if(g_reg_index[reg_index] != 0) {
                    INS_InsertCall(ins, after, (AFUNPTR) &dump_reg_w_after, IARG_UINT32, g_reg_index[reg_index]-1, IARG_REG_VALUE, reg_index, IARG_END);
                }
            }
        }
    }

    INS_InsertCall(ins, after, (AFUNPTR) &print_newline, IARG_END);
}

int main(int argc, char *argv[])
{
    for (int i = 3; i < argc; i++) {
        if(!strcmp(argv[i], "--only-main-module")) {
            only_main_module = 1;
        }
    }

    PIN_Init(argc, argv);

    // initialize reg indices (as we keep a state at some point)
    for (uint32_t i = 0; i < sizeofarray(g_reg_names_order); i++) {
        g_reg_index[g_reg_names_order[i]] = i + 1;
    }

    INS_AddInstrumentFunction(&insns, 0);

    PIN_StartProgram();
    return 0;
}
