#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "filter_common.h"
#include "publicador.h"
#include "util.h"

#define FILENAME_MAX_SIZE 128

int image_fd = 0, output_fd = 0;
void *input_pixels_image = NULL, *output_pixels_image = NULL;

void
copy_image_data(void *dest, BMP_Image *image, int is_input) {
    // copiar los datos de la imagen a la memoria compartida
    size_t row_size = image->header.width_px * sizeof(Pixel);
    for (int i = 0; i < image->norm_height; i++) {
        if (is_input) {
            memcpy((char *)dest + i * row_size, image->pixels[i], row_size);
        } else {
            memcpy(image->pixels[i], (char *)dest + i * row_size, row_size);
        }
    }
}

int
create_shm(const char *name, int *fd, void **pixels_image, size_t shm_size) {
    // crear el espacio de memoria compartida
    *fd = shm_open(name, O_CREAT | O_TRUNC | O_RDWR, 0666);
    if (*fd == -1 || ftruncate(*fd, shm_size) != 0) {
        return -1;
    }

    // mapear la memoria compartida
    *pixels_image =
        mmap(0, shm_size, PROT_WRITE | PROT_READ, MAP_SHARED, *fd, 0);

    // si no se pudo mapear, eliminar el espacio de memoria compartida
    if (*pixels_image == NULL) {
        shm_unlink(name);
        return -1;
    }

    return 0;
}

pid_t
apply_filter(BMP_Image *img, int num_threads, const char *filter_name,
             int start_row, int end_row) {
    pid_t child = fork();
    if (child == -1) {
        return -1;
    }

    // hijo
    if (child == 0) {
        char args[5][16];
        snprintf(args[0], 16, "%d", start_row);
        snprintf(args[1], 16, "%d", end_row);
        snprintf(args[2], 16, "%d", img->norm_height);
        snprintf(args[3], 16, "%d", img->header.width_px);
        snprintf(args[4], 16, "%d", num_threads);
        execl(filter_name, filter_name, args[0], args[1], args[2], args[3],
              args[4], NULL);

        // no deberia llegar nunca aquí
        die("Exec falló");
    }

    return child;
}

int
process_image(const char *filename, BMP_Image *image, BMP_Image *new_image,
              int num_threads) {
    FILE *source = fopen(filename, "rb");
    if (!source || readImage(source, image) != 0 ||
        !checkBMPValid(&image->header)) {
        fprintf(stderr, "No se pudo leer la imagen o la imagen es inválida\n");
        if (source)
            fclose(source);
        return 1;
    }

    printf("IMAGEN DE ENTRADA:\n");
    printBMPHeader(&image->header);
    printBMPImage(image);

    // crear la memoria compartida para la imagen original y la imagen
    // procesada
    size_t shm_size =
        image->norm_height * image->header.width_px * sizeof(Pixel);
    if (create_shm(IMAGE_INPUT, &image_fd, &input_pixels_image, shm_size) !=
            0 ||
        create_shm(IMAGE_OUTPUT, &output_fd, &output_pixels_image, shm_size) !=
            0) {
        fprintf(stderr, "Error creando espacios de memoria compartida\n");
        fclose(source);
        return 1;
    }

    // copiar la imagen original a la memoria compartida
    copy_image_data(input_pixels_image, image, 1);

    // aplicar los filtros (desenfocador y realzador)
    pid_t filtro_desenfocador =
        apply_filter(image, num_threads, "desenfocador",
                     image->norm_height / 2, image->norm_height);
    pid_t filtro_realzador = apply_filter(image, num_threads, "realzador", 0,
                                          image->norm_height / 2 - 1);

    // esperar a que los filtros terminen
    int status;
    if (filtro_desenfocador == -1 || filtro_realzador == -1 ||
        waitpid(filtro_desenfocador, &status, 0) == -1 || !WIFEXITED(status) ||
        WEXITSTATUS(status) != 0 ||
        waitpid(filtro_realzador, &status, 0) == -1 || !WIFEXITED(status) ||
        WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Error procesando la imagen\n");
        fclose(source);
        return 1;
    }

    // copiar el header de la imagen original a la nueva imagen
    memcpy(&(new_image->header), &(image->header), sizeof(BMP_Header));
    if (image->header.bits_per_pixel == 24) {
        new_image->header.bits_per_pixel = 32;
        new_image->header.imagesize =
            image->norm_height * image->header.width_px * 4;
        new_image->header.size =
            new_image->header.imagesize + sizeof(BMP_Header);
    }
    new_image->bytes_per_pixel = new_image->header.bits_per_pixel / 8;
    new_image->norm_height = image->norm_height;

    // crear la memoria para la imagen procesada
    new_image->pixels = malloc(new_image->norm_height * sizeof(Pixel *));
    if (!new_image->pixels) {
        printError(MEMORY_ERROR);
        fclose(source);
        return 1;
    }

    // crear la memoria para la imagen procesada
    for (int i = 0; i < image->norm_height; i++) {
        new_image->pixels[i] = malloc(image->header.width_px * sizeof(Pixel));
        if (!new_image->pixels[i]) {
            for (int j = 0; j < i; j++)
                free(new_image->pixels[j]);
            free(new_image->pixels);
            printError(MEMORY_ERROR);
            fclose(source);
            return 1;
        }
    }

    // copiar la imagen procesada a la memoria compartida
    copy_image_data(output_pixels_image, new_image, 0);

    // escribir la imagen procesada en un archivo
    FILE *dest = fopen("outputs/filtered.bmp", "wb");
    if (!dest || writeImage(dest, new_image) != 0) {
        fprintf(stderr, "Error escribiendo la imagen\n");
        if (dest)
            fclose(dest);
        fclose(source);
        return 1;
    }

    // exito :)
    printf("=== Imagen procesada y guardada correctamente ===\n\n");

    // clean up todo
    fclose(source);
    fclose(dest);
    close(image_fd);
    close(output_fd);
    munmap(input_pixels_image, shm_size);
    munmap(output_pixels_image, shm_size);
    shm_unlink(IMAGE_INPUT);
    shm_unlink(IMAGE_OUTPUT);

    return 0;
}

int
main(int argc, char **argv) {
    if (argc != 2 || (atoi(argv[1])) <= 0) {
        die("Uso: %s <hilos>", argv[0]);
    }

    int num_threads = atoi(argv[1]);
    char filename[FILENAME_MAX_SIZE] = {0};

    while (1) {
        while (1) {
            printf("Ingrese la ruta de la imagen (o 'q' para salir): ");
            if (fgets(filename, sizeof(filename), stdin)) {
                filename[strcspn(filename, "\n")] = 0;
                if (strcmp(filename, "q") == 0) {
                    printf("Saliendo del programa...\n");
                    return 0;
                }
                FILE *sc = fopen(filename, "rb");
                if (sc) {
                    fclose(sc);
                    break;
                }
            }
            fprintf(stderr, "Ingrese una ruta válida...\n");
        }

        // procesar img
        BMP_Image image = {0}, new_image = {0};
        if (process_image(filename, &image, &new_image, num_threads) != 0) {
            fprintf(stderr, "Error procesando la imagen %s\n", filename);
        }

        // clean up
        for (int i = 0; i < image.norm_height; i++) {
            free(image.pixels[i]);
            free(new_image.pixels[i]);
        }
        free(image.pixels);
        free(new_image.pixels);
    }

    return 0;
}