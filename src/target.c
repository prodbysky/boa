#include "target.h"
#include "backend/codegen/nasm_x86_64_linux.h"
#include "util.h"
#include <stdio.h>

static bool linux_nasm_gen(char *root_path, const SSAModule *mod);
static bool linux_nasm_assemble(char *root_path);
static bool linux_nasm_link(char *root_path);
static void linux_nasm_cleanup(char *root_path);


static Target TARGET_LINUX_NASM = {
    .tk = TK_Linux_x86_64_NASM,
    .name = "linux_nasm",
    .generate = linux_nasm_gen,
    .assemble = linux_nasm_assemble,
    .link = linux_nasm_link,
    .cleanup = linux_nasm_cleanup,
};

static Target *targets[] = {&TARGET_LINUX_NASM, NULL};

bool find_target(Target** out, const char* name) {
    for (size_t i = 0; targets[i] != NULL; i++) {
        if (strcmp(targets[i]->name, name) == 0) {
            *out = targets[i];
            return true;
        }
    }
    return false;
}

const char *target_enum_to_str(TargetKind kind) {
    for (size_t i = 0; targets[i] != NULL; i++) {
        if (targets[i]->tk == kind) return targets[i]->name;
    }
    return NULL;
}


static bool linux_nasm_gen(char *root_path, const SSAModule *mod) {
    Path p = path_from_cstr(root_path);
    path_add_ext(&p, "asm");
    char *p_c = path_to_cstr(&p);
    free(p.path.items);

    FILE *f = fopen(p_c, "wb");
    bool result = nasm_x86_64_linux_generate_file(f, mod);

    fclose(f);
    free(p_c);

    return result;
}

static bool linux_nasm_assemble(char *root_path) {
    Path p_asm = path_from_cstr(root_path);
    Path p_o = path_from_cstr(root_path);

    path_add_ext(&p_asm, "asm");
    char *p_asm_c = path_to_cstr(&p_asm);
    free(p_asm.path.items);

    path_add_ext(&p_o, "o");
    char *p_o_c = path_to_cstr(&p_o);
    free(p_o.path.items);

    if (run_program("nasm", 5, (char *[]){p_asm_c, "-f", "elf64", "-o", p_o_c, NULL}) != 0) {
        log_diagnostic(LL_ERROR, "nasm failed");
        free(p_asm_c);
        free(p_o_c);
        return false;
    }
    free(p_asm_c);
    free(p_o_c);
    return true;
}

static bool linux_nasm_link(char *root_path) {
    Path p_o = path_from_cstr(root_path);
    path_add_ext(&p_o, "o");
    char *p_o_c = path_to_cstr(&p_o);
    free(p_o.path.items);

    if (run_program("ld", 3, (char *[]){p_o_c, "-o", root_path, NULL}) != 0) {
        log_diagnostic(LL_ERROR, "ld failed");
        free(p_o_c);
        return false;
    }

    free(p_o_c);
    return true;
}

static void linux_nasm_cleanup(char *root_path) {
    Path p_asm = path_from_cstr(root_path);
    Path p_o = path_from_cstr(root_path);

    path_add_ext(&p_asm, "asm");
    char *p_asm_c = path_to_cstr(&p_asm);
    free(p_asm.path.items);

    path_add_ext(&p_o, "o");
    char *p_o_c = path_to_cstr(&p_o);
    free(p_o.path.items);

    remove(p_o_c);
    remove(p_asm_c);
    free(p_o_c);
    free(p_asm_c);
}
