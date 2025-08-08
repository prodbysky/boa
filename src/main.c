#include "log.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

static void usage(char* program_name);


#define STR_FMT "%.*s"
#define STR_ARG(s) (int)(s).count, (s).items

typedef struct {
    char* items;
    size_t count;
    size_t capacity;
} String;

/*
 * Argument `file_name`: input file name (null-terminated C-str)
 * Argument `s`: output parameter which will be filled out
 * Return: indicates failure (where false is failed read operation)
*/
bool read_file(const char* file_name, String* s);

int main(int argc, char **argv) {
    // TODO: Unhardcode the argument parsing
    char* program_name = argv[0];
    if (argc < 2) {
        usage(program_name);
        return 1;
    }
    char* input_name = argv[1];

    String s = {0};

    if (!read_file(input_name, &s)) return 1;
    log_message(LL_INFO, STR_FMT, STR_ARG(s));
}

static void usage(char* program_name) {
    log_message(LL_INFO, "Usage: ");
    log_message(LL_INFO, "  %s <input.boa>", program_name);
}


bool read_file(const char* file_name, String* s) {
    if (file_name == NULL) return false;
    memset(s, 0, sizeof(String));
    FILE* file = fopen(file_name, "r");
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
