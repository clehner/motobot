#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include "bot.h"
#include "module.h"
#include "hash/hash.h"
#include "command.h"
#include "todo.h"
#include "chains/chains.h"

struct todo {
	module_t module;
	char filename[256];
};

FILE *
todo_open(todo_t *todo, const char *mode) {
	FILE *file = fopen(todo->filename, mode);
	if (!file) {
		perror("fopen");
		return NULL;
	}
	return file;
}

// count todo items in the file
int
count_items(todo_t *todo) {
	int num_items = 0;
	FILE *file = todo_open(todo, "r");
	if (!file) {
		return -1;
	}

	for (char c = 0; c != EOF; c = fgetc(file)) {
		if (c == '\n') num_items++;
	}

	fclose(file);
	return num_items;
}

// List items that will fit in a buffer
// Returns string, or NULL if no more items
char *
todo_list_partial(todo_t *todo, command_env_t *env, FILE *file, int *item_id) {
	static char items[512];
	int i = 0;
	char *line = NULL;
	char temp_line[512];
	int temp_size;
	size_t len = 0;
	ssize_t chars;

	if (feof(file)) {
		return NULL;
	}

	items[0] = '\0';

	while ((chars = getline(&line, &len, file)) != -1) {
		if (chars == 0) continue;

		// Skip newline if found
		if (line[chars-1] == '\n') {
			line[chars-1] = '\0';
		}

		// Append to list
		temp_size = snprintf(temp_line, sizeof(temp_line), " [%d] %s",
				*item_id, line);

		if (temp_size > sizeof(temp_line)) {
			// Item was truncated.
			temp_size = sizeof(temp_line);
		}

		if (i + temp_size > sizeof(items)) {
			// List is too long
			// Rewind last line
			fseek(file, chars, SEEK_CUR);
			return items;
		}

		strncat(items + i, temp_line, temp_size);
		i += temp_size;
		*item_id = *item_id + 1;
	}

	return items;
}

void
todo_list(todo_t *todo, command_env_t *env) {
	FILE *file = todo_open(todo, "r");
	int item_id = 0;
	char *items;

	if (!file) {
		command_respond(*env, "No todo items.");
		return;
	}

	while ((items = todo_list_partial(todo, env, file, &item_id))) {
		command_respond(*env, "Todo:%s", items);
	}

	fclose(file);
}

void
todo_add(todo_t *todo, command_env_t *env, char *item) {
	int item_id;
	FILE *file = todo_open(todo, "a+");

	if (!file) {
		command_respond(*env, "Unable to open todo file.");
		return;
	}

	// Write the line
	fprintf(file, "%s\n", item);

	// Get the index of the newly added item
	rewind(file);
	item_id = count_items(todo)-1;
	if (item_id >= 0) {
		command_respond(*env, "Added todo item %d", item_id);
	} else {
		command_respond(*env, "Unable to add todo item");
	}

}

bool
get_id(char *str, int *item_id) {
	errno = 0;
	*item_id = strtol(str, NULL, 10);
	return (errno == 0);
}

void
todo_remove(todo_t *todo, command_env_t *env, int item_id)
{
	char c;
	size_t offset = 0;
	size_t remove_start;
	int line_num = -1;
	char buf[1024];
	size_t after_len;
	FILE *file = todo_open(todo, "r+");
	if (!file) {
		return;
	}

	for (c = '\n'; c != EOF; c = fgetc(file)) {
		offset++;
		if (c == '\n') {
			line_num++;
			if (line_num == item_id) {
				break;
			}
		}
	}

	if (line_num != item_id) {
		// Didn't find the item to remove
		command_respond(*env, "Unable to find todo item %d", item_id);
		fclose(file);
		return;
	}

	remove_start = offset-1;
	// Find the end of the line
	for (c = '\0'; c != EOF && c != '\n'; c = fgetc(file)) {
		offset++;
	}

	// Read the rest of the file into memory after the line being removed
	after_len = fread(buf, 1, sizeof(buf), file);
	if (ferror(file)) {
		command_respond(*env, "Unable to remove todo item %d", item_id);
		fclose(file);
		return;
	}

	// Go back to the line to remove, and overwrite it with what follows it
	fseek(file, remove_start, SEEK_SET);
	fwrite(buf, 1, after_len, file);

	fclose(file);

	// Truncate file
	if (truncate(todo->filename, remove_start + after_len)) {
		perror("truncate");
		command_respond(*env, "Unable to fully remove todo item %d", item_id);
	}

	command_respond(*env, "Removed todo %d", item_id);
}

void
command_todo(command_env_t env, int argc, char **argv) {
	int item_id;
	todo_t *todo = (todo_t *)env.command_module;
	if (!todo->filename) {
		command_respond(env, "Missing todo file name");
		return;
	}

	if (argc == 1) {
		todo_list(todo, &env);

	} else if (argc == 2 && get_id(argv[1], &item_id)) {
		todo_remove(todo, &env, item_id);

	} else if (argc > 1) {
		char item[256];
		// Flatten the arguments into a string
		sequence_concat(item, argv+1, sizeof(item), argc-1, 1);
		// Add the item
		todo_add(todo, &env, item);
	}
	//command_respond(env, "Usage: %s [add|list|rm] ...", argv[0]);
}

static void
config(module_t *module, const char *name, const char *value) {
	todo_t *todo = (todo_t *)module;
	if (strcmp(name, "file") == 0) {
		strncpy(todo->filename, value, sizeof(todo->filename));
	}
}

static command_t commands[] = {
	{"/todo", command_todo},
	{NULL}
};

module_t *
todo_new() {
	todo_t *todo = calloc(1, sizeof(todo_t));
	if (!todo) {
		perror("calloc");
		return NULL;
	}

	todo->module.type = "todo";
	todo->module.config = config;
	todo->module.commands = commands;

	return (module_t *)todo;
}
