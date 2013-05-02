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

#else

const char *g_reg_names[] = {
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
    "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
};

REG g_reg_names_order[16] = {
    REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
    REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15
};

#endif

uint32_t g_reg_index[REG_LAST];

void registers_init()
{
    // initialize reg indices (as we keep a state at some point)
    for (uint32_t i = 0; i < sizeofarray(g_reg_names_order); i++) {
        g_reg_index[g_reg_names_order[i]] = i + 1;
    }
}
