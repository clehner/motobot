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
#define R_ID "^\\([^\\(++\\)\\(--\\)]\\+\\)[\\(++\\)\\(--\\)]\\{2,\\}"
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
	char *id_copy;
	char errbuf[128];
	regmatch_t matches[3];
	int error;
	// We use ssize_t for votes because it is signed and the size of a void*,
	// which hash.h expects as a value
	ssize_t votes;

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
			size_t id_len = matches[1].rm_eo - matches[1].rm_so;
			// Get the token name
			strncpy(id, word + matches[1].rm_so, id_len);
			id[id_len] = '\0';
			votes = 0;

			// Count the ++
			if (!regexec(&r_plus, word, 1, matches, 0)) {
				votes += (matches[0].rm_eo - matches[0].rm_so) / 2;
			}

			// Count the --
			if (!regexec(&r_minus, word, 1, matches, 0)) {
				votes -= (matches[0].rm_eo - matches[0].rm_so) / 2;
			}

			// Update the hashtable with the count
			if (votes) {
				// if (hash_has(table, id)) {
				// Warning: memory leak here.
				// Because hash_has is not working, we use hash_get to test.
				// That means if a karma goes to 0, we think it's not in the
				// table, and reallocate the string key for it

				if (hash_get(table, id)) {
					// The table already has this id in it
					id_copy = id;
				} else {
					// Allocate a string for this id to put it into the table
					id_copy = (char *)malloc(id_len * sizeof(char));
					if (!id_copy) {
						perror("malloc");
						return;
					}
					strncpy(id_copy, id, id_len);
				}
				votes += (ssize_t)hash_get(table, id);
				hash_set(table, id_copy, (void *)votes);
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

		/*
		hash_each(karma->karma, {
			printf("%s: %zd\n", (char *)key, (ssize_t)val);
		});
		*/
		return;
	}

	char *id = argv[1];
	// Cast void* to signed int using ssize_t
	ssize_t amount = (ssize_t)hash_get(karma->karma, id);
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

