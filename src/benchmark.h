#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "cli.h"

int run_headless_benchmark(const CliOptions *opt, int screen_width, int screen_height);

int run_diff(const CliOptions *opt);

#endif