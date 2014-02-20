/*
 * irc.c
 * Copyright (c) 2014 Charles Lehner
 */

#include <stdio.h>
#include <string.h>
#include "irc.h"

char *my_strdup(const char *s) {
	size_t len = strlen(s) + 1;
    char *p = malloc(len);
    if(p) strncpy(p, s, len);
    return p;
}
char *my_strdup(const char *s);
#define strdup(x) my_strdup(x)

void
event_connect(irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count) {
	(void) session;
	(void) params;
	(void) event;
	(void) origin;
	(void) params;
	(void) count;
	printf("Connected!\n");
}

void
event_numeric(irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count) {
	(void) session;
	(void) params;
	(void) count;
	printf("Got event %u (%s)\n", event, origin);
}


irc_t *
irc_new() {
	irc_t *irc = calloc(1, sizeof(irc_t));
	// irc_t *irc = malloc(sizeof(irc_t));
	// memset(irc, 0, sizeof(irc_t));
	if (!irc) {
		perror("calloc");
		return NULL;
	}

	irc->module.type = "irc";
	irc->module.connect = irc_module_connect;
	irc->module.config = irc_config;
	irc->module.process_select_descriptors =
		irc_module_process_select_descriptors;
	irc->module.add_select_descriptors = irc_module_add_select_descriptors;

	irc->server = "";
	irc->ssl = 0;
	irc->ipv6 = 0;
	irc->port = 6667;
	irc->password = NULL;
	irc->nick = "guest";
	irc->username = "nobody";
	irc->realname = "noname";

	irc->callbacks.event_connect = event_connect;
	irc->callbacks.event_numeric = event_numeric;

	irc->session = irc_create_session(&irc->callbacks);
	if (!irc->session) {
		fprintf(stderr, "irc: %s\n", irc_strerror(irc_errno(irc->session)));
	}

	return irc;
}

int
irc_config(module_t *module, const char *name, const char *value) {
	irc_t *irc = (irc_t *)module;

	if (strcmp(name, "server") == 0) {
		// Prepend server with "#" in case we want SSL
		int len = strlen(value)+1;
		char *server = malloc(len+1);
		if (server) {
			server[0] = '#';
			strncpy(server+1, value, len);
		}
		irc->server = server;

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
	} else if (strcmp(name, "username") == 0) {
		irc->username = strdup(value);
	} else if (strcmp(name, "realname") == 0) {
		irc->realname = strdup(value);
	}
	return 1;
}

int
irc_module_connect(module_t *module) {
	irc_t *irc = (irc_t *)module;
	printf("Connecting to server: %s, port: %u\n",
		&irc->server[irc->ssl ? 0 : 1],
		irc->port);
	if ((irc->ipv6 ? irc_connect6 : irc_connect)
			(irc->session,
			 &irc->server[irc->ssl ? 0 : 1],
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

int
irc_module_add_select_descriptors(module_t *module, fd_set *in_set,
		fd_set *out_set, int *maxfd) {
	irc_t *irc = (irc_t *)module;
	return irc_add_select_descriptors(irc->session, in_set, out_set, maxfd);
}

int
irc_module_process_select_descriptors(module_t *module, fd_set *in_set,
		fd_set *out_set) {
	irc_t *irc = (irc_t *)module;
	return irc_process_select_descriptors(irc->session, in_set, out_set);
}

