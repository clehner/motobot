#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "module.h"
#include "irc.h"
#include "conversation.h"
#include "pipe.h"
#include "log.h"
#include "karma.h"
#include "command.h"

module_t *
module_new(const char *type) {
	if (strcmp(type, "irc") == 0) {
		return irc_new();
	} else if (strcmp(type, "conversation") == 0) {
		return conversation_new();
	} else if (strcmp(type, "pipe") == 0) {
		return pipe_new();
	} else if (strcmp(type, "log") == 0) {
		return log_new();
	} else if (strcmp(type, "karma") == 0) {
		return karma_new();
	} else if (strcmp(type, "commands") == 0) {
		return commands_new();
	} else {
		return NULL;
	}
}
