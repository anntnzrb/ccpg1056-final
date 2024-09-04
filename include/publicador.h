#ifndef PUBLICADOR_H
#define PUBLICADOR_H

#include <sys/types.h>

#include "bmp.h"

/**
 * @brief Procesa una imagen utilizando un número específico de hilos.
 */
int
process_image(const char *filename, const char *output_path, BMP_Image *image, BMP_Image *new_image,
              int num_threads);

/**
 * @brief Copia los datos de la imagen a la memoria compartida.
 *
 * Wrapper de memcpy.
 */
void
copy_image_data(void *dest, BMP_Image *image, int is_input);

/**
 * @brief Crea un objeto de memoria compartida.
 */
int
create_shm(const char *name, int *fd, void **pixels_image, size_t shm_size);

/**
 * @brief Aplica un filtro a una porción de la imagen.
 */
pid_t
apply_filter(BMP_Image *img, int num_threads, const char *filter_name,
             int start_row, int end_row);

#endif // PUBLICADOR_H
