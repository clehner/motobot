#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "bot.h"
#include "module.h"
#include "hash/hash.h"
#include "karma.h"
#include "regex.h"
#include "command.h"

struct karma {
	module_t module;
	hash_t *karma;
};

#define DELIMETERS " \n\r\t"
// no non-greedy star in POSIX regex
#define R_ID "^\\([^\\(++\\)\\(--\\)]\\+\\?\\)[\\(++\\)\\(--\\)]"
#define R_PLUS "\\(++\\)\\+"
#define R_MINUS "\\(--\\)\\+"

regex_t r_id, r_plus, r_minus;
bool regexes_compiled = false;

static int
compile_regexes() {
	if (regexes_compiled) return 1;
	regexes_compiled = true;
	if (regcomp(&r_id, R_ID, 0) ||
			regcomp(&r_plus, R_PLUS, 0) ||
			regcomp(&r_minus, R_MINUS, 0)) {
		fprintf(stderr, "Failed to compile regexes\n");
		return 0;
	}
	return 1;
}

// Scan a message for ++/-- and store the results in the karma hashtable
static void
process_message(karma_t *karma, const char *message) {
	char message_copy[512];
	char id[64];
	char errbuf[128];
	regmatch_t matches[3];
	int votes;
	int error;

	hash_t *table = karma->karma;
	strncpy(message_copy, message, sizeof message_copy);

	// Split the message into words
	for (const char *word = strtok(message_copy, DELIMETERS);
			word;
			word = strtok(NULL, DELIMETERS)) {

		// Match the word against the regexes
		error = regexec(&r_id, word, 2, matches, 0);
		if (error == REG_NOMATCH) {
			// printf("No match for %s\n", word);
		} else if (error) {
			regerror(error, &r_id, errbuf, sizeof errbuf);
			fprintf(stderr, "regexec: %s\n", errbuf);
		} else {
			// Get the token name
			strncpy(id, word + matches[1].rm_so, matches[1].rm_eo);
			id[matches[1].rm_eo] = '\0';
			votes = 0;

			// Count the ++
			if (!regexec(&r_plus, word, 1, matches, 0)) {
				votes += (matches[0].rm_eo - matches[0].rm_so) / 2;
			}

			// Count the --
			if (!regexec(&r_minus, word, 1, matches, 0)) {
				votes -= (matches[0].rm_eo - matches[0].rm_so) / 2;
			}

			// printf("Matched %s [%s]: %d\n", word, id, votes);

			// Update the hashtable with the count
			if (votes) {
				hash_set(table, id, hash_get(table, id) + votes);
			}
		}
	}
}

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	karma_t *karma = (karma_t *)module;
	process_message(karma, message);
}

static void
on_read_log(module_t *module, const char *sender, const char *message) {
	karma_t *karma = (karma_t *)module;
	process_message(karma, message);
}

void
command_karma(command_env_t env, int argc, char **argv) {
	karma_t *karma = (karma_t *)env.command_module;
	if (argc < 2) {
		// TODO: Give a listing of top karma
		command_respond(env, "Usage: %s thing", argv[0]);
		return;
	}

	char *id = argv[1];
	// Cast void* to signed int using ssize_t
	int amount = (ssize_t)hash_get(karma->karma, id);
	command_respond(env, "%s: %d", id, amount);
}

static command_t commands[] = {
	{"/karma", command_karma},
	{NULL}
};

module_t *
karma_new() {
	karma_t *karma = calloc(1, sizeof(karma_t));
	if (!karma) {
		perror("calloc");
		return NULL;
	}

	compile_regexes();

	karma->module.type = "karma";
	karma->module.on_msg = on_msg;
	karma->module.on_read_log = on_read_log;
	karma->module.commands = commands;
	karma->karma = hash_new();

	return (module_t *)karma;
}

