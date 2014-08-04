#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <regex.h>
#include "chains/chains.h"
#include "command.h"
#include "pinger.h"

struct ping {
	struct ping *next;
	const char *channel;
	const char *from;
	const char *to;
	time_t time;
};

struct pinger {
	module_t module;
	struct ping pings; // pings in progress (head node is ignored)
};

#define R_PING "^\\(.*\\): p\\(ing$\\|ong\\)"
#define MAX_TIME_STR_LEN 32

static regex_t r_ping;
static bool regexes_compiled = false;

static int
compile_regexes() {
	if (regexes_compiled) return 1;
	regexes_compiled = true;
	if (regcomp(&r_ping, R_PING, REG_ICASE)) {
		fprintf(stderr, "Failed to compile regexes\n");
		return 0;
	}
	return 1;
}

void
ping_free(struct ping *ping) {
	free((void *)ping->channel);
	free((void *)ping->from);
	free((void *)ping->to);
	free(ping);
}

// check whether a ping is the same from/to/channel (not time)
bool
ping_is_same(struct ping *ping1, struct ping *ping2) {
	return !strcmp(ping1->channel, ping2->channel) &&
		!strcmp(ping1->from, ping2->from) &&
		!strcmp(ping1->to, ping2->to);
}

struct ping *
pinger_lookup_ping(pinger_t *pinger, struct ping *ping_find) {
	if (!ping_find) return NULL;
	for (struct ping *ping = pinger->pings.next; ping; ping = ping->next) {
		if (ping_is_same(ping_find, ping)) {
			return ping;
		}
	}
	return NULL;
}

void
pinger_store_ping(pinger_t *pinger, struct ping *ping) {
	size_t len;
	struct ping *ping_copy;

	if (!ping) {
		fprintf(stderr, "attempted to store empty ping");
		return;
	}

	ping_copy = (struct ping*) malloc(sizeof(struct ping));
	ping_copy->next = pinger->pings.next;
	pinger->pings.next = ping_copy;

	// copy data to dynamically allocated ping struct and stuff
	len = strlen(ping->from)+1;
	ping_copy->from = (const char *) malloc(len * sizeof(char));
	strncpy((char *)ping_copy->from, ping->from, len);

	len = strlen(ping->to)+1;
	ping_copy->to = (const char *) malloc(len * sizeof(char));
	strncpy((char *)ping_copy->to, ping->to, len);

	len = strlen(ping->channel)+1;
	ping_copy->channel = (const char *) malloc(len * sizeof(char));
	strncpy((char *)ping_copy->channel, ping->channel, len);

	ping_copy->time = ping->time;
}

void
pinger_remove_ping(pinger_t *pinger, struct ping *ping_find) {
	for (struct ping *ping = &pinger->pings; ping; ping = ping->next) {
		if (ping->next == ping_find) {
			ping->next = ping_find->next;
			return;
		}
	}
	fprintf(stderr, "Unable to remove ping\n");
}

// build a string representing a time duration. string is static.
const char *
duration_format(double duration) {
	static char time_str[MAX_TIME_STR_LEN];

	unsigned int secs = duration;
	if (secs < 60) {
		snprintf(time_str, MAX_TIME_STR_LEN, "%us", secs);
	} else {
		unsigned int mins = secs / 60;
		secs %= 60;

		if (mins < 60) {
			snprintf(time_str, MAX_TIME_STR_LEN, "%um %us", mins, secs);
		} else {
			unsigned int hours = mins / 60;
			mins %= 60;

			if (hours < 24) {
				snprintf(time_str, MAX_TIME_STR_LEN, "%uh %um %us",
					hours, mins, secs);
			} else {
				unsigned int days = hours / 24;
				hours %= 24;
				snprintf(time_str, MAX_TIME_STR_LEN, "%ud %uh %um %us",
					days, hours, mins, secs);
			}
		}
	}

	return time_str;
}

void
respond_ping(pinger_t *pinger, module_t *from_module, struct ping *ping,
		double duration) {
	char response[MAX_LINE_LENGTH];

	snprintf(response, sizeof(response), "Ping time (%s -> %s): %s",
		ping->from, ping->to, duration_format(duration));

	bot_send(pinger->module.bot, &pinger->module, from_module, ping->channel,
		response);
}

static void
on_msg(module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message) {
	char errbuf[128];
	regmatch_t matches[3];
	int error;
	char *name;
	pinger_t *pinger = (pinger_t *)module;
	struct ping *ping;

	error = regexec(&r_ping, message, 3, matches, 0);
	if (error == REG_NOMATCH) {
		return;
	} else if (error) {
		regerror(error, &r_ping, errbuf, sizeof errbuf);
		fprintf(stderr, "regexec: %s\n", errbuf);
		return;
	}

	// Get the addressee name
	size_t name_len = matches[1].rm_eo - matches[1].rm_so;
	name = (char *)malloc((name_len+1) * sizeof(char));
	strncpy(name, message + matches[1].rm_so, name_len);
	name[name_len] = '\0';

	char some_char = message[matches[2].rm_so];
	bool is_pong = (some_char == 'o' || some_char == 'O');
	if (is_pong) {
		// look up the ping that is being answered
		ping = pinger_lookup_ping(pinger, &(struct ping){
			.from = name,
			.to = sender,
			.channel = channel
		});
		if (!ping) {
			// pong without ping
			return;
		}
		// send response to channel
		double duration = difftime(time(NULL), ping->time);
		respond_ping(pinger, from_module, ping, duration);

		pinger_remove_ping(pinger, ping);
		ping_free(ping);
	} else {

		// create the ping
		pinger_store_ping(pinger, &(struct ping){
			.from = sender,
			.to = name,
			.channel = channel,
			.time = time(NULL)
		});
	}
}

module_t *
pinger_new() {
	pinger_t *pinger = calloc(1, sizeof(pinger_t));
	if (!pinger) {
		perror("calloc");
		return NULL;
	}

	compile_regexes();

	pinger->module.type = "pinger";
	pinger->module.name = "pinger";
	pinger->module.on_msg = on_msg;

	return (module_t *)pinger;
}
