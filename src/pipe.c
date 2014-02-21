#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "module.h"
#include "bot.h"
#include "pipe.h"
#include "chains/chains.h"

static char bufin[4096];

static int
add_select(module_t *module, fd_set *in_set, fd_set *out_set, int *maxfd) {
	(void) module;
	(void) maxfd;
	(void) out_set;
	FD_SET(STDIN_FILENO, in_set);
	// FD_SET(STDOUT_FILENO, out_set);
	return 1;
}

static int
process_select(module_t *module, fd_set *in_set, fd_set *out_set) {
	(void) module;
	(void) out_set;
	if (FD_ISSET(STDIN_FILENO, in_set)) {
		if(fgets(bufin, sizeof bufin, stdin) == NULL) {
			// pipe closed. remove module
			exit(0);
			/*
			fprintf(stderr, "broken pipe\n");
			bot_remove_module(module->bot, module);
			free(module);
			return 0;
			*/
		}

		// trim
		int len = strlen(bufin);
		if (len > 0 && bufin[len-1] == '\n') {
			bufin[len-1] = '\0';
		}

		bot_on_msg(module->bot, module, NULL, "(stdin)", bufin);
	}
	return 1;
}

static void
send(module_t *module, const char *channel, const char *message) {
	(void) module;
	(void) channel;
	puts(message);
}

module_t *
pipe_new() {
	module_t *module = calloc(1, sizeof(module_t));
	if (!module) {
		perror("calloc");
		return NULL;
	}

	module->type = "pipe";
	module->name = "(stdout)";
	module->process_select_descriptors = process_select;
	module->add_select_descriptors = add_select;
	module->send = send;

	return module;
}

