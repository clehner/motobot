/*
 * irc.c
 * Copyright (c) 2014 Charles Lehner
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include "libircclient/libircclient.h"
#include "libircclient/libirc_rfcnumeric.h"
#include "libircclient/libirc_errors.h"
#include "libircclient/libirc_events.h"
#include "module.h"
#include "bot.h"
#include "irc.h"

#define debug 0

typedef struct irc irc_t;

struct irc {
	module_t module;
	irc_session_t *session;
	struct irc *next;

	// config
	const char *server;
	unsigned char ssl;
	unsigned char ipv6;
	unsigned short port;
	const char *password;
	const char *nick;
	const char *username;
	const char *realname;
	struct channel *channels;

	// current
	char *current_nick;
	char in_playback_mode;
};

struct channel {
	const char *name;
	struct channel *next;
	struct nick *nicks;
};

typedef struct channel channel_t;

static const char me_prefix[] = "/me ";

// Linked-list of module instances
// This is used to map irc_session to irc module
// because ircclient doesn't let us store arbitrary data in the irc_session_t
static irc_t *modules = NULL;

char *my_strdup(const char *s) {
	size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if(p) strncpy(p, s, len);
    return p;
}
char *my_strdup(const char *s);
#define strdup(x) my_strdup(x)

// Get the next module in the linked list of created modules
irc_t *
get_module(irc_session_t *session) {
	for (irc_t *irc = modules; irc; irc = irc->next) {
		if (irc->session == session) {
			return irc;
		}
	}
	return NULL;
}

channel_t *
add_channel(irc_t *irc, const char *channel_name) {
	if (!channel_name) return NULL;
	// Add channel to the linked list
	channel_t *channel = malloc(sizeof(channel_t));
	channel->name = strdup(channel_name);
	channel->next = irc->channels;
	channel->nicks = NULL;
	irc->channels = channel;
	return channel;
}

channel_t *
get_channel(irc_t *irc, const char *channel_name) {
	if (!channel_name) return NULL;
	for (channel_t *channel = irc->channels; channel; channel = channel->next) {
		if (channel->name && !strcmp(channel->name, channel_name)) {
			return channel;
		}
	}
	return NULL;
}

void
channel_add_nick(channel_t *channel, const char *name) {
	nick_t *nick = malloc(sizeof(nick_t));
	if (!nick) return;
	nick->name = strdup(name);
	nick->next = channel->nicks;
	channel->nicks = nick;
	//printf("Adding nick \"%s\" to channel \"%s\"\n", name, channel->name);
}

void
channel_remove_nick(channel_t *channel, const char *name) {
	//printf("Removing nick \"%s\" from channel \"%s\"\n", name, channel->name);
	if (!name) return;
	nick_t *prev_nick = channel->nicks;
	if (!prev_nick) return;
	if (prev_nick->name && !strcmp(prev_nick->name, name)) {
		channel->nicks = NULL;
		free(prev_nick->name);
		free(prev_nick);
		return;
	}
	for (nick_t *nick = prev_nick->next; nick; nick = nick->next) {
		if (nick->name && !strcmp(nick->name, name)) {
			prev_nick->next = nick->next;
			free(nick->name);
			free(nick);
			return;
		}
	}
	fprintf(stderr, "Unable to remove nick \"%s\"\n", name);
}

/*
int
channel_contains_nick(channel_t *channel, const char *name) {
	if (!name) return 0;
	for (nick_t *nick = channel->nicks; nick; nick = nick->next) {
		if (nick->name && !strcmp(nick->name, name)) {
			return 1;
		}
	}
	return 0;
}
*/

void
print_array(const char *array[], int n) {
	for (int i = 0; i < n; i++) {
		printf("%s%s", array[i], i < n-1 ? ", " : "\n");
	}
}

static void
event_connect(irc_session_t *session, const char *event, const char *origin,
		const char **params, unsigned int count) {
	irc_t *irc = get_module(session);

	printf("Connected to %s:%u.\n", irc->server, irc->port);

	// Join our channels
	for (channel_t *channel = irc->channels; channel; channel = channel->next) {
		if (irc_cmd_join(session, channel->name, NULL)) {
			fprintf(stderr, "irc: %s\n", irc_strerror(irc_errno(session)));
			return;
		}
	}
}

