#include <stdlib.h>
#include <stdio.h>
#include "module.h"
#include "bot.h"
#include "conversation.h"
#include "chains/chains.h"

struct conversation {
	module_t module;
	struct markov_model *model;
};

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	(void) sender;
	conversation_t *conv = (conversation_t *)module;
	char response[MAX_LINE_LENGTH];

	if (!mm_respond_and_learn(conv->model, message, response, 1)) {
		fprintf(stderr, "Unable to respond\n");
		return;
	}

	// Send response
	(*from_module->send)(from_module, channel, response);
}

conversation_t *
conversation_new() {
	conversation_t *conv = calloc(1, sizeof(conversation_t));
	if (!conv) {
		perror("calloc");
		return NULL;
	}

	conv->module.type = "conversation";
	conv->module.on_msg = on_msg;
	conv->model = mm_new();

	return conv;
}
