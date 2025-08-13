#include "util.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int run_program(const char *prog, char *args[]) {
    pid_t pi = fork();

    if (pi == -1) {
        log_message(LL_ERROR, "Failed to fork :(");
        return -1;
    }
    if (pi == 0) {
        if (execvp(prog, args) == -1) {
            log_message(LL_ERROR, "Failed to execute %s: %s", prog, strerror(errno));
            exit(-1);
        }
    } else {
        for (;;) {
            int status = 0;
            if (waitpid(pi, &status, 0) == -1) {
                log_message(LL_ERROR, "Failed to wait on %d: %s", pi, strerror(errno));
                return -1;
            }
            if (WIFEXITED(status)) { return WEXITSTATUS(status); }
        }
    }
    return 0;
}

bool read_source_file(const char *file_name, SourceFile *out) {
    ASSERT(file_name, "Passed in NULL for the file name");
    ASSERT(out, "Passed in NULL for the out source file parameter");

    if (!read_file(file_name, &out->src)) return false;
    out->name = file_name;
    return true;
}

bool read_file(const char *file_name, String *s) {
    if (file_name == NULL) return false;
    memset(s, 0, sizeof(String));
    FILE *file = fopen(file_name, "r");
    if (file == NULL) {
        log_message(LL_ERROR, "Failed to open file %s: %s", file_name, strerror(errno));
        return false;
    }

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    s->count = size;
    s->capacity = size;
    s->items = malloc(sizeof(char) * size + 1);
    fseek(file, 0, SEEK_SET);
    size_t read = fread(s->items, sizeof(char), size, file);
    if (read != size) {
        log_message(LL_ERROR, "Failed to read whole file %s");
        return false;
    }

    return true;
}
