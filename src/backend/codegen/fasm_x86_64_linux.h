#ifndef FASM_X86_64_LINUX_H_
#define FASM_X86_64_LINUX_H_

#include <stdio.h>
#include "../ir.h"

bool generate_fasm_x86_64_linux(FILE* out, const IRModule* mod);

#endif
