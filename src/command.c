#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "command.h"

void
command_help(command_env_t env, int argc, char **argv);
void
command_unknown(command_env_t env, int argc, char **argv);

typedef struct {
	const char *name;
	command_handler_t handler;
} named_handler_t;

named_handler_t command_handlers[] = {
	{"/help", command_help},
	{NULL, command_unknown}
};

void
command_help(command_env_t env, int argc, char **argv) {
	char resp[256];
	resp[0] = '\0';
	for (int i = 0; command_handlers[i].name; i++) {
		if (i > 0) strcat(resp, ", ");
		strcat(resp, command_handlers[i].name);
	}
	command_respond(env, "Commands: %s", resp);
}

void
command_unknown(command_env_t env, int argc, char **argv) {
	command_respond(env, "Unknown command %s", argv[0]);
}

// Parse a message into handler function and argument string,
// and execute the handler function with the arguments as an array
int
command_exec(command_env_t env, const char *message) {
	const char *command;
	command_handler_t handler;
	int command_argc = 0;
	char *command_argv[128];
	char command_buf[256];

	// Commands are formatted 'nick: /command ...' or 'nick /command ...'
	// We've already read the 'nick: ' part

	// Split on spaces to get the command name and args
	strncpy(command_buf, message, sizeof command_buf);
	for (char *arg = strtok(command_buf, " "); arg; arg = strtok(NULL, " ")) {
		// Add the current word
		printf("arg: %s\n", arg);
		command_argv[command_argc++] = arg;
	}
	command = command_argv[0];

	// Find the command handler.
	// The last command in the handlers array should be a catch-all
	// and have null name.
	int i = 0;
	while (command_handlers[i].name &&
			strcmp(command, command_handlers[i].name) != 0) i++;
	handler = command_handlers[i].handler;
	handler(env, command_argc, command_argv);
	return 1;
}

void
command_respond(command_env_t env, const char *fmt, ...) {
	char buf[256];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);
	bot_send(env.module->bot, env.module, env.from_module, env.channel, buf);
}

