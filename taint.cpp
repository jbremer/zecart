#include <stdio.h>
#include <stdint.h>
#include "pin.H"
#include "taint.h"

#ifdef TARGET_IA32
static uint8_t g_reg_taint[8];
#else
static uint8_t g_reg_taint[16];
#endif

static void prop_dstreg_srcreg(uint32_t dst, uint32_t src)
{
    g_reg_taint[dst] = g_reg_taint[src];
}


#define T_MEM 1
#define T_ADR 2
#define T_REG 4
#define T_IMM 8
#define T_RD  16
#define T_WR  32

static uint32_t taint_handler_count[XED_ICLASS_LAST];
static uint32_t taint_handler_flag[XED_ICLASS_LAST][16];
static AFUNPTR taint_handler_fn[XED_ICLASS_LAST][16];


void taint_ins(INS ins, void *v)
{
    uint32_t opcode = INS_Opcode(ins);
    uint32_t opcount = INS_OperandCount(ins);

    // the generic stuff only supports up to 5 operands
    if(opcount < 6 && taint_handler_count[opcode] != 0) {
        // first we have to build up the flags, which is kind of expensive.
        uint32_t flags = 0;

        for (uint32_t i = 0; i < opcount; i++) {
            uint32_t op_flag = 0;
            if(INS_OperandIsMemory(ins, i) != 0) {
                op_flag |= T_MEM;
            }
            if(INS_OperandIsAddressGenerator(ins, i) != 0) {
                op_flag |= T_ADR;
            }
            if(INS_OperandIsReg(ins, i) != 0) {
                op_flag |= T_REG;
            }
            if(INS_OperandIsImmediate(ins, i) != 0) {
                op_flag |= T_IMM;
            }
            if(INS_OperandRead(ins, i) != 0) {
                op_flag |= T_RD;
            }
            if(INS_OperandWritten(ins, i) != 0) {
                op_flag |= T_WR;
            }
            flags |= op_flag << (6 * i);
        }

        for (uint32_t i = 0; i < taint_handler_count[opcode]; i++) {
            if(taint_handler_flag[opcode][i] == flags) {
                return;
            }
        }
    }
}

// sense - none.. without the vararg parameter, cl starts crying about a fifth
// argument.. seriously?!
static inline uint32_t combine(uint32_t a, uint32_t b, uint32_t c,
    uint32_t d, uint32_t e, ...)
{
    return a | (b << 6) | (c << 12) | (d << 18) | (e << 24);
}

#define _FLAGS(a, b, c, d, e, ...) combine(a, b, c, d, e)
#define FLAGS(...) _FLAGS(##__VA_ARGS__, 0, 0, 0, 0)

#define T(cls, handler, ...) { \
    const uint32_t count = taint_handler_count[XED_ICLASS_##cls]; \
    taint_handler_fn[XED_ICLASS_##cls][count] = (AFUNPTR) handler; \
    taint_handler_flag[XED_ICLASS_##cls][count] = FLAGS(__VA_ARGS__); \
    taint_handler_count[XED_ICLASS_##cls]++; }

void taint_init()
{
    T(MOV, &prop_dstreg_srcreg, T_REG|T_WR, T_REG|T_RD);
    T(SUB, NULL, T_REG|T_RD|T_WR, T_IMM|T_RD, T_REG|T_WR);
}
