#include <stdlib.h>
#include <stdio.h>
#include "module.h"
#include "bot.h"
#include "conversation.h"
#include "chains/chains.h"

typedef struct conversation conversation_t;

struct conversation {
	module_t module;
	struct markov_model *model;
};

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	conversation_t *conv = (conversation_t *)module;
	char response[MAX_LINE_LENGTH];

	// Respond before learning
	if (!mm_respond_and_learn(conv->model, message, response, 0)) {
		fprintf(stderr, "Unable to respond\n");
		return;
	}
	// Learn message
	mm_learn_sentence(conv->model, message);

	// Send response
	bot_send(module->bot, module, from_module, channel, response);
	// (*from_module->send)(from_module, channel, response);

	// Learn and log response if it is in a channel
	/*
	if (channel) {
		printf("Learning from %s\n", from_module->name);
		bot_on_read_log(module->bot, from_module->name, response);
	}
	*/
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
	conv->module.on_read_log = on_read_log;
	conv->model = mm_new();

	return (module_t *)conv;
}
