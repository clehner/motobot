#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "bot.h"
#include "module.h"
#include "log.h"

struct log {
	module_t module;
	char post_url[256];
	char filename[256];
	FILE *file;
};

static int
log_message(log_t *log, const char *sender, const char *message) {
	if (log->file && feof(log->file)) {
		fprintf(stderr, "Log file stream is closed.\n");
		return 0;
	}

	fprintf(log->file, "%s %s\n", sender, message);
	return 1;
}

// Learn a corpus from a log file
static int
load_file(log_t *log, const char *filename) {
	char line[576];
	char sender[64];
	char *message;

	FILE *file = fopen(filename, "r");
	if (!file) {
		perror("fopen");
		return 0;
	}

	// Read the log
	while (!feof(file)) {
		// Get the line
		if (!fgets(line, sizeof(line), file)) {
			break;
		}

		// trim
		int len = strlen(line);
		if (len > 0 && line[len-1] == '\n') {
			line[len-1] = '\0';
		}

		// Split line into message and sender
		message = strchr(line, ' ');
		if (message) {
			int len = message - line;
			strncpy(sender, line, len);
			sender[len] = '\0';
			message++;
		} else {
			message = line;
			sender[0] = '\0';
		}

		// Learn the sentence
		bot_on_read_log(log->module.bot, sender, message);
	}

	fclose(file);
	return 1;
}

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	log_t *log = (log_t *)module;
	if (!sender) {
		// Ignore messages without a sender, e.g. from pipe module on stdout
		return;
	}
	log_message(log, sender, message);
}

static void
config(module_t *module, const char *name, const char *value) {
	log_t *log = (log_t *)module;
	if (strcmp(name, "file") == 0) {
		strncpy(log->filename, value, sizeof(log->filename));
	} else if (strcmp(name, "post") == 0) {
		strncpy(log->post_url, value, sizeof(log->post_url));
	}
}

static int
module_connect(module_t *module) {
	log_t *log = (log_t *)module;

	// Read log
	load_file(log, log->filename);

	// Open log file for writing
	log->file = fopen(log->filename, "a");
	if (!log->file) {
		perror("fopen");
		return 0;
	}
	return 1;
}

module_t *
log_new() {
	log_t *log = calloc(1, sizeof(log_t));
	if (!log) {
		perror("calloc");
		return NULL;
	}

	log->module.type = "log";
	log->module.config = config;
	log->module.connect = module_connect;
	log->module.on_msg = on_msg;

	return (module_t *)log;
}

