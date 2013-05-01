#include "pin.H"
#include "addresses.h"
#include "instructions.h"
#include "logging.h"
#include "registers.h"

#define sizeofarray(x) (sizeof(x)/sizeof((x)[0]))

#ifndef TARGET_LINUX
#define strdup _strdup
#endif
