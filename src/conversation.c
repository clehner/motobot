#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "module.h"
#include "bot.h"
#include "conversation.h"
#include "chains/chains.h"
#include "command.h"

struct conversation {
	module_t module;
	struct markov_model *model;
};

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	conversation_t *conv = (conversation_t *)module;
	char response[MAX_LINE_LENGTH];
	size_t name_len = strlen(from_module->name);

	// Message must start with our nick for us to consider it
	if (strncmp(from_module->name, message, name_len)) {
		return;
	}
	// Skip the initial nick prefix
	message += name_len;

	// Read ':' so that message must start with 'nick:'
	if (message[0] != ':') {
		return;
	}
	message++;
	// Skip spaces
	while(message[0] == ' ') {
		message++;
	}

	// Is this a command?
	if (message[0] == '/') {
		command_exec((command_env_t) {module, from_module, channel, sender},
				message);
		return;
	}

	// Generate response before learning
	if (!mm_respond_and_learn(conv->model, message, response, 0)) {
		fprintf(stderr, "Unable to respond\n");
		return;
	}

	// Learn message
	mm_learn_sentence(conv->model, message);

	// Send response to channel
	bot_send(module->bot, module, from_module, channel, response);
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
	conv->module.on_msg = on_msg;
	conv->module.on_privmsg = on_privmsg;
	conv->module.on_read_log = on_read_log;
	conv->model = mm_new();

	return (module_t *)conv;
}
