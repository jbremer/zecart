
#ifdef TARGET_IA32

extern const char *g_reg_names[8];
extern REG g_reg_names_order[8];

#else

extern const char *g_reg_names[16];
extern REG g_reg_names_order[16];

#endif

extern uint32_t g_reg_index[REG_LAST];

void registers_init();
