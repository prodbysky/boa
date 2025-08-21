#ifndef CONFIG_H_
#define CONFIG_H_

#include "arena.h"
#include "target.h"

typedef struct {
    char *exe_name;
    char *input_name;
    char *output_name;
    Target* target;
    bool should_free_output_name;
    bool keep_build_artifacts;
} Config;

bool parse_config(Config *conf, int argc, char **argv, Arena* arena);

void usage(char *program_name);

#endif

