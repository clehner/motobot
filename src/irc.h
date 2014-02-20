/*
 * irc.h
 * Copyright (c) 2014 Charles Lehner
 */

#ifndef IRC_H
#define IRC_H

#include "libircclient/libircclient.h"
#include "libircclient/libirc_rfcnumeric.h"
#include "libircclient/libirc_errors.h"
#include "libircclient/libirc_events.h"
#include "module.h"

typedef struct irc irc_t;

struct irc {
	module_t module;
	irc_callbacks_t callbacks;
	irc_session_t *session;
	const char *server;
	unsigned char ssl;
	unsigned char ipv6;
	unsigned short port;
	const char *password;
	const char *nick;
	const char *username;
	const char *realname;
};

irc_t *
irc_new();

#endif /* IRC_H */
