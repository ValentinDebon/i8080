#define POSIXLY_CORRECT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include "i8080/cpu.h"

#include "board.h"
#include "board/cpm.h"
#include "board/space_invaders.h"

struct i8080_args {
	const struct i8080_board *board;
	const char *preset, *plugin;
};

static const struct i8080_preset {
	const char *name;
	const struct i8080_board *board;
} presets[] = {
	{ "CP/M", &cpm_board },
	{ "space-invaders", &space_invaders_board },
};

static const struct option longopts[] = {
	{ "preset", required_argument },
	{ "plugin", required_argument },
	{ },
};

static const struct i8080_board *
i8080_preset_find(const char *preset) {
	const struct i8080_preset *current = presets, *end = presets + sizeof(presets) / sizeof(*presets);

	while(current != end && strcasecmp(current->name, preset) != 0) {
		current++;
	}

	if(current == end) {
		fprintf(stderr, "Unable to find preset named '%s'\n", preset);
		exit(EXIT_FAILURE);
	}

	return current->board;
}

static void
i8080_usage(const char *i8080name) {
	fprintf(stderr, "usage: %s [-preset <preset> | -plugin <plugin>] program\n", i8080name);
	exit(EXIT_FAILURE);
}

static struct i8080_args
i8080_parse_args(int argc, char **argv) {
	struct i8080_args args = {
		.board = &cpm_board,
		.preset = NULL,
		.plugin = NULL,
	};
	int longindex, c;

	while(c = getopt_long_only(argc, argv, ":", longopts, &longindex), c != -1) {
		switch(c) {
		case 0:
			switch(longindex) {
			case 0: args.preset = optarg; break;
			case 1: args.plugin = optarg; break;
			}
			break;
		case '?':
			fprintf(stderr, "%s: Invalid option %s\n", *argv, argv[optind - 1]);
			i8080_usage(*argv);
		case ':':
			fprintf(stderr, "%s: Missing option argument after -%s\n", *argv, longopts[longindex].name);
			i8080_usage(*argv);
		}
	}

	if(args.preset != NULL) {
		if(args.plugin != NULL) {
			fprintf(stderr, "%s: Cannot specify both a preset (%s) and a plugin (%s)\n", *argv, args.preset, args.plugin);
			i8080_usage(*argv);
		}

		args.board = i8080_preset_find(args.preset);
	}

	if(args.plugin != NULL) {
		fprintf(stderr, "%s: Plugins not yet supported!\n", *argv);
		exit(EXIT_FAILURE);
	}

	if(argc - optind != 1) {
		fprintf(stderr, "%s: Expected one program file\n", *argv);
		i8080_usage(*argv);
	}

	return args;
}

int
main(int argc, char **argv) {
	const struct i8080_args args = i8080_parse_args(argc, argv);
	const struct i8080_board * const board = args.board;
	const char * const program = argv[optind];
	struct i8080_cpu cpu;

	i8080_cpu_init(&cpu, board->io);

	board->setup(&cpu, program);

	while(board->isonline(&cpu)) {
		board->poll(&cpu);

		i8080_cpu_next(&cpu);

		board->sync(&cpu);
	}

	board->teardown(&cpu);

	i8080_cpu_deinit(&cpu);

	return EXIT_SUCCESS;
}
