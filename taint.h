#include "xed-iclass-enum.h"
#include "taint-handlers.h"
#include "taint-sources.h"

void taint_init(int log_taint);
void taint_ins(INS ins, void *v);
