#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "inih/ini.h"
#include "libircclient/libircclient.h"
#include "libircclient/libirc_rfcnumeric.h"

typedef struct {
} configuration_t;

static int config_handler(void* user, const char* section, const char* name,
    const char* value)
{
    printf("[%s] %s = %s\n", section, name, value);
    /*
    configuration_t* pconfig = (configuration_t*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("protocol", "version")) {
        pconfig->version = atoi(value);
    } else if (MATCH("user", "name")) {
        pconfig->name = strdup(value);
    } else if (MATCH("user", "email")) {
        pconfig->email = strdup(value);
    } else {
        return 0;  // unknown section/name, error
    }
*/
    return 1;
}

int
main(int argc, char *argv[]) {
    configuration_t config;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s config.ini\n", argv[0]);
        return 1;
    }

    if (ini_parse(argv[1], config_handler, &config) < 0) {
        fprintf(stderr, "Can't load '%s'\n", argv[1]);
        return 1;
    }

    return 0;
}

