#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command.h"

void
command_help(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *args);
void
command_unknown(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *args);

static struct {
	const char *name;
	command_handler_t handler;
} command_handlers[] = {
	{"help", command_help},
	{NULL, command_unknown}
};

void
command_help(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *args) {
	const char *response = "help";
	bot_send(module->bot, module, from_module, channel, "Commands:");
	for (int i = 0; command_handlers[i].name; i++) {
		// printf("command_handlers[i].name\n", i);
		bot_send(module->bot, module, from_module, channel, command_handlers[i].name);
	}
	// bot_send(module->bot, module, from_module, channel, response);
}

void
command_unknown(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *args) {
	const char *response = "Unknown command";
	bot_send(module->bot, module, from_module, channel, response);
}

// Parse a message into handler function and argument string
int
command_parse(module_t *module, const char *command,
		command_handler_t *handler, const char **args) {
	size_t command_len;
	const char *command_args;

	// Commands are formatted 'nick: /command ...' or 'nick /command ...'
	// We've already read the 'nick: ' part

	// Skip '/'
	if (command[0] == '/') {
		command++;
	} else {
		return 0;
	}

	// Split command name and args
	command_args = strchr(command, ' ');
	if (command_args) {
		command_len = command_args - command;
		// skip space
		while (command_args[0] == ' ') {
			command_args++;
		}
	} else {
		// no arguments, only command name
		command_len = strlen(command);
		command_args = NULL;
	}
	*args = command_args;

	// Find the command handler.
	// The last command in the handlers array should be a catch-all
	// and have null name.
	int i = 0;
	while (command_handlers[i].name &&
			strncmp(command, command_handlers[i].name, command_len) != 0) i++;
	*handler = command_handlers[i].handler;
	printf("i=%d, name=%s\n", i, command_handlers[i].name);
	return 1;
}

