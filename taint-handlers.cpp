#include <stdio.h>
#include <stdint.h>
#include <unordered_map>
#include "pin.H"
#include "taint-handlers.h"
#include "registers.h"

#ifdef TARGET_IA32
static uint8_t g_reg_taint[8];
#else
static uint8_t g_reg_taint[16];
#endif

static std::unordered_map<ADDRINT, uint8_t> g_mem_taint;


static void prop_dstreg_srcreg(uint32_t dst, uint32_t src)
{
    g_reg_taint[dst] = g_reg_taint[src];
}

static void prop_dstreg_srcmem(uint32_t idx, ADDRINT addr, uint32_t size)
{
    uint8_t flags = 0;
    for (; size != 0; addr++, size--) {
        if(g_mem_taint.find(addr) != g_mem_taint.end()) {
            flags |= g_mem_taint[addr];
        }
    }
    g_reg_taint[idx] = flags;
}

static void prop_dstreg_untaint(uint32_t idx)
{
    g_reg_taint[idx] = 0;
}

static void prop_dstmem_srcreg(ADDRINT addr, uint32_t size, uint32_t idx,
    uint32_t offset)
{
    addr += offset;

    while (size-- != 0) {
        g_mem_taint[addr++] = g_reg_taint[idx];
    }
}

static void prop_dstmem_untaint(ADDRINT addr, uint32_t size)
{
    for (; size != 0; addr++, size--) {
        if(g_mem_taint.find(addr) != g_mem_taint.end()) {
            g_mem_taint[addr] = 0;
        }
    }
}

static void prop_dstreg_srcreg_srcreg(uint32_t d, uint32_t s, uint32_t s2)
{
    g_reg_taint[d] = g_reg_taint[s] | g_reg_taint[s2];
}


#define MEM_READ() IARG_MEMORYREAD_EA, IARG_MEMORYREAD_SIZE
#define MEM_WRITE() IARG_MEMORYWRITE_EA, IARG_MEMORYWRITE_SIZE

#define REG_INDEX(idx) \
    IARG_UINT32, g_reg_index[REG_FullRegName(INS_OperandReg(ins, idx))]-1

#define VALID_REG(idx) \
    (g_reg_index[REG_FullRegName(INS_OperandReg(ins, idx))] != 0)

void taint_prop_handle(INS ins, taint_prop_t prop, const uint8_t *args)
{
    switch (prop) {
    case P_INVLD:
        fprintf(stderr, "Invalid Prop handler!\n");
        exit(1);

    case P_NOP:
        break;

    case P_DREG_SREG:
        if(VALID_REG(args[0]) && VALID_REG(args[1])) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &prop_dstreg_srcreg,
                REG_INDEX(args[0]), REG_INDEX(args[1]), IARG_END);
        }
        break;

    case P_DREG_SMEM:
        if(VALID_REG(args[0])) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &prop_dstreg_srcmem,
                REG_INDEX(args[0]), MEM_READ(), IARG_END);
        }
        break;

    case P_DREG_UNTAINT:
        if(VALID_REG(args[0])) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &prop_dstreg_untaint,
                REG_INDEX(args[0]), IARG_END);
        }
        break;

    case P_DMEM_SREG:
        if(VALID_REG(args[1])) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &prop_dstmem_srcreg,
                MEM_WRITE(), REG_INDEX(args[1]), IARG_UINT32, 0, IARG_END);
        }
        break;

    case P_DMEM_SREG_PUSH:
        if(VALID_REG(args[1])) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &prop_dstmem_srcreg,
                MEM_WRITE(), REG_INDEX(args[1]), IARG_UINT32, -4, IARG_END);
        }
        break;

    case P_DMEM_UNTAINT:
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &prop_dstmem_untaint,
            MEM_WRITE(), IARG_END);
        break;

    case P_DREG_SREG_SREG:
        if(VALID_REG(args[0]) && VALID_REG(args[1]) && VALID_REG(args[2])) {
            INS_InsertCall(ins, IPOINT_BEFORE,
                (AFUNPTR) &prop_dstreg_srcreg_srcreg, REG_INDEX(args[0]),
                REG_INDEX(args[1]), REG_INDEX(args[2]), IARG_END);
        }
        break;
    }
}

void taint_register(uint32_t idx)
{
    g_reg_taint[idx] = 1;
}

void taint_memory(ADDRINT addr, uint32_t size)
{
    for (; size != 0; addr++, size--) {
        g_mem_taint[addr] = 1;
    }
}