static void
event_numeric(irc_session_t *session, unsigned int event, const char *origin,
		const char **params, unsigned int count) {
	switch(event) {
		case 464:
			fprintf(stderr, "[%s] Bad password\n", origin);
			break;
		case LIBIRC_RFC_RPL_MOTD:
			/*
			if (count > 1) {
				printf("[%s] MOTD: %s\n", origin, params[1]);
			}
			*/
		case LIBIRC_RFC_RPL_MOTDSTART:
		case LIBIRC_RFC_RPL_ENDOFMOTD:
			break;
		case LIBIRC_RFC_RPL_NAMREPLY:
			if (count < 4) break;
			irc_t *irc = get_module(session);
			//printf("[%s] NAMREPLY (%u): ", origin, count);
			//print_array(params, count);
			//nick, thingy, channel, member member2...
			const char *channel_name = params[2];
			const char *members = params[3];
			char members_copy[256];
			channel_t *channel = get_channel(irc, channel_name);
			if (!channel) {
				// maybe the server joined us to a channel
				channel = add_channel(irc, channel_name);
			}
			strncpy(members_copy, members, sizeof(members_copy));
			printf("[%s] Members of %s: %s\n", origin, channel_name, members);

			// Add each nick to the list
			// ignore nick prefixes
			static const char *delimeters = " +@";
			for (const char *nick = strtok(members_copy, delimeters);
				nick;
				nick = strtok(NULL, delimeters)) {

				// Don't add outself
				if (strcasecmp(nick, irc->current_nick)) {
					channel_add_nick(channel, nick);
				}
			}

		case LIBIRC_RFC_RPL_ENDOFNAMES:
			break;

		case LIBIRC_RFC_ERR_BANNEDFROMCHAN:
		case LIBIRC_RFC_ERR_INVITEONLYCHAN:
		case LIBIRC_RFC_ERR_BADCHANNELKEY:
		case LIBIRC_RFC_ERR_CHANNELISFULL:
		case LIBIRC_RFC_ERR_BADCHANMASK:
		case LIBIRC_RFC_ERR_NOSUCHCHANNEL:
		case LIBIRC_RFC_ERR_TOOMANYCHANNELS:
			fprintf(stderr, "Unable to join channel: %s\n",
					irc_strerror(irc_errno(session)));
			break;

		default:
			if (debug) {
				printf("[%s] %u: ", origin, event);
				print_array(params, count);
			}
	}
}

static void
event_channel(irc_session_t *session, const char *event, const char *origin,
		const char **params, unsigned int count) {
	const char *channel = params[0];
	const char *message = params[1];
	printf("[%s] <%s> %s\n", channel, origin, message);

	irc_t *irc = get_module(session);

	// Treat ZNC's buffered-up messages as new
	// (potentially responding to them)
	if (!irc->in_playback_mode) {
		if (strcmp(origin, "***") == 0 &&
				strcmp(message, "Buffer Playback...") == 0) {
			if (debug) printf("Entering buffer playback mode\n");
			irc->in_playback_mode = 1;
			return;
		}

	} else {
		if (strcmp(origin, "***") == 0 &&
				strcmp(message, "Playback Complete.") == 0) {
			if (debug) printf("Buffer playback complete\n");
			irc->in_playback_mode = 0;
			return;
		}

		// This is a played-back message.
		// Skip the timestamp (first word)
		message = strchr(message, ' ');
		if (!message) {
			fprintf(stderr, "Broken played-back message\n");
			return;
		}
		// Skip the space after the timestamp
		message++;
	}

	// Process message
	bot_on_msg(irc->module.bot, &irc->module, channel, origin, message);
}

static void
event_privmsg(irc_session_t *session, const char *event, const char *origin,
		const char **params, unsigned int count) {
	// const char *my_nick = params[0];
	const char *message = params[1];
	printf("<%s> %s\n", origin, message);

	// Ignore messages from *special
	if (origin[0] == '*') {
		return;
	}

	if (message) {
		irc_t *irc = get_module(session);
		bot_on_privmsg(irc->module.bot, &irc->module, origin, message);
	}
}

