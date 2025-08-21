#include "config.h"
#include "log.h"
#include "target.h"
#include "util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool parse_config(Config *conf, int argc, char **argv, Arena* arena) {
    conf->exe_name = *argv;
    argc--;
    argv++;

    while (argc > 0) {
        if (strcmp(*argv, "-o") == 0) {
            if (conf->output_name != NULL) {
                log_diagnostic(LL_ERROR, "Found a duplicate output file name flag");
                return false;
            }
            argc--;
            argv++;
            if (argc <= 0) {
                log_diagnostic(LL_ERROR, "Expected a file name to be here to set a custom output file");
                return false;
            }
            conf->output_name = *argv;
            argc--;
            argv++;
        } else if (strcmp(*argv, "-keep-artifacts") == 0) {
            conf->keep_build_artifacts = true;
            argc--;
            argv++;
        } else if (strcmp(*argv, "-help") == 0) {
            usage(conf->exe_name);
            exit(0);
        } else if (strcmp(*argv, "-list-targets") == 0) {
            for (TargetKind tk = 0; tk < TK_Count; tk++) { log_diagnostic(LL_INFO, "%s", target_enum_to_str(tk)); }
            exit(0);
        } else if (strcmp(*argv, "-target") == 0) {
            argc--;
            argv++;
            if (argc <= 0) {
                log_diagnostic(LL_ERROR, "Expected a target name (run %s -list-targets)", conf->exe_name);
                return false;
            }
            Target *t;
            if (!find_target(&t, *argv)) {
                log_diagnostic(LL_ERROR, "Unknown target name %s (run %s -list-targets)", *argv, conf->exe_name);
                return false;
            } else {
                conf->target = *argv;
                argc--;
                argv++;
            }
        } else {
            if (**argv == '-') {
                log_diagnostic(LL_ERROR, "Unknown flag supplied");
                return false;
            }
            if (conf->input_name != NULL) {
                log_diagnostic(LL_ERROR, "Compiling multiple files at the same time is not supported");
                return false;
            }
            conf->input_name = *argv;
            argc--;
            argv++;
        }
    }

    if (conf->input_name == NULL) {
        log_diagnostic(LL_ERROR, "No input name is provided");
        return false;
    }

    if (conf->output_name == NULL) {
        size_t len = strlen(conf->input_name) - 3;
        conf->output_name = arena_alloc(arena, sizeof(char) * (len + 1));
        conf->output_name[len] = 0;
        conf->should_free_output_name = true;
        snprintf(conf->output_name, len, "%s", conf->input_name);
    }

    return true;
}

void usage(char *program_name) {
    log_diagnostic(LL_INFO, "Usage: ");
    log_diagnostic(LL_INFO, "  %s <input.boa> [FLAGS]", program_name);
    log_diagnostic(LL_INFO, "  [FLAGS]:");
    log_diagnostic(LL_INFO, "    -help           : Show this help message");
    log_diagnostic(LL_INFO, "    -o              : Customize the output file name (format: -o <name>)");
    log_diagnostic(LL_INFO, "    -keep-artifacts : Keep the build artifacts (.asm, .o files)");
    log_diagnostic(LL_INFO, "    -no-opt         : Don't optimize the code");
    log_diagnostic(LL_INFO, "    -target <TARGET>: Select the target");
    log_diagnostic(LL_INFO, "    -list-targets   : List available targets");
}
