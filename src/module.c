#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "module.h"
#include "irc.h"
#include "conversation.h"
#include "pipe.h"

module_t *
module_new(const char *type) {
	if (strcmp(type, "irc") == 0) {
		return (module_t *)irc_new();
	} else if (strcmp(type, "conversation") == 0) {
		return (module_t *)conversation_new();
	} else if (strcmp(type, "pipe") == 0) {
		return pipe_new();
	} else {
		return NULL;
	}
}

void
module_config(module_t *module, const char *name, const char *value) {
	if (module->config) (*module->config)(module, name, value);
}