static void
event_ctcp_action(irc_session_t *session, const char *event, const char *origin,
		const char **params, unsigned int count) {
	const char *channel = params[0];
	const char *message = params[1];
	printf("[%s] * %s %s\n", channel, origin, message);
	if (!message) return;
	irc_t *irc = get_module(session);

	// Handle action as a message /me ...
	char me_message[256];
	strcpy(me_message, me_prefix);
	strncat(me_message, message, sizeof(me_message) - sizeof(me_prefix));
	bot_on_msg(irc->module.bot, &irc->module, channel, origin, me_message);
}

static void
event_nick(irc_session_t *session, const char *event, const char *old_nick,
		const char **params, unsigned int count) {
	const char *new_nick = params[0];
	if (count < 1) {
		return;
	}
	irc_t *irc = get_module(session);
	if (irc->current_nick && strcmp(irc->current_nick, old_nick)) {
		printf("(nick) %s -> %s\n", old_nick, new_nick);
	} else {
		if (irc->current_nick) {
			free(irc->current_nick);
		}
		irc->current_nick = strdup(new_nick);
		irc->module.name = irc->current_nick;
		if (debug) {
			printf("Nick changed: %s\n", new_nick);
		}
		if (irc_cmd_nick(session, irc->nick)) {
			fprintf(stderr, "irc: %s\n", irc_strerror(irc_errno(irc->session)));
		}
	}
}

static void
event_join(irc_session_t *session, const char *event, const char *nick,
		const char **params, unsigned int count) {
	const char *channel_name = params[0];
	if (count < 1) return;
	irc_t *irc = get_module(session);
	if (irc->current_nick && !strcasecmp(irc->current_nick, nick)) {
		// so we joined
		// good for us
	} else {
		// someone else joined
		if (debug) {
			printf("Joined (%s): %s\n", channel_name, nick);
		}

		// Update our internal list of members of the channel
		channel_t *channel = get_channel(irc, channel_name);
		channel_add_nick(channel, nick);
	}
}

static void
event_part(irc_session_t *session, const char *event, const char *nick,
		const char **params, unsigned int count) {
	const char *channel_name = params[0];
	if (count < 1) return;
	irc_t *irc = get_module(session);
	if (irc->current_nick && !strcmp(irc->current_nick, nick)) {
		// we left
	} else {
		// someone else left
		if (debug) {
			printf("Parted (%s): %s\n", channel_name, nick);
		}

		// Update our internal list of members of the channel
		channel_t *channel = get_channel(irc, channel_name);
		channel_remove_nick(channel, nick);
	}
}

nick_t *
get_nicks(module_t *module, const char *channel_name) {
	irc_t *irc = (irc_t *)module;
	channel_t *channel = get_channel(irc, channel_name);
	return channel ? channel->nicks : NULL;
}

static void
config(module_t *module, const char *name, const char *value) {
	irc_t *irc = (irc_t *)module;

	if (strcmp(name, "server") == 0) {
		// Prepend server with "#" in case we want SSL
		int len = strlen(value)+1;
		char *server = malloc(len+1);
		if (server) {
			server[0] = '#';
			strncpy(server+1, value, len);
		}
		irc->server = server+1;

	} else if (strcmp(name, "ssl") == 0) {
		if (strcmp(value, "yes") == 0) {
			irc->ssl = 1;
		} else if (strcmp(value, "no") == 0) {
			irc->ssl = 0;
		} else {
			fprintf(stderr, "SSL option '%s' should be yes or no)\n", value);
		}

	} else if (strcmp(name, "ipv6") == 0) {
		if (strcmp(value, "yes") == 0) {
			irc->ipv6 = 1;
		} else if (strcmp(value, "no") == 0) {
			irc->ipv6 = 0;
		} else {
			fprintf(stderr, "IPv6 option '%s' should be yes or no)\n", value);
		}

	} else if (strcmp(name, "port") == 0) {
		irc->port = atoi(value);

	} else if (strcmp(name, "password") == 0) {
		irc->password = strdup(value);
	} else if (strcmp(name, "nick") == 0) {
		irc->nick = strdup(value);
		irc->current_nick = strdup(value);
		irc->module.name = irc->current_nick;
	} else if (strcmp(name, "username") == 0) {
		irc->username = strdup(value);
	} else if (strcmp(name, "realname") == 0) {
		irc->realname = strdup(value);

	} else if (strcmp(name, "channel") == 0) {
		add_channel(irc, value);
	}
}

