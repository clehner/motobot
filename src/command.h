/*
 * command.h
 * Copyright (c) 2014 Charles Lehner
 */

#ifndef COMMAND_H
#define COMMAND_H

#include "module.h"
#include "bot.h"

typedef void (*command_handler_t) (module_t *module, module_t *from_module,
		const char *channel, const char *sender, const char *args);

int
command_parse(module_t *module, const char *command,
		command_handler_t *handler, const char **args);

#endif /* COMMAND_H */

