#ifndef TARGET_H_
#define TARGET_H_
#include "backend/ir/ssa.h"

typedef enum {
    TK_Linux_x86_64_NASM,
    TK_Count,
} TargetKind;

typedef struct {
    const char *name;
    TargetKind tk;
    bool (*generate)(char *root_path, const SSAModule *mod, Arena* arena);
    bool (*assemble)(char *root_path, Arena* arena);
    bool (*link)(char *root_path, Arena* arena);
    void (*cleanup)(char *root_path, Arena* arena);
} Target;

bool find_target(Target** out, const char* name);
const char* target_enum_to_str(TargetKind kind);

#endif
