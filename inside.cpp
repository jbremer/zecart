#include <stdio.h>
#include <stdint.h>
#include "zecart.h"

// store global data
static uint32_t g_inside_count = 0;
static ADDRINT g_inside_start[0x100];
static ADDRINT g_inside_end[0x100];

// current inside counter
static uint32_t g_inside_cur = 0;

void add_instrument_inside(ADDRINT start, ADDRINT end)
{
    if(g_inside_count == sizeofarray(g_inside_start)) {
        fprintf(stderr, "Specified too many inside sequences!\n");
        exit(1);
    }

    g_inside_start[g_inside_count] = start;
    g_inside_end[g_inside_count++] = end;
}

int is_inside_sequence(INS ins)
{
    ADDRINT addr = INS_Address(ins);
    for (uint32_t i = 0; i < g_inside_count; i++) {
        // enter a sequence
        if(g_inside_start[i] == addr) {
            g_inside_cur++;
            return 1;
        }

        // leave a sequence
        if(g_inside_end[i] == addr) {
            if(g_inside_cur == 0) {
                fprintf(stderr, "Negative inside sequence count?!\n");
                exit(1);
            }
            g_inside_cur--;
            break;
        }
    }

    return g_inside_cur != 0;
}
