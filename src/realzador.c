#include <stdlib.h>
#include <string.h>

#include "filter_common.h"
#include "util.h"

int filter[FILTER_SIZE][FILTER_SIZE] = {
    {-1, -1, -1}, {-1, 8, -1}, {-1, -1, -1}};

int
main(int argc, char **argv) {
    if (argc != 6) {
        die("Uso: %s <fila_inicio> <fila_fin> <alto> <ancho> <hilos>",
            argv[0]);
    }

    int fila_inicio = atoi(argv[1]);
    int fila_fin = atoi(argv[2]);
    int alto = atoi(argv[3]);
    int ancho = atoi(argv[4]);
    int hilos = atoi(argv[5]);

    Pixel *image_data =
        (Pixel *)get_shared_image(IMAGE_INPUT, alto * ancho * sizeof(Pixel));
    if (image_data == NULL) {
        die("No se pudo recuperar la imagen compartida de entrada");
    }

    Pixel *output_image_data =
        (Pixel *)get_shared_image(IMAGE_OUTPUT, alto * ancho * sizeof(Pixel));
    if (output_image_data == NULL) {
        die("No se pudo recuperar la imagen compartida de salida");
    }

    apply_filter_parallel(image_data, output_image_data, alto, ancho,
                          fila_inicio, fila_fin, hilos, filter);

    return 0;
}
