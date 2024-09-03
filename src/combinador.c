#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bmp.h"
#include "desenfocador.h"
#include "publicador.h"
#include "realzador.h"

#define THREAD_NUM 8
#define SHM_NAME "/bmp_imagen_compartida"

static void
handle_err(const int error_code) {
    printError(error_code);
    exit(EXIT_FAILURE);
}

int
main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ruta_entrada> <ruta_salida>\n", argv[0]);
        exit(1);
    }

    const char *ruta_entrada = argv[1];
    const char *ruta_salida = argv[2];

    // Publicar imagen de entrada
    BMP_Image *imagen_compartida;
    if (publicar_imagen(ruta_entrada, &imagen_compartida) != 0) {
        handle_err(FILE_ERROR);
    }

    // Asignar memoria para imagen de salida
    BMP_Image *imageOut = malloc(sizeof(BMP_Image));
    if (!imageOut)
        handle_err(MEMORY_ERROR);

    // Verificar y copiar imagen de entrada
    if (!checkBMPValid(&imagen_compartida->header))
        handle_err(VALID_ERROR);

    transBMP(imagen_compartida, imageOut);
    if (!checkBMPValid(&imageOut->header))
        handle_err(VALID_ERROR);

    // Aplicar filtros en paralelo
    apply_realzador_parallel(imagen_compartida, imageOut, THREAD_NUM);
    apply_desenfocador_parallel(imagen_compartida, imageOut, THREAD_NUM);

    // Guardar imagen combinada
    writeImage((char *)ruta_salida, imageOut);

    // Liberar memoria
    freeImage(imageOut);

    // Desmapear memoria compartida
    size_t shm_size = sizeof(BMP_Image) + imagen_compartida->header.imagesize;
    munmap(imagen_compartida, shm_size);

    shm_unlink(SHM_NAME);

    return 0;
}
