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
	bool leaderboard_enabled;
};

// karma leaderboard unit
typedef struct {
	const char *id;
	ssize_t amount;
} vote_t;

typedef void
(*on_vote_fn_t) (command_env_t *env, vote_t vote, ssize_t delta);

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
process_message(karma_t *karma, const char *message, command_env_t *env,
		on_vote_fn_t on_vote) {
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
					id_copy = (char *)malloc((id_len+1) * sizeof(char));
					if (!id_copy) {
						perror("malloc");
						return;
					}
					strncpy(id_copy, id, id_len+1);
				}
				ssize_t total = votes + (ssize_t)hash_get(table, id);
				hash_set(table, id_copy, (void *)total);
				// Pass the vote callback to the parent
				if (on_vote && env) {
					on_vote(env, (vote_t){id, total}, votes);
				}
			}
		}
	}
}

static void
got_vote(command_env_t *env, vote_t vote, ssize_t delta) {
	// Repond about a thing that was up/downvoted
	char response[512];
	const char *direction =
		(delta < 0) ? "down" :
		(delta > 0) ? "up" : "side";
	snprintf(response, sizeof(response), "%s %svoted %s (karma: %zd)", env->sender,
			direction, vote.id, vote.amount);
	bot_send(env->module->bot, env->module, env->from_module, env->channel,
			response);
}

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	karma_t *karma = (karma_t *)module;

	// Process votes in the message
	command_env_t env = {module, from_module, channel, sender, module};
	process_message(karma, message, &env, got_vote);
}

static void
on_read_log(module_t *module, const char *sender, const char *message) {
	karma_t *karma = (karma_t *)module;
	process_message(karma, message, NULL, NULL);
}

// Get top n ids + amounts
int
ksort(karma_t *karma, vote_t *votes, int n, bool reverse) {
	int actual_n = 0;
	int sign = reverse ? -1 : 1;
	hash_each(karma->karma, {
		ssize_t amount = (ssize_t)val;
		const char *id = (const char *)key;
		if (actual_n == 0) {
			actual_n++;
			votes[0].amount = amount;
			votes[0].id = id;
		} else if (amount * sign > votes[actual_n-1].amount * sign) {
			// Find the index to place this vote at
			for (int i = 0; i < actual_n; i++) {
				if (amount * sign > votes[i].amount * sign) {
					// Move the following votes down the list
					for (int j = n-1; j > i; j--) {
						votes[j] = votes[j-1];
					}
					// Insert our vote
					votes[i].amount = amount;
					votes[i].id = id;
					if (actual_n < n) {
						actual_n++;
					}
					break;
				}
			}
		}
		// printf("%s: %zd\n", id, amount);
	});
	return actual_n;
}

static void
karma_listing(karma_t *karma, command_env_t env) {
	static const int top_n = 10;
	static const int bottom_n = 5;
	vote_t top_ids[top_n];
	int actual_n = ksort(karma, top_ids, top_n, false);
	for (int i = 0; i < actual_n; i++) {
		vote_t vote = top_ids[i];
		if (vote.id) {
			command_respond(env, "%s: %zd", vote.id, vote.amount);
		} else {
			break;
		}
	}
	command_respond(env, "...");

	actual_n = ksort(karma, top_ids, bottom_n, true);
	for (int i = actual_n-1; i >= 0; i--) {
		vote_t vote = top_ids[i];
		if (vote.id) {
			command_respond(env, "%s: %zd", vote.id, vote.amount);
		} else {
			break;
		}
	}
}

void
command_karma(command_env_t env, int argc, char **argv) {
	karma_t *karma = (karma_t *)env.command_module;
	if (argc < 2) {
		if (karma->leaderboard_enabled) {
			karma_listing(karma, env);
		} else {
			command_respond(env, "Usage: %s thing", argv[0]);
		}
		return;
	}

	char *id = argv[1];
	// Cast void* to signed int using ssize_t
	ssize_t amount = (ssize_t)hash_get(karma->karma, id);
	command_respond(env, "%s: %d", id, amount);
}

static void
config(module_t *module, const char *name, const char *value) {
	karma_t *karma = (karma_t *)module;
	if (strcmp(name, "leaderboard") == 0) {
		if (strcmp(value, "yes") == 0) {
			karma->leaderboard_enabled = true;
		} else if (strcmp(value, "no") == 0) {
			karma->leaderboard_enabled = false;
		} else {
			fprintf(stderr, "leaderboard option should be yes or no)\n");
		}
	}
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
	karma->module.config = config;
	karma->module.on_msg = on_msg;
	karma->module.on_read_log = on_read_log;
	karma->module.commands = commands;
	karma->karma = hash_new();
	karma->leaderboard_enabled = 0;

	return (module_t *)karma;
}

