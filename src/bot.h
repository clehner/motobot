/*
 * bot.h
 * Copyright (c) 2014 Charles Lehner
 */

#ifndef BOT_H
#define BOT_H

#include "module.h"

typedef struct bot bot_t;

struct bot {
	module_t *modules;
};

void
bot_add_module(bot_t *bot, module_t* module);

void
bot_remove_module(bot_t *bot, module_t* module);

int
bot_connect(bot_t *bot);

int
bot_add_select_descriptors(bot_t *bot, fd_set *in_set, fd_set *out_set, int * maxfd);

int
bot_process_select_descriptors(bot_t *bot, fd_set *in_set, fd_set *out_set);

void
bot_on_msg(bot_t *bot, module_t *from_module, const char *channel,
		const char *sender, const char *message);

#endif /* BOT_H */
