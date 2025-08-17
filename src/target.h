#include "backend/ir/ssa.h"

typedef enum {
    TK_Linux_x86_64_NASM,
    TK_Win_x86_64_MINGW,
} TargetKind;

typedef struct {
    const char *name;
    TargetKind tk;
    bool (*generate)(char *root_path, const SSAModule *mod);
    bool (*assemble)(char *root_path);
    bool (*link)(char *root_path);
    void (*cleanup)(char *root_path);
} Target;

Target* find_target(const char* name);
const char* target_enum_to_str(TargetKind kind);


