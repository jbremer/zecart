
typedef enum _taint_prop_t {
    P_INVLD = 0,
    P_NOP,
    P_DREG_SREG,
    P_DREG_SMEM,
    P_DREG_UNTAINT,
    P_DMEM_SREG,
    P_DMEM_SREG_PUSH,
    P_DMEM_UNTAINT,
    P_DREG_SREG_SREG,
} taint_prop_t;

void taint_prop_handle(INS ins, taint_prop_t prop, const uint8_t *args);

void taint_register(uint32_t idx);
void taint_memory(ADDRINT addr, uint32_t size);
