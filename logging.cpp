#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "zecart.h"

#ifdef TARGET_IA32
static ADDRINT g_reg_values[8];
#else
static ADDRINT g_reg_values[16];
#endif

void log_info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fflush(stderr);
}

static void print_addrint(ADDRINT value)
{
#ifdef TARGET_IA32
    printf("0x%08x", value);
#else
    printf("0x%016lx", value);
#endif
}

static void execute_instruction(ADDRINT addr, const char *insns)
{
    print_addrint(addr);
    printf(": %-32s  |  ", insns);
}

static void dump_memory_helper(void *ptr, uint32_t size)
{
    switch (size) {
    case 1:
        printf("0x%02x", *(uint8_t *) ptr);
        break;

    case 2:
        printf("0x%04x", *(uint16_t *) ptr);
        break;

    case 4:
        printf("0x%08x", *(uint32_t *) ptr);
        break;

    case 8:
#ifdef TARGET_IA32
        printf("0x%016llx", *(uint64_t *) ptr);
#else
        printf("0x%016lx", *(uint64_t *) ptr);
#endif
        break;

    default:
        printf("INVALID SIZE");
        break;
    }
}

static void dump_read_memory(void *ptr, ADDRINT size)
{
    printf("R ");
    dump_memory_helper(ptr, size);
    printf(" (");
    print_addrint((ADDRINT) ptr);
    printf("), ");
}

static void *g_dump_addr;
static uint32_t g_dump_size;

static void dump_written_memory_before(void *ptr, uint32_t size)
{
    g_dump_addr = ptr;
    g_dump_size = size;
}

static void dump_written_memory_after()
{
    printf("W ");
    dump_memory_helper(g_dump_addr, g_dump_size);
    printf(" (");
    print_addrint((ADDRINT) g_dump_addr);
    printf("), ");
}

static void dump_reg_before(uint32_t reg_index, ADDRINT value)
{
    g_reg_values[reg_index] = value;
}

static void dump_reg_rw_after(uint32_t reg_index, ADDRINT value)
{
    printf("RW %s ", g_reg_names[reg_index]);
    print_addrint(g_reg_values[reg_index]);
    printf(" ");
    print_addrint(value);
    printf(", ");
}

static void dump_reg_r_after(uint32_t reg_index)
{
    printf("R %s ", g_reg_names[reg_index]);
    print_addrint(g_reg_values[reg_index]);
    printf(", ");
}

static void dump_reg_w_after(uint32_t reg_index, ADDRINT value)
{
    printf("W %s ", g_reg_names[reg_index]);
    print_addrint(value);
    printf(", ");
}

static void print_newline()
{
    printf("\n");
}

void log_ins(INS ins)
{
    // dump the instruction
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &execute_instruction,
        IARG_INST_PTR, IARG_PTR, strdup(INS_Disassemble(ins).c_str()),
        IARG_END);

    // reads memory (1)
    if(INS_IsMemoryRead(ins) != 0) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &dump_read_memory,
            IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE, IARG_END);
    }

    // reads memory (2)
    if(INS_HasMemoryRead2(ins) != 0) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &dump_read_memory,
            IARG_MEMORYREAD2_EA, IARG_MEMORYREAD_SIZE, IARG_END);
    }

    IPOINT after = IPOINT_AFTER;
    if(INS_IsCall(ins) != 0) {
        // TODO is this correct?
        after = IPOINT_TAKEN_BRANCH;
    }
    else if(INS_IsSyscall(ins) != 0) {
        // TODO support syscalls
        return;
    }
    else if(INS_HasFallThrough(ins) == 0 && (INS_IsBranch(ins) != 0 ||
            INS_IsRet(ins) != 0)) {
        // TODO is this correct?
        after = IPOINT_TAKEN_BRANCH;
    }

    // dump written memory
    if(INS_IsMemoryWrite(ins) != 0) {
        INS_InsertCall(ins, IPOINT_BEFORE,
            (AFUNPTR) &dump_written_memory_before, IARG_MEMORYWRITE_EA,
            IARG_MEMORYWRITE_SIZE, IARG_END);

        INS_InsertCall(ins, after, (AFUNPTR) &dump_written_memory_after,
            IARG_END);
    }

    // dump all affected registers
    for (UINT32 i = 0; i < INS_OperandCount(ins); i++) {
        if(INS_OperandIsMemory(ins, i) != 0) {
            if(INS_OperandMemoryBaseReg(ins, i) != REG_INVALID()) {
                REG base_reg = INS_OperandMemoryBaseReg(ins, i);

                if(g_reg_index[base_reg] != 0) {
                    INS_InsertCall(ins, IPOINT_BEFORE,
                        (AFUNPTR) &dump_reg_before,
                        IARG_UINT32, g_reg_index[base_reg]-1,
                        IARG_REG_VALUE, INS_OperandMemoryBaseReg(ins, i),
                        IARG_END);

                    INS_InsertCall(ins, after,
                        (AFUNPTR) &dump_reg_r_after,
                        IARG_UINT32, g_reg_index[base_reg]-1, IARG_END);
                }
            }
            if(INS_OperandMemoryIndexReg(ins, i) != REG_INVALID()) {
                REG index_reg = INS_OperandMemoryIndexReg(ins, i);

                if(g_reg_index[index_reg] != 0) {
                    INS_InsertCall(ins, IPOINT_BEFORE,
                        (AFUNPTR) &dump_reg_before,
                        IARG_UINT32, g_reg_index[index_reg]-1,
                        IARG_REG_VALUE, INS_OperandMemoryIndexReg(ins, i),
                        IARG_END);

                    INS_InsertCall(ins, after,
                        (AFUNPTR) &dump_reg_r_after,
                        IARG_UINT32, g_reg_index[index_reg]-1, IARG_END);
                }
            }
        }
        if(INS_OperandIsReg(ins, i) != 0) {
            REG reg_index = REG_FullRegName(INS_OperandReg(ins, i));

            if(INS_OperandReadAndWritten(ins, i) != 0) {
                if(g_reg_index[reg_index] != 0) {
                    INS_InsertCall(ins, IPOINT_BEFORE,
                        (AFUNPTR) &dump_reg_before,
                        IARG_UINT32, g_reg_index[reg_index]-1,
                        IARG_REG_VALUE, reg_index, IARG_END);

                    INS_InsertCall(ins, after, (AFUNPTR) &dump_reg_rw_after,
                        IARG_UINT32, g_reg_index[reg_index]-1,
                        IARG_REG_VALUE, reg_index, IARG_END);
                }
            }
            else if(INS_OperandRead(ins, i) != 0) {
                if(g_reg_index[reg_index] != 0) {
                    INS_InsertCall(ins, IPOINT_BEFORE,
                        (AFUNPTR) &dump_reg_before,
                        IARG_UINT32, g_reg_index[reg_index]-1,
                        IARG_REG_VALUE, reg_index, IARG_END);

                    INS_InsertCall(ins, after, (AFUNPTR) &dump_reg_r_after,
                        IARG_UINT32, g_reg_index[reg_index]-1, IARG_END);
                }
            }
            else if(INS_OperandWritten(ins, i) != 0) {
                if(g_reg_index[reg_index] != 0) {
                    INS_InsertCall(ins, after, (AFUNPTR) &dump_reg_w_after,
                        IARG_UINT32, g_reg_index[reg_index]-1,
                        IARG_REG_VALUE, reg_index, IARG_END);
                }
            }
        }
    }

    INS_InsertCall(ins, after, (AFUNPTR) &print_newline, IARG_END);
}
