#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "pin.H"
#include "instructions.h"

static uint32_t g_mnemonic_count = 0;
static int g_mnemonics[XED_ICLASS_LAST];

void add_instrument_instruction(char *mnemonic)
{
    // the mnemonic has to be uppercase, so let's make it uppercase
    for (char *p = mnemonic; *p != 0; p++) {
        *p = toupper(*p);
    }

    xed_iclass_enum_t ins = str2xed_iclass_enum_t(mnemonic);
    if(ins == XED_ICLASS_INVALID) {
        fprintf(stderr, "Invalid mnemnonic: %s\n", mnemonic);
        exit(1);
    }

    g_mnemonic_count++;
    g_mnemonics[ins] = 1;
}

int is_accepted_mnemonic(OPCODE mnemonic)
{
    return g_mnemonic_count == 0 || g_mnemonics[mnemonic] != 0;
}
