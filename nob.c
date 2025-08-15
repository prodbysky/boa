#include <string.h>
#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define BUILD_DIR "build"
#define TEST_DIR "tests"
#define SOURCES "src/log.c", "src/frontend/lexer.c", "src/arena.c", "src/frontend/parser.c", "src/backend/ir/ssa.c", "src/backend/codegen/nasm_x86_64_linux.c", "src/util.c", "src/backend/ir/pseudo_reg.c"

#ifdef _WIN32
    // @sa.pohod ty <3
    #error "Your Windows version is too old for this software.\nGet up to date at https://kernel.org !";
#endif

void common_flags(Cmd *cmd);

[[nodiscard]] bool run_tests();

int main(int argc, char *argv[]) {
    NOB_GO_REBUILD_URSELF(argc, argv);


    if (argc > 1 && strcmp(argv[1], "test")) {
        if (!run_tests()) {
            nob_log(NOB_ERROR, "Some tests failed. Go check the logs :)");
            return 1;
        }
        return 0;
    }

    Cmd cmd = {0};

    mkdir_if_not_exists(BUILD_DIR);
    mkdir_if_not_exists(BUILD_DIR "/" TEST_DIR);

    if (!run_tests()) {
        nob_log(NOB_ERROR, "Some tests failed. Go check the logs :)");
        return 1;
    }

    cmd_append(&cmd, "cc", SOURCES, "src/main.c", "-o", BUILD_DIR "/boa");
    common_flags(&cmd);

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    return 0;
}

bool run_tests() {
    nob_log(NOB_INFO, "Running tests...");

    File_Paths test_paths = {0};
    read_entire_dir(TEST_DIR, &test_paths);

    Cmd cmd = {0};

    Procs test_compilations = {0};

    for (int i = 0; i < (int)test_paths.count; i++) {
        if (*test_paths.items[i] == '.') continue;
        char *path = temp_sprintf(TEST_DIR "/%s", test_paths.items[i]);

        char *exe_path = temp_sprintf(BUILD_DIR "/" TEST_DIR "/%s", test_paths.items[i]);
        exe_path[strlen(exe_path) - 2] = 0; // strip .c suffix

        cmd_append(&cmd, "cc");
        common_flags(&cmd);
        cmd_append(&cmd, SOURCES, path, "-o", exe_path);
        if (!nob_procs_append_with_flush(&test_compilations, cmd_run_async_and_reset(&cmd), 16)) {
            nob_log(NOB_WARNING, "Failed to build %s test", path);
            return false;
        }
    }

    if (!procs_wait_and_reset(&test_compilations)) {
        nob_log(NOB_WARNING, "Failed to build some test");
        return false;
    }

    for (int i = 0; i < (int)test_paths.count; i++) {
        if (*test_paths.items[i] == '.') continue;

        char *exe_path = temp_sprintf("./" BUILD_DIR "/" TEST_DIR "/%s", test_paths.items[i]);
        exe_path[strlen(exe_path) - 2] = 0; // strip .c suffix

        cmd_append(&cmd, exe_path);
        if (!nob_cmd_run_sync_and_reset(&cmd)) {
            nob_log(NOB_WARNING, "%s test failed", exe_path);
            return false;
        }
    }

    return true;
}

void common_flags(Cmd *cmd) { cmd_append(cmd, "-Wall", "-Wextra", "-Werror", "-std=c23", "-O0", "-g", "-Wno-nonnull", "-Wno-format-overflow" ); }
