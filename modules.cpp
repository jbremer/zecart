#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "pin.H"
#include "zecart.h"

static uint32_t g_main_module = 0;

static uint32_t g_range_count = 0;
static ADDRINT g_range_start[0x100];
static ADDRINT g_range_end[0x100];

void set_main_module()
{
    g_main_module = 1;
}

void add_instrument_range(ADDRINT start, ADDRINT end)
{
    if(g_range_count == sizeofarray(g_range_start)) {
        fprintf(stderr, "Specified too many ranges!\n");
        exit(1);
    }

    g_range_start[g_range_count] = start;
    g_range_end[g_range_count++] = end;
}

int is_accepted_address(ADDRINT addr)
{
    // if main-module is not specified, there's no whitelisted ranges, and
    // there are no whitelisted instrutions, then we instrument everything
    int ret = g_main_module == 0 && g_range_count == 0;

    // check if we're in the main module
    if(g_main_module != 0) {
        IMG img = IMG_FindByAddress(addr);
        int main_module = IMG_Valid(img) ? IMG_IsMainExecutable(img) : 0;

        if(main_module == 1) {
            return 1;
        }
    }

    // check if the instruction lays in one of the given ranges
    for (uint32_t i = 0; i < g_range_count; i++) {
        if(addr >= g_range_start[i] && addr < g_range_end[i]) {
            return 1;
        }
    }

    return ret;
}
