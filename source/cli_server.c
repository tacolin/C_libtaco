#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli_server.h"

#define CTRL(key) (key - '@')

#define derror(a, b...) fprintf(stderr, "[ERROR] %s(): "a"\n", __func__, ##b)
#define CHECK_IF(assertion, error_action, ...) \
{\
    if (assertion) \
    { \
        derror(__VA_ARGS__); \
        {error_action;} \
    }\
}

static const char _negotiate[] = "\xFF\xFB\x03"
                                 "\xFF\xFB\x01"
                                 "\xFF\xFD\x03"
                                 "\xFF\xFD\x01";
