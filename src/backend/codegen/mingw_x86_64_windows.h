
#ifndef MINGW_X86_64_WINDOWS_H
#define MINGW_X86_64_WINDOWS_H

#include "../ir/ssa.h"
#include <stdbool.h>
#include <stdio.h>

bool mingw_x86_64_windows_generate_file(FILE *sink, const SSAModule *mod);

#endif
