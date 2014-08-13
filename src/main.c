#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "inih/ini.h"
#include "bot.h"
#include "module.h"
#include "strdup.h"

static bot_t bot = {0};

static int
bot_config_handler(void* obj, const char* section, const char* name,
		const char* value) {
	bot_open_section(&bot, section);
	return bot_config_set(&bot, section, name, value);
}

int
main(int argc, char *argv[]) {
	struct timeval tv;
	fd_set in_set, out_set;
	int maxfd = 0;

	bot_init(&bot);
	bot.config_filename = argv[1];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s config.ini\n", argv[0]);
        return 1;
    }

	// Read and process the configuration
    if (ini_parse(bot.config_filename, bot_config_handler, 0) < 0) {
        fprintf(stderr, "Can't load '%s'\n", argv[1]);
        return 1;
    }

	// Start connecting stuff
	bot_connect(&bot);

	while (1) {

		// Initialize the sets
		FD_ZERO (&in_set);
		FD_ZERO (&out_set);

		// Wait 1 sec for the events.
		// ircclient has internal timeouts.
		tv.tv_usec = 0;
		tv.tv_sec = 1;

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
