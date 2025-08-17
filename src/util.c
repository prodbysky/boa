#include "util.h"
#include "sv.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#ifdef __unix__
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

Path path_from_cstr(char *cstr) {
    return (Path){
        .path =
            (String){
                .items = strndup(cstr, strlen(cstr)),
                .count = strlen(cstr),
                .capacity = strlen(cstr),
            },
    };
}

void path_add_ext(Path *path, const char *ext) {
    da_push(&path->path, '.');
    for (size_t i = 0; i < strlen(ext); i++) { da_push(&path->path, ext[i]); }
}

char *path_to_cstr(Path *path) {
    char *c_str = malloc(sizeof(char) * (path->path.count + 1));
    c_str[path->path.count] = 0;
    strncpy(c_str, path->path.items, path->path.count);
    return c_str;
}

int run_program(const char *program, int argc, char *argv[]) {
#ifdef _WIN32
    char cmdline[4096] = {0};
    int pos = snprintf(cmdline, sizeof(cmdline), "%s", program);

    for (int i = 0; i < argc; i++) {
        pos += snprintf(cmdline + pos, sizeof(cmdline) - pos, " %s", argv[i]);
        if (pos >= (int)sizeof(cmdline)) {
            fprintf(stderr, "Command line too long\n");
            return -1;
        }
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessA(NULL,       // application name
                        cmdline,    // command line (mutable buffer)
                        NULL, NULL, // security attributes
                        FALSE,      // inherit handles
                        0,          // creation flags
                        NULL, NULL, // environment, current directory
                        &si, &pi)) {
        fprintf(stderr, "CreateProcess failed (error %lu)\n", GetLastError());
        return -1;
    }

    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code;
    if (!GetExitCodeProcess(pi.hProcess, &exit_code)) exit_code = -1;

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return (int)exit_code;
#else
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
#endif
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
        fclose(file);
        return false;
    }
    fclose(file);

    return true;
}
