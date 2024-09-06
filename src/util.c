#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "util.h"

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

static const char *spinner = "⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏";
static int spinner_index = 0;

void
die(const char *format, ...) {
    va_list args;
    va_start(args, format);
    fprintf(stderr, ANSI_COLOR_RED "Error: " ANSI_COLOR_RESET);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

void
print_progress_bar(float progress, const char *prefix) {
    static time_t start_time = 0;
    const int bar_width = 30;
    const int pos = bar_width * progress;
    const int percent = progress * 100;

    if (start_time == 0) {
        start_time = time(NULL);
    }
    int elapsed_seconds = (int)difftime(time(NULL), start_time);

    printf("\r\033[K%c " ANSI_COLOR_CYAN "%s " ANSI_COLOR_YELLOW
           "[%3d%%] " ANSI_COLOR_BLUE "[" ANSI_COLOR_RESET,
           spinner[spinner_index], prefix, percent);

    spinner_index = (spinner_index + 1) % strlen(spinner);

    for (int i = 0; i < bar_width; ++i) {
        printf(i < pos    ? ANSI_COLOR_GREEN "█"
               : i == pos ? ANSI_COLOR_YELLOW "▓"
                          : ANSI_COLOR_RED "░");
    }

    printf(ANSI_COLOR_BLUE "]" ANSI_COLOR_RESET " %02d:%02d",
           elapsed_seconds / 60, elapsed_seconds % 60);

    if (progress >= 1.0) {
        printf("\n");
        start_time = 0;
    }

    fflush(stdout);
}
