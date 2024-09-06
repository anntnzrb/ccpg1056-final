#ifndef FILTER_COMMON_H
#define FILTER_COMMON_H

#include <pthread.h>

#include "bmp.h"

#define FILTER_SIZE 3

// img keys
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
    int is_bottom_up;
} FilterParameters;

/**
 * @brief Obtiene una imagen desde la memoria compartida.
 * @param image_key Clave de la imagen (IMAGE_INPUT o IMAGE_OUTPUT)
 * @param size Tamaño de la imagen en bytes
 * @return Puntero a la imagen en memoria compartida, o NULL si hay un error
 */
void *
get_shared_image(const char *image_key, int size);

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