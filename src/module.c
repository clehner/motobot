#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "module.h"
#include "irc.h"
#include "conversation.h"
#include "pipe.h"
#include "log.h"
#include "karma.h"
#include "pinger.h"
#include "command.h"
#include "config.h"
#include "todo.h"

module_t *
module_new(const char *type) {
	module_t *module;
	if (strcmp(type, "irc") == 0) {
		module = irc_new();
	} else if (strcmp(type, "conversation") == 0) {
		module = conversation_new();
	} else if (strcmp(type, "pipe") == 0) {
		module = pipe_new();
	} else if (strcmp(type, "log") == 0) {
		module = log_new();
	} else if (strcmp(type, "karma") == 0) {
		module = karma_new();
	} else if (strcmp(type, "commands") == 0) {
		module = commands_new();
	} else if (strcmp(type, "pinger") == 0) {
		module = pinger_new();
	} else if (strcmp(type, "config") == 0) {
		module = config_new();
	} else if (strcmp(type, "todo") == 0) {
		module = todo_new();
	} else {
		return NULL;
	}

	module->config_values = hash_new();
	return module;
}
