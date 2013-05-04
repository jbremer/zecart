#include <stdio.h>
#include <map>
#include "pin.H"
#include "registers.h"
#include "taint-handlers.h"

std::map<std::string, uint32_t> g_enabled_sources;

static void taint_source_getc(const CONTEXT *ctx)
{
    taint_register(0);
}

// TODO works only for x86, as x64 has fastcall..
static void taint_source_scanf(const CONTEXT *ctx, const char **_fmt)
{
    ADDRINT *addr = (ADDRINT *) _fmt;

    uint32_t idx = 1, size = 0;
    for (const char *fmt = *_fmt; *fmt != 0; fmt++) {
        if(*fmt != '%') continue;
        switch (*++fmt) {
        case '%': case '-':
            break;

        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            size = (size * 10) + (*fmt - '0');
            break;

        case 'l':
            size = 8;
            break;

        case 'x': case 'i': case 'u':
            taint_memory(addr[idx++], size == 0 ? 4 : size);
            break;

        case 's':
            // let's assume that when there's no limit we don't abuse the
            // buffer overflow, but rather fill upto the first 128 characters
            taint_memory(addr[idx++], size == 0 ? 128 : size);
            break;
        }
    }
}

#define T(when, name, ...) { \
    RTN rtn = RTN_FindByName(img, #name); \
    if(g_enabled_sources[#name] != 0 && RTN_Valid(rtn)) { \
        printf("enabled taint source %s\n", #name); \
        RTN_Open(rtn); \
        RTN_InsertCall(rtn, IPOINT_##when, (AFUNPTR) &taint_source_##name, \
            IARG_CONST_CONTEXT, ##__VA_ARGS__, IARG_END); \
        RTN_Close(rtn); \
    }}

void taint_sources_handler(IMG img, void *v)
{
    T(AFTER, getc);
    T(BEFORE, scanf, IARG_FUNCARG_ENTRYPOINT_REFERENCE, 0);
}

void taint_source_enable(const char *funcname)
{
    g_enabled_sources[funcname] = 1;
}
