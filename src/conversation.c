#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "module.h"
#include "bot.h"
#include "conversation.h"
#include "chains/chains.h"

struct conversation {
	module_t module;
	struct markov_model *model;
};

// Generate a response to be sent back
static int
generate_response(conversation_t *conv, module_t *from_module,
		const char *sender, const char *message, char *response,
		size_t max_response_len) {
	size_t name_len = from_module->name ? strlen(from_module->name) : 0;

	// For us to respond to a message, it must contain our nick
	// not followed by a letter/number/_-
	char *name_start = strstr(message, from_module->name);
	if (!name_start) {
		return 0;
	}
	char name_after = name_start[name_len];
	if (name_after == '_' || name_after == '-'
			|| (name_after >= 'a' && name_after <= 'z')
			|| (name_after >= 'A' && name_after <= 'Z')
			|| (name_after >= '0' && name_after <= '9')) {
		return 0;
	}

	// Ignore commands (messages of the form "mynick: /")
	if (message == name_start && message[name_len] == ':') {
		char *after = name_start + name_len + 1;
		// Skip spaces
		while (*after == ' ') {
			after++;
		}
		if (*after == '/') {
			return 0;
		}
	}

	int tries = 0;
	do {
		tries++;
		// Generate response before learning
		if (!mm_respond_and_learn(conv->model, message, response, 0)) {
			fprintf(stderr, "Unable to respond\n");
			return 0;
		}

	// If we have an empty response, try again
	} while (response[0] == '\0' && tries < 5);

	// Transform self-addressment
	if (strncmp(from_module->name, response, name_len) == 0) {
		int msg_len = strlen(response);

		// Remove self ping
		if (response[name_len] == ':') {
			name_len++;
			// Require space or end of response
			if (response[name_len] == ' ' || response[name_len] == '\0') {
				// Skip spaces
				while (response[name_len] == ' ') {
					name_len++;
				}
				for (int i = name_len; i <= msg_len; i++) {
					response[i-name_len] = response[i];
				}
				// If we've shortened the response to nothing,
				// change it to ping the sender
				if (response[0] == '\0') {
					strncpy(response, sender, max_response_len - 2);
					strcat(response, ":");
				}
			}

		// Turn me statement into /me action
		} else if (response[name_len] == ' ') {
			static const char *me_prefix = "/me ";
			// Skip spaces
			do {
				name_len++;
			} while (response[name_len] == ' ');
			// Replace name with /me
			char tmp_response[MAX_LINE_LENGTH];
			strcpy(tmp_response, &response[name_len]);
			strcpy(response, me_prefix);
			strcat(response, tmp_response);
		}
	}

	return 1;
}

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	conversation_t *conv = (conversation_t *)module;
	char response[MAX_LINE_LENGTH];

	if (generate_response(conv, from_module, sender, message,
				(char *)&response, sizeof(response))) {
		// Send response to channel
		bot_send(module->bot, module, from_module, channel, response);
	}

	// Learn message
	mm_learn_sentence(conv->model, message);
}

static void
on_privmsg(module_t *module, module_t *from_module, const char *sender,
		const char *message) {
	conversation_t *conv = (conversation_t *)module;
	char response[MAX_LINE_LENGTH];

	// Generate response, without learning
	if (!mm_respond_and_learn(conv->model, message, response, 0)) {
		fprintf(stderr, "Unable to respond\n");
		return;
	}

	// Send PM response
	bot_send(module->bot, module, from_module, sender, response);
}

static void
on_read_log(module_t *module, const char *sender, const char *message) {
	conversation_t *conv = (conversation_t *)module;

	if (!mm_learn_sentence(conv->model, message)) {
		fprintf(stderr, "Failed to learn sentence: '%s'\n", message);
	}
}

module_t *
conversation_new() {
	conversation_t *conv = calloc(1, sizeof(conversation_t));
	if (!conv) {
		perror("calloc");
		return NULL;
	}

	conv->module.type = "conversation";
	conv->module.name = "???";
	conv->module.on_msg = on_msg;
	conv->module.on_privmsg = on_privmsg;
	conv->module.on_read_log = on_read_log;
	conv->model = mm_new();

	return (module_t *)conv;
}
