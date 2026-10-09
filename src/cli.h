#ifndef CLI_H
#define CLI_H
#include <stdbool.h>

typedef struct {
    bool headless;
    bool compare;
    int mode;
    int bodies;
    const char *scenario_path;
    const char *out_path;
    const char *compare_a;
    const char *compare_b;
    float dt;
    unsigned long steps;
    float tol;
    int print_count;
    int threads;
} CliOptions;

void parse_cli_options(int argc, char *argv[], CliOptions *opt);

#endif