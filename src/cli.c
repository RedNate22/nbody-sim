#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "cli.h"
#include "body.h"

/**
 * Checks whether arg matches the pattern "--flag=value".
 *
 * @param arg The argv string being checked.
 * @param flag The flag name to match, including its leading "--".
 * @param out_value Set to point at the text after '=' when arg matches flag.
 * @return true if arg starts with flag followed by '=', false otherwise.
 */
static bool arg_value(const char *arg, const char *flag, const char **out_value) {
    size_t flag_len = strlen(flag);
    if (strncmp(arg, flag, flag_len) != 0) return false;
    if (arg[flag_len] != '=') return false;
    *out_value = arg + flag_len + 1;
    return true;
}

/**
 * Parses command line arguments into a CliOptions struct.
 *
 * @param argc Argument count, as passed to main.
 * @param argv Argument vector, as passed to main.
 * @param opt Filled with default values, then overridden by any recognized
 *            flags found in argv. Unrecognized arguments are printed to
 *            stderr and otherwise ignored.
 */
void parse_cli_options(int argc, char *argv[], CliOptions *opt) {
    opt->headless = false;
    opt->compare = false;
    opt->mode = 0;
    opt->bodies = DEFAULT_STAR_SCENARIO_BODY_COUNT;
    opt->scenario_path = NULL;
    opt->out_path = NULL;
    opt->compare_a = NULL;
    opt->compare_b = NULL;
    opt->dt = 1.0f / 60.0f;
    opt->steps = 3600;
    opt->tol = 1e-3f;
    opt->print_count = 0;

    const char *value;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--headless") == 0) {
            opt->headless = true;
        } else if (arg_value(argv[i], "--mode", &value)) {
            opt->mode = atoi(value);
        } else if (arg_value(argv[i], "--bodies", &value)) {
            opt->bodies = atoi(value);
        } else if (arg_value(argv[i], "--scenario", &value)) {
            opt->scenario_path = value;
        } else if (arg_value(argv[i], "--out", &value)) {
            opt->out_path = value;
        } else if (arg_value(argv[i], "--dt", &value)) {
            opt->dt = (float)atof(value);
        } else if (arg_value(argv[i], "--steps", &value)) {
            opt->steps = strtoul(value, NULL, 10);
        } else if (arg_value(argv[i], "--tol", &value)) {
            opt->tol = (float)atof(value);
        } else if (arg_value(argv[i], "--compare-a", &value)) {
            opt->compare_a = value;
            opt->compare = true;
        } else if (arg_value(argv[i], "--compare-b", &value)) {
            opt->compare_b = value;
            opt->compare = true;
        } else if (arg_value(argv[i], "--print", &value)) {
            opt->print_count = atoi(value);
        }
        
        else {
            fprintf(stderr, "unrecognized argument: %s\n", argv[i]);
        }
    }
}