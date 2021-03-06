#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "zecart.h"

int should_instrument_ins(INS ins)
{
    if(is_accepted_address(ins)) {
        return is_accepted_mnemonic(INS_Opcode(ins));
    }
    return 0;
}

void insns(INS ins, void *v)
{
    if(should_instrument_ins(ins) != 0) {
        log_ins(ins);
    }
}

int main(int argc, char *argv[])
{
    int log_taint = 0;
    for (int i = 3; i < argc; i++) {
        if(!strcmp(argv[i], "--main-module")) {
            set_main_module();
            log_info("Whitelisted the main module\n");
        }
        else if(!strcmp(argv[i], "--range")) {
            ADDRINT start = strtoul(argv[++i], NULL, 16);
            ADDRINT end = strtoul(argv[++i], NULL, 16);
            add_instrument_range(start, end);
            log_info("Whitelisted range 0x%08x..0x%08x\n", start, end);
        }
        else if(!strcmp(argv[i], "--ins")) {
            log_info("Whitelisted mnemonic(s): %s\n", argv[++i]);
            add_instrument_instruction(argv[i]);
        }
        else if(!strcmp(argv[i], "--inside")) {
            ADDRINT start = strtoul(argv[++i], NULL, 16);
            ADDRINT end = strtoul(argv[++i], NULL, 16);
            add_instrument_inside(start, end);
            log_info("Whitelisted inside sequence 0x%08x..0x%08x\n",
                start, end);
        }
        else if(!strcmp(argv[i], "--module")) {
            add_instrument_module(argv[++i]);
            log_info("Whitelisted modules containing: %s\n", argv[i]);
        }
    }

    PIN_Init(argc, argv);
    PIN_InitSymbols();

    registers_init();
    taint_init(log_taint);

    taint_source_enable("getc");
    taint_source_enable("scanf");

    IMG_AddInstrumentFunction(&module_range_handler, NULL);
    IMG_AddInstrumentFunction(&taint_sources_handler, NULL);
    INS_AddInstrumentFunction(&insns, NULL);

    PIN_StartProgram();
    return 0;
}
