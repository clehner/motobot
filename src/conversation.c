#include <stdlib.h>
#include <stdio.h>
#include "module.h"
#include "bot.h"
#include "conversation.h"

struct conversation {
	module_t module;
};

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	conversation_t *conv = (conversation_t *)module;
	// echo
	(*from_module->send)(from_module, channel, message);
	// printf("Got message on %s [%s] %s: %s\n",
			// from_module->type, channel, sender, message);
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

	return conv;
}
