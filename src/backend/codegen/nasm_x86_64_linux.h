#ifndef NASM_X86_64_LINUX_H
#define NASM_X86_64_LINUX_H

#include "../ir/ssa.h"
#include <stdio.h>

bool nasm_x86_64_linux_generate_file(FILE* sink, const SSAModule* mod);

#endif
