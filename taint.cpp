#include <stdio.h>
#include <stdint.h>
#include "pin.H"
#include "taint.h"

#define T_MEM 1
#define T_ADR 2
#define T_REG 4
#define T_IMM 8
#define T_RD  16
#define T_WR  32

static uint32_t taint_handler_count[XED_ICLASS_LAST];
static uint32_t taint_handler_flag[XED_ICLASS_LAST][16];
static taint_prop_t taint_handler_fn[XED_ICLASS_LAST][16];
static const uint8_t *taint_handler_args[XED_ICLASS_LAST][16];

static int g_log_taint;


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
                taint_prop_handle(ins, taint_handler_fn[opcode][i],
                    taint_handler_args[opcode][i]);
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
#define FLAGS(...) _FLAGS(##__VA_ARGS__, 0, 0, 0, 0, 0)

#define T(cls, handler, a0, a1, a2, ...) { \
    const uint32_t count = taint_handler_count[XED_ICLASS_##cls]; \
    taint_handler_fn[XED_ICLASS_##cls][count] = handler; \
    taint_handler_flag[XED_ICLASS_##cls][count] = FLAGS(__VA_ARGS__); \
    const static uint8_t indices[] = {a0-1, a1-1, a2-1}; \
    taint_handler_args[XED_ICLASS_##cls][count] = indices; \
    taint_handler_count[XED_ICLASS_##cls]++; }

void taint_init(int log_taint)
{
    g_log_taint = log_taint;

    T(MOV, P_DREG_SREG, 1, 2, 0, T_REG|T_WR, T_REG|T_RD);
    T(MOV, P_DREG_SMEM, 1, 2, 0, T_REG|T_WR, T_MEM|T_RD);
    T(MOV, P_DMEM_UNTAINT, 1, 0, 0, T_MEM|T_WR, T_IMM|T_RD);
    T(MOV, P_DMEM_SREG, 1, 2, 0, T_MEM|T_WR, T_REG|T_RD);
    T(MOV, P_DREG_UNTAINT, 1, 0, 0, T_REG|T_WR, T_IMM|T_RD);

    T(MOVSX, P_DREG_SMEM, 1, 2, 0, T_REG|T_WR, T_MEM|T_RD);

    T(ADD, P_NOP, 0, 0, 0, T_REG|T_RD|T_WR, T_IMM|T_RD, T_REG|T_WR);
    T(DEC, P_NOP, 0, 0, 0, T_REG|T_RD|T_WR, T_REG|T_WR);
    T(SUB, P_NOP, 0, 0, 0, T_REG|T_RD|T_WR, T_IMM|T_RD, T_REG|T_WR);
    T(XOR, P_DREG_SREG_SREG, 1, 1, 2, T_REG|T_RD|T_WR, T_REG|T_RD,
        T_REG|T_WR);

    T(POP, P_DREG_SMEM, 1, 2, 0, T_REG|T_WR, T_RD, T_MEM|T_RD, T_RD|T_WR);
    T(PUSH, P_DMEM_SREG_PUSH, 3, 1, 0, T_REG|T_RD, T_WR, T_MEM|T_WR,
        T_RD|T_WR);

    T(CMP, P_NOP, 0, 0, 0, T_MEM|T_RD, T_IMM|T_RD, T_REG|T_WR);
    T(CMP, P_NOP, 0, 0, 0, T_REG|T_RD, T_IMM|T_RD, T_REG|T_WR);
    T(TEST, P_NOP, 0, 0, 0, T_REG|T_RD, T_REG|T_RD, T_REG|T_WR);

    T(JB, P_NOP, 0, 0, 0, T_RD, T_REG|T_RD|T_WR, T_REG|T_RD);
    T(JZ, P_NOP, 0, 0, 0, T_RD, T_REG|T_RD|T_WR, T_REG|T_RD);
    T(JNZ, P_NOP, 0, 0, 0, T_RD, T_REG|T_RD|T_WR, T_REG|T_RD);
    T(JMP, P_NOP, 0, 0, 0, T_RD, T_REG|T_RD|T_WR);
}
