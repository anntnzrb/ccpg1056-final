#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "publicador.h"
#include "bmp.h"

#define DEBUG_PUBLISHER 1

#define SHM_NAME "/bmp_imagen_compartida"

int publicar_imagen(const char *nombre_archivo, BMP_Image **imagen_compartida) {
    printf("Debug: Iniciando publicar_imagen\n");

    FILE *archivo = fopen(nombre_archivo, "rb");
    if (!archivo) {
        perror("Error al abrir el archivo");
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug: Archivo abierto correctamente\n");
    #endif

    BMP_Image *imagen = malloc(sizeof(BMP_Image));
    if (!imagen) {
        perror("Error de asignación de memoria");
        fclose(archivo);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug: Memoria asignada para imagen\n");
    #endif

    if (readImage(archivo, imagen), imagen->pixels == NULL) {
        fprintf(stderr, "Error al leer la imagen BMP\n");
        fclose(archivo);
        free(imagen);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug: Imagen leída correctamente\n");
    #endif

    fclose(archivo);

    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error al crear memoria compartida");
        free(imagen);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug: Memoria compartida creada\n");
    #endif

    size_t shm_size = sizeof(BMP_Image) + imagen->header.imagesize;
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("Error al establecer el tamaño de la memoria compartida");
        free(imagen);
        shm_unlink(SHM_NAME);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug: Tamaño de memoria compartida establecido\n");
    #endif

    void *ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        free(imagen);
        shm_unlink(SHM_NAME);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug: Memoria compartida mapeada\n");
    #endif

    memcpy(ptr, imagen, sizeof(BMP_Image));
    memcpy((char*)ptr + sizeof(BMP_Image), imagen->pixels, imagen->header.imagesize);

    #if DEBUG_PUBLISHER
    printf("Debug: Imagen copiada a memoria compartida\n");
    #endif

    *imagen_compartida = (BMP_Image *)ptr;

    free(imagen);

    #if DEBUG_PUBLISHER
    printf("Debug: publicar_imagen completado con éxito\n");
    #endif

    return 0;
}
