#include "target.h"
#include "backend/codegen/mingw_x86_64_windows.h"
#include "backend/codegen/nasm_x86_64_linux.h"
#include "util.h"

static bool linux_nasm_gen(char *root_path, const SSAModule *mod);
static bool linux_nasm_assemble(char *root_path);
static bool linux_nasm_link(char *root_path);
static void linux_nasm_cleanup(char *root_path);

static bool windows_mingw_gen(char *root_path, const SSAModule *mod);
static bool windows_mingw_assemble(char *root_path);
static bool windows_mingw_link(char *root_path);
static void windows_mingw_cleanup(char *root_path);

static Target TARGET_LINUX_NASM = {
    .tk = TK_Linux_x86_64_NASM,
    .name = "linux_nasm",
    .generate = linux_nasm_gen,
    .assemble = linux_nasm_assemble,
    .link = linux_nasm_link,
    .cleanup = linux_nasm_cleanup,
};

static Target TARGET_WINDOWS_MINGW = {
    .tk = TK_Win_x86_64_MINGW,
    .name = "windows_mingw",
    .generate = windows_mingw_gen,
    .assemble = windows_mingw_assemble,
    .link = windows_mingw_link,
    .cleanup = windows_mingw_cleanup,
};

static Target *targets[] = {&TARGET_LINUX_NASM, &TARGET_WINDOWS_MINGW, NULL};

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

static bool windows_mingw_gen(char *root_path, const SSAModule *mod) {
    Path p = path_from_cstr(root_path);
    path_add_ext(&p, "s");
    char *p_c = path_to_cstr(&p);
    free(p.path.items);

    FILE *f = fopen(p_c, "wb");
    bool result = mingw_x86_64_windows_generate_file(f, mod);

    fclose(f);
    free(p_c);

    return result;
}

static bool windows_mingw_assemble(char *root_path) {
    Path p_asm = path_from_cstr(root_path);
    Path p_o = path_from_cstr(root_path);

    path_add_ext(&p_asm, "s");
    char *p_asm_c = path_to_cstr(&p_asm);
    free(p_asm.path.items);

    path_add_ext(&p_o, "obj");
    char *p_o_c = path_to_cstr(&p_o);
    free(p_o.path.items);

    if (run_program("x86_64-w64-mingw32-as", 3, (char *[]){p_asm_c, "-o", p_o_c, NULL}) != 0) {
        log_diagnostic(LL_ERROR, "as failed");
        free(p_asm_c);
        free(p_o_c);
        return false;
    }
    free(p_asm_c);
    free(p_o_c);
    return true;
}
static bool windows_mingw_link(char *root_path) {
    Path p_o = path_from_cstr(root_path);
    path_add_ext(&p_o, "obj");
    char *p_o_c = path_to_cstr(&p_o);
    free(p_o.path.items);

    Path p_final = path_from_cstr(root_path);
    path_add_ext(&p_o, "exe");
    char *p_final_c = path_to_cstr(&p_final);
    free(p_final.path.items);

    // holy chunk
    if (run_program("x86_64-w64-mingw32-gcc", 8,
                    (char *[]){
                        p_o_c,
                        "-o",
                        p_final_c,
                        "-e",
                        "_start",
                        "-nostdlib",
                        "-Wl,--subsystem=console",
                        "-lkernel32",
                        NULL,
                    }) != 0) {
        log_diagnostic(LL_ERROR, "ld failed");
        free(p_o_c);
        free(p_final_c);
        return false;
    }

    free(p_final_c);
    return true;
}
static void windows_mingw_cleanup(char *root_path) {
    Path p_s = path_from_cstr(root_path);
    Path p_obj = path_from_cstr(root_path);

    path_add_ext(&p_s, "s");
    char *p_s_c = path_to_cstr(&p_s);
    free(p_s.path.items);

    path_add_ext(&p_obj, "obj");
    char *p_obj_c = path_to_cstr(&p_obj);
    free(p_obj.path.items);

    run_program("del", 2, (char *[]){p_obj_c, p_s_c, NULL});
    free(p_obj_c);
    free(p_s_c);
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

    run_program("rm", 2, (char *[]){p_o_c, p_asm_c, NULL});
    free(p_o_c);
    free(p_asm_c);
}
