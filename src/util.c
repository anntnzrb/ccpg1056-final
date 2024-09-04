#include <stdio.h>
#include <stdlib.h>

#include "util.h"

void
die(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}
