#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "bot.h"
#include "module.h"
#include "hash/hash.h"
#include "strdup.h"

void
bot_init(bot_t *bot) {
	bot->modules_by_section = hash_new();
}

// Get the module corresponding to a section in the config
module_t *
bot_get_module(bot_t *bot, const char *section) {
	return (module_t *)hash_get(bot->modules_by_section, (char *)section);
}

const char *
bot_config_get(bot_t *bot, const char* section, const char* name) {
	module_t *module = bot_get_module(bot, section);
	if (!module) return NULL;
	return hash_get(module->config_values, (char *)name);
}

int
bot_config_set(bot_t *bot, const char *section, const char* name,
		const char* value) {
	module_t *module;
	char module_type[64];
	char *delimiter;

	module = bot_get_module(bot, section);
	if (module == NULL) {
		// No matching module found.
		// Create the module.
		// Strip characters after dot in section name, to allow for multiple
		// sections of the same type.
		strncpy(module_type, section, sizeof module_type);
		if ((delimiter = strchr(module_type, '.'))) {
			*delimiter = '\0';
		}
		module = module_new(module_type);
		section = strdup(section);
		hash_set(bot->modules_by_section, (char *)section, (void *)module);
		bot_open_section(bot, section);
		if (!module) {
			fprintf(stderr, "Unable to create module '%s'\n", module_type);
			return 0;
		}
	}

	// duplicate the strings so they can persist in the hashtable
	name = strdup(name);
	value = strdup(value);
	// TODO: free previous names and values

	// Set the value in the config hashtable
	hash_set(module->config_values, (char *)name, (void *)value);

	// Allow disabling/re-enabling modules
	if (!strcmp("enabled", name)) {
		if (!strcmp("yes", value)) {
			// Add the module
			bot_add_module(bot, module);
		} else if (!strcmp("no", value)) {
			bot_remove_module(bot, module);
			// TODO: free module and completely remove it
		}
	}

	// Configure the module
	//printf("config (%p): %s=%s\n", module->config, name, value);
	if (module->config)
		(*module->config)(module, name, value);
	return 1;
}

// Set the given section as the current last section of the config file.
int
bot_open_section(bot_t *bot, const char *section) {
	if (bot->last_section && strcmp(bot->last_section, section) == 0) {
		// section already open
		return 0;
	}
	if (bot->last_section) {
		free(bot->last_section);
	}
	// open section
	bot->last_section = strdup(section);
	return 1;
}

// Write a config value to the config file.
// Simply append it to the end of the file, because we are lazy.
void
bot_config_append(bot_t *bot, const char *section, const char *key,
		const char *value) {
	FILE *file = fopen(bot->config_filename, "a");
	if (!file) {
		perror("fopen");
		return;
	}

	if (bot_open_section(bot, section)) {
		// Establish the desired section in the config file
		if (fprintf(file, "\n[%s]\n", section) < 0){
			perror("fprintf");
		}
	}

	if (fprintf(file, "%s=%s\n", key, value) < 0) {
		perror("fprintf");
	}

	fclose(file);
}

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
bot_on_privmsg(bot_t *bot, module_t *from_module, const char *sender,
		const char *message) {
	for (module_t *module = bot->modules; module; module = module->next) {
		if (module->on_privmsg)
			(*module->on_privmsg)(module, from_module, sender, message);
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
		} else if (from_module->name) {
			if (module->on_msg)
				(*module->on_msg)(module, from_module, channel,
						to_module->name, message);
		}
	}
}
