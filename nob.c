#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#include "nob.h"

#define BUILD_DIR "build"

#define SOURCES "src/log.c"

void common_flags(Cmd* cmd);

int main(int argc, char *argv[]) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Cmd cmd = {0};

    mkdir_if_not_exists(BUILD_DIR);
    cmd_append(&cmd, "cc", SOURCES, "src/main.c", "-o", BUILD_DIR"/boa");
    common_flags(&cmd);

    if (!nob_cmd_run_sync_and_reset(&cmd)) return 1;

    return 0;
}

void common_flags(Cmd* cmd) {
    cmd_append(cmd, "-Wall", "-Wextra", "-Werror", "-std=c17", "-pedantic", "-O3", "-g");
}
