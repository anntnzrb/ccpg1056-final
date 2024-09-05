#ifndef FILTER_COMMON_H
#define FILTER_COMMON_H

#include <pthread.h>

#include "bmp.h"

#define FILTER_SIZE 3

extern const char *IMAGE_INPUT;
extern const char *IMAGE_OUTPUT;

// enum para los canales de la imagen
typedef enum { RED, BLUE, GREEN } Channel;

// struct para los parámetros del filtro
typedef struct {
    Pixel *image_in;
    Pixel *image_out;
    int start_row;
    int end_row;
    int columns;
    int height;
    const int (*filter)[FILTER_SIZE];
} FilterParameters;

/**
 * @brief Obtiene la imagen de entrada desde la memoria compartida.
 */
void *
get_input_image(int size);

/**
 * @brief Obtiene la imagen de salida desde la memoria compartida.
 */
void *
get_output_image(int size);

/**
 * @brief Aplica un filtro a una porción de la imagen en paralelo.
 */
void
apply_filter_parallel(Pixel *image_in, Pixel *image_out, int height, int width,
                      int start_row, int end_row, int num_threads,
                      const int filter[FILTER_SIZE][FILTER_SIZE]);

/**
 * @brief Añade padding a la imagen.
 */
Pixel **
add_padding(Pixel *image_in, int height, int width);

/**
 * @brief Libera la memoria de la imagen con padding.
 */
void
free_padded_image(Pixel **data, int height);

/**
 * @brief Trabajo del hilo del filtro.
 */
void *
filter_thread_worker(void *args);

/**
 * @brief Aplica un filtro a un canal de la imagen.
 */
int
apply_filter_to_channel(Pixel *pixels_in, Channel c, int x, int y, int width,
                        int height,
                        const int filter[FILTER_SIZE][FILTER_SIZE]);

/**
 * @brief Obtiene un canal de un pixel.
 */
static inline int
get_pixel_channel(const Pixel *pixel, Channel c) {
    switch (c) {
    case RED:
        return pixel->red;
    case BLUE:
        return pixel->blue;
    case GREEN:
        return pixel->green;
    default:
        return 0;
    }
}

#endif // FILTER_COMMON_H