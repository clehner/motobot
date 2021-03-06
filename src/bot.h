/*
 * bot.h
 * Copyright (c) 2014 Charles Lehner
 */

#ifndef BOT_H
#define BOT_H

#include "module.h"
#include "hash/hash.h"

typedef struct bot bot_t;

struct bot {
	module_t *modules;
	hash_t *modules_by_section;
	const char *config_filename;
	char *last_section;
};

void
bot_init(bot_t *bot);

module_t *
bot_get_module(bot_t *bot, const char *section);

const char *
bot_config_get(bot_t *bot, const char *section, const char *name);

int
bot_config_set(bot_t *bot, const char *section, const char *name,
		const char *value);

int
bot_open_section(bot_t *bot, const char *section);

void
bot_config_append(bot_t *bot, const char *section, const char *key,
		const char *value);

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

void
bot_on_privmsg(bot_t *bot, module_t *from_module, const char *sender,
		const char *message);

void
bot_on_read_log(bot_t *bot, const char *sender, const char *message);

void
bot_send(bot_t *bot, module_t *from_module, module_t *to_module,
		const char *channel, const char *message);

#endif /* BOT_H */
