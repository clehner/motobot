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
} command_env_t;

typedef void (*command_handler_t) (command_env_t env, int argc, char **argv);

int
command_exec(command_env_t env, const char *message);

void
command_respond(command_env_t env, const char *resp);

#endif /* COMMAND_H */

