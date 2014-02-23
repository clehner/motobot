/*
 * command.h
 * Copyright (c) 2014 Charles Lehner
 */

#ifndef COMMAND_H
#define COMMAND_H

#include "module.h"
#include "bot.h"

typedef struct {
	module_t *module;
	module_t *from_module;
	const char *channel;
	const char *sender;
	module_t *command_module;
} command_env_t;

typedef void (*command_handler_t) (command_env_t env, int argc, char **argv);

struct command {
	const char *name;
	command_handler_t handler;
};

typedef struct command command_t;

int
command_exec(command_env_t env, const char *message);

void
command_respond(command_env_t env, const char *resp, ...);

module_t *
commands_new();

#endif /* COMMAND_H */

