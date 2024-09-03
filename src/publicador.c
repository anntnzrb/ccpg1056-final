#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bmp.h"
#include "publicador.h"

#define DEBUG_PUBLISHER 0

// nombre de shared mem
#define SHM_NAME "/bmp_imagen_compartida"

int
publicar_imagen(const char *nombre_archivo, BMP_Image **imagen_compartida) {
    // Abrir archivo BMP en modo lectura binaria
    FILE *archivo = fopen(nombre_archivo, "rb");
    if (!archivo) {
        fprintf(stderr, "Error: No se pudo abrir el archivo '%s'\n", nombre_archivo);
        return -1;
    }

#if DEBUG_PUBLISHER
    printf("Debug (%s): Archivo abierto correctamente\n", nombre_archivo);
#endif

    // Asignar memoria para estructura BMP_Image
    BMP_Image *imagen = malloc(sizeof(BMP_Image));
    if (!imagen) {
        perror("Error de asignación de memoria");
        fclose(archivo);
        return -1;
    }

#if DEBUG_PUBLISHER
    printf("Debug (%s): Memoria asignada para imagen\n", nombre_archivo);
#endif

    // Leer imagen
    readImage(archivo, imagen);
    if (imagen->pixels == NULL) {
        fprintf(stderr, "Error: No se pudo leer la imagen '%s'\n", nombre_archivo);
        fclose(archivo);
        free(imagen);
        return -1;
    }

#if DEBUG_PUBLISHER
    printf("Debug (%s): Imagen leída correctamente\n", nombre_archivo);
#endif

    fclose(archivo);

    // Abrir memoria compartida existente
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error al abrir memoria compartida");
        free(imagen);
        return -1;
    }

#if DEBUG_PUBLISHER
    printf("Debug (%s): Memoria compartida abierta\n", nombre_archivo);
#endif

    // Calcular tamaño total necesario para memoria compartida
    size_t required_shm_size = sizeof(size_t) + sizeof(BMP_Image) + imagen->header.imagesize;
    printf("Debug: Required image size = %zu bytes\n", required_shm_size);

    // Obtener el tamaño actual de la memoria compartida
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("Error al obtener el tamaño actual de la memoria compartida");
        free(imagen);
        close(shm_fd);
        return -1;
    }
    size_t current_shm_size = shm_stat.st_size;

    // Ajustar el tamaño de la memoria compartida si es necesario
    if (required_shm_size > current_shm_size) {
        if (ftruncate(shm_fd, required_shm_size) == -1) {
            perror("Error al ajustar el tamaño de la memoria compartida");
            free(imagen);
            close(shm_fd);
            return -1;
        }
        current_shm_size = required_shm_size;
    }

    // Mapear la memoria compartida
    void *shm_ptr = mmap(NULL, current_shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        free(imagen);
        close(shm_fd);
        return -1;
    }

#if DEBUG_PUBLISHER
    printf("Debug (%s): Memoria compartida mapeada\n", nombre_archivo);
#endif

    // Actualizar el tamaño en la memoria compartida
    *((size_t *)shm_ptr) = current_shm_size;

    // Copiar la imagen a la memoria compartida
    *imagen_compartida = (BMP_Image *)((char *)shm_ptr + sizeof(size_t));
    memcpy(*imagen_compartida, imagen, sizeof(BMP_Image));
    memcpy((*imagen_compartida)->pixels, imagen->pixels, imagen->header.imagesize);

    // Liberar memoria
    free(imagen);

    // Desmapear la memoria compartida
    if (munmap(shm_ptr, current_shm_size) == -1) {
        perror("Error al desmapear la memoria compartida");
    }

    // Cerrar el descriptor de archivo de memoria compartida
    if (close(shm_fd) == -1) {
        perror("Error al cerrar el descriptor de archivo de memoria compartida");
    }

#if DEBUG_PUBLISHER
    printf("Debug (%s): publicar_imagen completado con éxito\n", nombre_archivo);
#endif

    return 0;
}
