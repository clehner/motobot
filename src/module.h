/*
 * module.h
 * Copyright (c) 2014 Charles Lehner
 */

#ifndef MODULE_H
#define MODULE_H

#include <sys/select.h>

typedef struct module module_t;

struct module {
	const char *type;
	const char *name;
	module_t *next;
	struct bot *bot;

	// Methods
	void (*config) (module_t *module, const char *name, const char *value);
	int (*connect) (module_t *module);
	int (*process_select_descriptors) (module_t *module, fd_set *in_set, fd_set *out_set);
	int (*add_select_descriptors) (module_t *module, fd_set *in_set, fd_set *out_set, int *maxfd);
	void (*send) (module_t *module, const char *channel, const char *message);

	// Callbacks
	void (*on_msg) (module_t *module, module_t *from_module, const char *channel,
		const char *sender, const char *message);
	void (*on_privmsg) (module_t *module, module_t *from_module,
		const char *sender, const char *message);
	void (*on_read_log) (module_t *module,
		const char *sender, const char *message);
};

module_t *
module_new(const char *module_type);

/*
int module_config (module_t *module, const char *name, const char *value);
int module_on_msg (module_t *module, module_t *sender_module);
int module_process_select_descriptors (module_t *module, fd_set *in_set, fd_set *out_set);
int module_add_select_descriptors (module_t *module, fd_set *in_set, fd_set *out_set, int *maxfd);
*/

#endif /* MODULE_H */
