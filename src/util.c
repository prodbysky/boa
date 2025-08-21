#include "util.h"
#include "arena.h"
#include "sv.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

Path path_from_cstr(char *cstr, Arena *arena) {
    char *alloc = arena_alloc(arena, strlen(cstr));
    memcpy(alloc, cstr, strlen(cstr));
    return (Path){
        .path =
            (String){
                .items = alloc,
                .count = strlen(cstr),
                .capacity = strlen(cstr),
            },
    };
}

void path_add_ext(Path *path, const char *ext, Arena* arena) {
    da_push(&path->path, '.', arena);
    for (size_t i = 0; i < strlen(ext); i++) { da_push(&path->path, ext[i], arena); }
}

char *path_to_cstr(Path *path, Arena *arena) {
    char *c_str = arena_alloc(arena, sizeof(char) * (path->path.count + 1));
    c_str[path->path.count] = 0;
    strncpy(c_str, path->path.items, path->path.count);
    return c_str;
}

int run_program(const char *program, int argc, char *argv[]) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        char **exec_argv = malloc((argc + 2) * sizeof(char *));
        if (!exec_argv) {
            perror("malloc");
            _exit(127);
        }
        exec_argv[0] = (char *)program;
        for (int i = 0; i < argc; i++) { exec_argv[i + 1] = (char *)argv[i]; }
        exec_argv[argc + 1] = NULL;

        execvp(program, exec_argv);
        perror("execvp");
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return -1;
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    } else {
        return -1;
    }
}

bool read_source_file(const char *file_name, SourceFile *out, Arena* arena) {
    ASSERT(file_name, "Passed in NULL for the file name");
    ASSERT(out, "Passed in NULL for the out source file parameter");

    if (!read_file(file_name, &out->src, arena)) return false;
    out->name = file_name;
    return true;
}

bool read_file(const char *file_name, String *s, Arena* arena) {
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
    s->items = arena_alloc(arena, sizeof(char) * size + 1);
    fseek(file, 0, SEEK_SET);
    size_t read = fread(s->items, sizeof(char), size, file);
    if (read != size) {
        log_message(LL_ERROR, "Failed to read whole file %s");
        fclose(file);
        return false;
    }
    fclose(file);

    return true;
}
