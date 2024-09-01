#include "publicador.h"
#include "bmp.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEBUG_PUBLISHER 0

// nombre de shared mem
#define SHM_NAME "/bmp_imagen_compartida"

int publicar_imagen(const char *nombre_archivo, BMP_Image **imagen_compartida) {
    // abrir archivo (img) bmp en lectura binaria
    FILE *archivo = fopen(nombre_archivo, "rb");
    if (!archivo) {
        perror("Error al abrir el archivo");
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Archivo abierto correctamente\n", nombre_archivo);
    #endif

    // asignar mem para struct BMP_Image
    BMP_Image *imagen = malloc(sizeof(BMP_Image));
    if (!imagen) {
        perror("Error de asignación de memoria");
        fclose(archivo);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Memoria asignada para imagen\n", nombre_archivo);
    #endif

    // leer img
    if (readImage(archivo, imagen), imagen->pixels == NULL) {
        fprintf(stderr, "Error al leer la imagen BMP\n");
        fclose(archivo);
        free(imagen);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Imagen leída correctamente\n", nombre_archivo);
    #endif

    fclose(archivo);

    // crear/abrir shared mem
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error al crear memoria compartida");
        free(imagen);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Memoria compartida creada\n", nombre_archivo);
    #endif

    // calc tamaño total necesario para shared mem
    // tam = tam de la struct + tam de los datos de los pixeles
    size_t shm_size = sizeof(BMP_Image) + imagen->header.imagesize;
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("Error al establecer el tamaño de la memoria compartida");
        free(imagen);
        shm_unlink(SHM_NAME);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Tamaño de memoria compartida establecido\n", nombre_archivo);
    #endif

    // mapear shared mem en el espacio de direcciones del proceso
    void *ptr = mmap(0, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        free(imagen);
        shm_unlink(SHM_NAME);
        return -1;
    }

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Memoria compartida mapeada\n", nombre_archivo);
    #endif

    // copiar struct BMP_Image a shared mem
    memcpy(ptr, imagen, sizeof(BMP_Image));
    // copiar pixeles a shared mem
    memcpy((char *)ptr + sizeof(BMP_Image), imagen->pixels,
           imagen->header.imagesize);

    #if DEBUG_PUBLISHER
    printf("Debug (%s): Imagen copiada a memoria compartida\n", nombre_archivo);
    #endif

    // asignar puntero de shared mem al ptr de salida
    *imagen_compartida = (BMP_Image *)ptr;

    // free mem
    free(imagen);

    #if DEBUG_PUBLISHER
    printf("Debug (%s): publicar_imagen completado con éxito\n", nombre_archivo);
    #endif

    return 0;
}
