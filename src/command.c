#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "command.h"

void
command_help(command_env_t env, int argc, char **argv) {
	char resp[256];
	resp[0] = '\0';
	// Iterate through modules
	bot_t *bot = env.module->bot;
	for (module_t *module = bot->modules; module; module = module->next) {
		// Iterate through each module's commands
		for (command_t *cmd = module->commands; cmd && cmd->name; cmd++) {
			strcat(resp, cmd->name);
			strcat(resp, ", ");
		}
	}
	// Trim last comma
	resp[strlen(resp)-2] = '\0';
	command_respond(env, "Commands: %s", resp);
}

void
command_unknown(command_env_t env, int argc, char **argv) {
	command_respond(env, "Unknown command %s", argv[0]);
}

static command_t default_commands[] = {
	{"/help", command_help},
	{NULL}
};

// Find a command of a given name registered to a bot
command_handler_t
command_find(command_env_t *env, const char *command) {
	bot_t *bot = env->module->bot;

	// Iterate through modules
	for (module_t *module = bot->modules; module; module = module->next) {
		// Iterate through each module's commands
		for (command_t *cmd = module->commands; cmd && cmd->name; cmd++) {
			if (strcmp(command, cmd->name) == 0) {
				env->command_module = module;
				return cmd->handler;
			}
		}
	}
	return command_unknown;
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
		command_argv[command_argc++] = arg;
	}
	command = command_argv[0];

	// Find the command handler.
	handler = command_find(&env, command);
	// Execute the command
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

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	size_t name_len = from_module->name ? strlen(from_module->name) : 0;

	// Message must start with our nick for us to consider it
	if (strncmp(from_module->name, message, name_len)) {
		return;
	}
	// Skip the initial nick prefix
	message += name_len;

	// Read ':' so that message must start with 'nick:'
	if (message[0] != ':') {
		return;
	}
	message++;
	// Skip spaces
	while(message[0] == ' ') {
		message++;
	}

	// Is this a command?
	if (message[0] == '/') {
		command_exec((command_env_t) {module, from_module, channel, sender},
				message);
	}
}

/*
static void
on_privmsg(module_t *module, module_t *from_module, const char *sender,
		const char *message) {

	// Is this a command?
	if (message[0] == '/') {
		command_exec((command_env_t) {module, from_module, sender, sender},
				message);
	}
}
*/

module_t *
commands_new() {
	module_t *module = calloc(1, sizeof(module_t));
	if (!module) {
		perror("calloc");
		return NULL;
	}

	module->name = "cmd";
	module->type = "commands";
	module->on_msg = on_msg;
	// module->on_privmsg = on_privmsg;
	module->commands = default_commands;

	return module;
}
