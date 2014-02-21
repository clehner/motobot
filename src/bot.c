#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "bot.h"
#include "module.h"

void
bot_module_foreach(bot_t *bot);

void
bot_add_module(bot_t *bot, module_t *module) {
	module->next = bot->modules;
	bot->modules = module;
	module->bot = bot;
}

void
bot_remove_module(bot_t *bot, module_t *module) {
	if (bot->modules == module) {
		bot->modules = module->next;
	} else for (module_t *prev = bot->modules; prev; prev = prev->next) {
		if (prev->next == module) {
			prev->next = module->next;
			break;
		}
	}
	module->bot = NULL;
}

int
bot_connect(bot_t *bot) {
	for (module_t *module = bot->modules; module; module = module->next) {
		if (module->connect)
			(*module->connect)(module);
	}
	return 1;
}

int
bot_add_select_descriptors(bot_t *bot, fd_set *in_set, fd_set *out_set,
		int *maxfd) {
	for (module_t *module = bot->modules; module; module = module->next) {
		if (module->add_select_descriptors)
			(*module->add_select_descriptors)(module, in_set, out_set, maxfd);
	}
	return 1;
}

int
bot_process_select_descriptors(bot_t *bot, fd_set *in_set, fd_set *out_set) {
	for (module_t *module = bot->modules; module; module = module->next) {
		if (module->process_select_descriptors)
			(*module->process_select_descriptors)(module, in_set, out_set);
	}
	return 1;
}

void
bot_on_msg(bot_t *bot, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	for (module_t *module = bot->modules; module; module = module->next) {
		if (module->on_msg)
			(*module->on_msg)(module, from_module, channel, sender, message);
	}
}

void
bot_on_read_log(bot_t *bot, const char *sender, const char *message) {
	for (module_t *module = bot->modules; module; module = module->next) {
		if (module->on_read_log)
			(*module->on_read_log)(module, sender, message);
	}
}

void
bot_send(bot_t *bot, module_t *from_module, module_t *to_module,
		const char *channel, const char *message) {
	for (module_t *module = bot->modules; module; module = module->next) {
		if (module == from_module) {
		} else if (module == to_module) {
			if (module->send)
				(*module->send)(module, channel, message);
		} else if (module->on_msg) {
			if (module->on_msg)
				(*module->on_msg)(module, from_module, channel,
						to_module->name, message);
		}
	}
}
