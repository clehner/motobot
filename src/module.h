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
	module_t *next;

	// Methods
	int (*config) (module_t *module, const char *name, const char *value);
	int (*connect) (module_t *module);
	int (*process_select_descriptors) (module_t *module, fd_set *in_set, fd_set *out_set);
	int (*add_select_descriptors) (module_t *module, fd_set *in_set, fd_set *out_set, int *maxfd);

	// Callbacks
	int (*on_msg) (module_t *module, module_t *sender_module);
};

module_t *
module_new(const char *module_type);

void
module_config(module_t *module, const char *name, const char *value);

/*
int module_config (module_t *module, const char *name, const char *value);
int module_on_msg (module_t *module, module_t *sender_module);
int module_process_select_descriptors (module_t *module, fd_set *in_set, fd_set *out_set);
int module_add_select_descriptors (module_t *module, fd_set *in_set, fd_set *out_set, int *maxfd);
*/

#endif /* MODULE_H */
