#include <stdio.h>
#include "cli.h"
#include "benchmark.h"

#define DEFAULT_SCREEN_WIDTH 1920
#define DEFAULT_SCREEN_HEIGHT 1080

int main(int argc, char *argv[]) {
	CliOptions opt;
	parse_cli_options(argc, argv, &opt);
	if (opt.compare) return run_diff(&opt);
	if (!opt.headless) {
		fprintf(stderr, "nbody_headless has no window, pass --headless\n");
		return 1;
	}
	return run_headless_benchmark(&opt, DEFAULT_SCREEN_WIDTH, DEFAULT_SCREEN_HEIGHT);
}