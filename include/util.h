#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>

#if defined(DEBUG_REALZADOR) || defined(DEBUG_DESENFOCADOR) ||                \
    defined(DEBUG_COMMON_FILTER)
#define DEBUG_PRINT(prefix, fmt, ...)                                         \
    do {                                                                      \
        fprintf(stdout, "%s: " fmt, prefix, ##__VA_ARGS__);                   \
    } while (0)
#else
#define DEBUG_PRINT(prefix, fmt, ...)                                         \
    do {                                                                      \
    } while (0)
#endif

#endif // UTIL_H