static int
module_connect(module_t *module) {
	irc_t *irc = (irc_t *)module;

	printf("Connecting to server: %s, port: %u\n",
		&irc->server[irc->ssl ? -1 : 0],
		irc->port);

	if ((irc->ipv6 ? irc_connect6 : irc_connect)
			(irc->session,
			 &irc->server[irc->ssl ? -1 : 0],
			 irc->port,
			 irc->password,
			 irc->nick,
			 irc->username,
			 irc->realname)) {
		fprintf(stderr, "irc: %s\n", irc_strerror(irc_errno(irc->session)));
		return 0;
	}
	return 1;
}

static int
add_select(module_t *module, fd_set *in_set, fd_set *out_set, int *maxfd) {
	irc_t *irc = (irc_t *)module;
	return irc_add_select_descriptors(irc->session, in_set, out_set, maxfd);
}

static int
process_select(module_t *module, fd_set *in_set, fd_set *out_set) {
	irc_t *irc = (irc_t *)module;
	return irc_process_select_descriptors(irc->session, in_set, out_set);
}

static void
send(module_t *module, const char *channel, const char *message) {
	irc_t *irc = (irc_t *)module;

	if (strncmp(message, me_prefix, strlen(me_prefix)) == 0) {
		const char *action = message + strlen(me_prefix);
		// Transmit an action
		printf("[%s] * %s %s\n", channel, irc->current_nick, action);

		if (irc_cmd_me(irc->session, channel, action)) {
			fprintf(stderr, "irc: %s\n", irc_strerror(irc_errno(irc->session)));
			return;
		}
	} else {
		// Send a regular message
		printf("[%s] <%s> %s\n", channel, irc->current_nick, message);

		if (irc_cmd_msg(irc->session, channel, message)) {
			fprintf(stderr, "irc: %s\n", irc_strerror(irc_errno(irc->session)));
			return;
		}
	}

	bot_on_read_log(module->bot, irc->current_nick, message);
}

// Callbacks for IRC client
irc_callbacks_t callbacks = {
	.event_connect = event_connect,
	.event_numeric = event_numeric,
	.event_channel = event_channel,
	.event_privmsg = event_privmsg,
	.event_nick = event_nick,
	.event_join = event_join,
	.event_part = event_part,
	//.event_ctcp_req = event_ctcp_req,
	.event_ctcp_action = event_ctcp_action
};

module_t *
irc_new() {
	irc_t *irc = calloc(1, sizeof(irc_t));

	if (!irc) {
		perror("calloc");
		return NULL;
	}

	irc->module.type = "irc";
	irc->module.name = NULL;
	irc->module.connect = module_connect;
	irc->module.config = config;
	irc->module.process_select_descriptors = process_select;
	irc->module.add_select_descriptors = add_select;
	irc->module.send = send;
	irc->module.get_nicks = get_nicks;

	irc->server = "";
	irc->ssl = 0;
	irc->ipv6 = 0;
	irc->port = 6667;
	irc->password = NULL;
	irc->nick = "guest";
	irc->username = "nobody";
	irc->realname = "noname";
	irc->channels = NULL;

	// Add this module to the global linked list
	irc->next = modules;
	modules = irc;

	irc->session = irc_create_session(&callbacks);
	if (!irc->session) {
		fprintf(stderr, "irc: %s\n", irc_strerror(irc_errno(irc->session)));
	}
	irc_option_set(irc->session, LIBIRC_OPTION_STRIPNICKS);

	return (module_t *)irc;
}
