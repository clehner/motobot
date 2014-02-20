#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "bot.h"
#include "module.h"

void
bot_module_foreach(bot_t *bot);

void
bot_add_module(bot_t *bot, module_t* module) {
	module->next = bot->modules;
	bot->modules = module;
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
