#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "inih/ini.h"
#include "bot.h"
#include "module.h"

static bot_t bot = {0};

static int
bot_config_handler(void* obj, const char* section, const char* name,
		const char* value) {
	module_t *module = bot.modules;
	(void)obj;

	if (!module || strcmp(section, bot.modules->type)) {
		// Add new module of given type
		/*
		module_type_t *module_type = module_get_type(section);
		if (!module_type) {
			fprintf(stderr, "Unknown module type '%s'\n", section);
			return 0;
		}
		*/
		module = module_new(section);
		if (!module) {
			fprintf(stderr, "Unable to create module '%s'\n", section);
			//exit(1);
			return 0;
		}
		printf("Created module '%s'\n", section);

		bot_add_module(&bot, module);
	}

	// Configure the module
	module_config(module, name, value);
	return 1;
}

int
main(int argc, char *argv[]) {

	// bot_module_add(irc);

    if (argc != 2) {
        fprintf(stderr, "Usage: %s config.ini\n", argv[0]);
        return 1;
    }

	// Read and process the configuration
    if (ini_parse(argv[1], bot_config_handler, 0) < 0) {
        fprintf(stderr, "Can't load '%s'\n", argv[1]);
        return 1;
    }

	// Start connecting stuff
	bot_connect(&bot);

	// Create the structures for select()
	struct timeval tv;
	fd_set in_set, out_set;
	int maxfd = 0;

	// Wait 0.25 sec for the events - you can wait longer if you want to, but the library has internal timeouts
	// so it needs to be called periodically even if there are no network events
	tv.tv_usec = 250000;
	tv.tv_sec = 0;

	while (1) {

		// Initialize the sets
		FD_ZERO (&in_set);
		FD_ZERO (&out_set);

		bot_add_select_descriptors(&bot, &in_set, &out_set, &maxfd);

		// Call select()
		if (select(maxfd + 1, &in_set, &out_set, 0, &tv) < 0) {
			perror("select");
		}

		// You may also check if any descriptor is active, but again the library needs to handle internal timeouts,
		// so you need to call irc_process_select_descriptors() for each session at least once in a few seconds

		// Call process_select_descriptors() for each session with the descriptor set
		bot_process_select_descriptors(&bot, &in_set, &out_set);
		// if ( process_select_descriptors (session, &in_set, &out_set) )
			// The connection failed, or the server disconnected. Handle it.
	}

	return 0;
}
