#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>

/**
 * @brief Termina el programa con un mensaje de error.
 *
 * Esta función imprime un mensaje de error formateado y termina
 * la ejecución del programa. Es útil para manejar errores fatales.
 */
void
die(const char *format, ...);

/**
 * @brief Imprime una barra de progreso.
 */
void
print_progress_bar(float progress, const char *prefix);

#endif // UTIL_H
