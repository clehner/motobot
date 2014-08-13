#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "bot.h"
#include "config.h"
#include "command.h"
#include "strdup.h"
#include "hash/hash.h"

struct config {
	module_t module;
};

// Get a string listing the sections in the bot's config
const char *
sprint_sections(bot_t *bot) {
	static char buf[512];
	size_t max_len = 512;
	buf[0] = '\0';
	hash_each_key(bot->modules_by_section, {
		strncat(buf, key, max_len -= strlen(key));
		strncat(buf, ", ", max_len -= 2);
	});
	// Trim last comma
	size_t len = strlen(buf);
	if (len >= 2) {
		buf[len-2] = '\0';
	}
	return buf;
}

// Get a string listing the keys in the given section of the bot's config
const char *
sprint_keys(bot_t *bot, const char *section) {
	static char buf[512];
	size_t max_len = 512;
	module_t *module = bot_get_module(bot, section);
	buf[0] = '\0';
	hash_each_key(module->config_values, {
		strncat(buf, key, max_len -= strlen(key));
		strncat(buf, ", ", max_len -= 2);
	});
	// Trim last comma
	size_t len = strlen(buf);
	if (len >= 2) {
		buf[len-2] = '\0';
	}
	return buf;
}


// get a config value, except for some values
const char *
config_get(bot_t *bot, const char* section, const char* name) {
	// Hide password
	// TODO: be more paradigmatic about this
	if (strncmp("irc", section, 3) == 0 && strcmp("password", name) == 0) {
		return "********";
	}

	return bot_config_get(bot, section, name);
}

const char *
config_can_set(bot_t *bot, const char *section, const char* name,
		const char* value) {
	// Prevent disabling config interface from config interface
	if (!strcmp("enabled", name) && !strcmp("no", value) &&
			(!strncmp("irc", section, 3) ||
			!strncmp("config", section, 6) ||
			!strncmp("commands", section, 8))) {
		return "I don't want to do that.";
	}

	return NULL;
}

void
command_config(command_env_t env, int argc, char **argv) {
	bot_t *bot = env.command_module->bot;
	if (argc < 1) {
		command_respond(env, "Usage: /config [section [key [value]]]");
	} else if (argc == 1) {
		// Get list of currently used config sections
		command_respond(env, "Config sections: %s", sprint_sections(bot));
	} else if (argc == 2) {
		// Get list of keys in a config section
		command_respond(env, "Config keys in %s: %s", argv[1],
			sprint_keys(bot, argv[1]));
	} else if (argc == 3) {
		// Get value for a key in a config section
		command_respond(env, "Config value for %s %s: %s", argv[1], argv[2],
			config_get(bot, argv[1], argv[2]));
	} else if (argc >= 4) {
		// Set value for a key in a config section

		const char *error_msg = config_can_set(bot, argv[1], argv[2], argv[3]);
		if (error_msg) {
			command_respond(env, error_msg);
			return;
		}

		// Set the value
		bot_config_set(bot, argv[1], argv[2], argv[3]);
		// Write the value to the file
		bot_config_append(bot, argv[1], argv[2], argv[3]);
		command_respond(env, "Config value for %s %s set to %s",
			argv[1], argv[2], argv[3]);
	}
}

static command_t commands[] = {
	{"/config", command_config},
	{NULL}
};

module_t *
config_new() {
	config_t *config = calloc(1, sizeof(config_t));
	if (!config) {
		perror("calloc");
		return NULL;
	}

	config->module.type = "config";
	config->module.name = "config";
	config->module.commands = commands;

	return (module_t *)config;
}
