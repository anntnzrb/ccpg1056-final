#include "publicador.h"
#include "bmp.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#define DEBUG_PUBLISHER 1

// nombre de shared mem
#define SHM_NAME "/bmp_imagen_compartida"

int publicar_imagen(const char *nombre_archivo, BMP_Image **imagen_compartida) {
    FILE *file = fopen(nombre_archivo, "rb");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Archivo abierto correctamente\n", nombre_archivo);
    #endif

    BMP_Image *imagen = (BMP_Image *)malloc(sizeof(BMP_Image));
    if (imagen == NULL) {
        perror("Error al asignar memoria para BMP_Image");
        fclose(file);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Memoria asignada para BMP_Image\n", nombre_archivo);
    #endif

    readImage(file, imagen);
    fclose(file);

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Imagen leída correctamente\n", nombre_archivo);
    printf("Debug (%s): Tamaño de imagen: %u bytes\n", nombre_archivo, imagen->header.imagesize);
    #endif

    // Crear memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error al crear memoria compartida");
        free(imagen);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Memoria compartida creada\n", nombre_archivo);
    #endif

    size_t shm_size = sizeof(BMP_Image) + imagen->header.imagesize;
    #if DEBUG_PUBLISHER
    printf("Debug (%s): Tamaño total de memoria compartida: %zu bytes\n", nombre_archivo, shm_size);
    #endif

    if (ftruncate(shm_fd, shm_size) == -1) {
        printf("Error al ajustar el tamaño de la memoria compartida: %s\n", strerror(errno));
        free(imagen);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Tamaño de memoria compartida ajustado\n", nombre_archivo);
    #endif

    *imagen_compartida = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (*imagen_compartida == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        free(imagen);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Memoria compartida mapeada\n", nombre_archivo);
    #endif

    // Copiar la imagen a la memoria compartida
    memcpy(*imagen_compartida, imagen, sizeof(BMP_Image));
    memcpy((*imagen_compartida)->pixels, imagen->pixels, imagen->header.imagesize);

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Imagen copiada a memoria compartida\n", nombre_archivo);
    #endif

    free(imagen);
    close(shm_fd);

    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <ruta_imagen>\n", argv[0]);
        return 1;
    }

    BMP_Image *imagen_compartida;
    if (publicar_imagen(argv[1], &imagen_compartida) == 0) {
        printf("Imagen publicada en memoria compartida\n");
        return 0;
    } else {
        fprintf(stderr, "Error al publicar la imagen\n");
        return 1;
    }
}
