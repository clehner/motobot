/*
 * bot.h
 * Copyright (c) 2014 Charles Lehner
 */

#ifndef BOT_H
#define BOT_H

#include "module.h"

typedef struct {
	module_t *modules;
} bot_t;

void
bot_add_module(bot_t *bot, module_t* type);

int
bot_connect(bot_t *bot);

int
bot_add_select_descriptors(bot_t *bot, fd_set *in_set, fd_set *out_set, int * maxfd);

int
bot_process_select_descriptors(bot_t *bot, fd_set *in_set, fd_set *out_set);

#endif /* BOT_H */
