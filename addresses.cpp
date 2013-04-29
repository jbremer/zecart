#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "pin.H"
#include "zecart.h"

// whitelist the main module
static uint32_t g_main_module = 0;

// whitelisted ranges
static uint32_t g_range_count = 0;
static ADDRINT g_range_start[0x100];
static ADDRINT g_range_end[0x100];

// data about the inside sequences
static uint32_t g_inside_count = 0;
static ADDRINT g_inside_start[0x100];
static ADDRINT g_inside_end[0x100];

// current inside counter
static uint32_t g_inside_cur = 0;

// whitelisted modules
static uint32_t g_module_count = 0;
static const char *g_modules[0x100];


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

void add_instrument_inside(ADDRINT start, ADDRINT end)
{
    if(g_inside_count == sizeofarray(g_inside_start)) {
        fprintf(stderr, "Specified too many inside sequences!\n");
        exit(1);
    }

    g_inside_start[g_inside_count] = start;
    g_inside_end[g_inside_count++] = end;
}

void add_instrument_module(const char *module)
{
    if(g_module_count == sizeofarray(g_modules)) {
        fprintf(stderr, "Specified too many modules!\n");
        exit(1);
    }

    g_modules[g_module_count++] = _strdup(module);
}

static void increase_inside_cur()
{
    g_inside_cur++;
}

static void decrease_inside_cur()
{
    if(g_inside_cur == 0) {
        fprintf(stderr, "Negative inside sequence count?!\n");
        exit(1);
    }

    g_inside_cur--;
}

int is_inside_sequence(INS ins)
{
    ADDRINT addr = INS_Address(ins);
    for (uint32_t i = 0; i < g_inside_count; i++) {
        // enter a sequence
        if(g_inside_start[i] == addr) {
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &increase_inside_cur,
                IARG_END);

            // just to make sure..
            g_inside_cur++;
            return 1;
        }

        // leave a sequence
        if(g_inside_end[i] == addr) {
            // TODO after the instruction, but then we have to make sure
            // that it's not a branch etc.
            INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) &decrease_inside_cur,
                IARG_END);

            // fixup
            g_inside_cur--;
            return 1;
        }
    }

    return g_inside_cur != 0;
}

int is_accepted_address(INS ins)
{
    // if main-module is not specified, there are no whitelisted ranges, there
    // are no inside sequences, and no whitelisted modules, then every
    // instruction should be logged
    if(g_main_module == 0 && g_range_count == 0 && g_inside_count == 0 &&
            g_module_count == 0) {
        return 1;
    }

    ADDRINT addr = INS_Address(ins);

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

    // check for inside sequences
    return is_inside_sequence(ins);
}

void module_range_handler(IMG img, void *v)
{
    const char *imgname = IMG_Name(img).c_str();
    for (uint32_t i = 0; i < g_module_count; i++) {
        if(strstr(imgname, g_modules[i]) != NULL) {
            ADDRINT start = IMG_LowAddress(img);
            ADDRINT end = IMG_HighAddress(img);
            add_instrument_range(start, end);

            log_info("Module %s covers range 0x%08x..0x%08x\n",
                imgname, start, end);
        }
    }
}
