#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp.h"

void
printError(int error) {
    switch (error) {
    case ARGUMENT_ERROR:
        printf("Uso: <programa> <hilos>\n");
        break;
    case FILE_ERROR:
        printf("No se pudo abrir el archivo!\n");
        break;
    case MEMORY_ERROR:
        printf("No se pudo asignar memoria!\n");
        break;
    case VALID_ERROR:
        printf("El archivo BMP no es valido!\n");
        break;
    default:
        break;
    }
}

BMP_Image *
createBMPImage(FILE *fptr, BMP_Image *dataImage) {
    if (memset(dataImage, 0, sizeof(BMP_Image)) == NULL ||
        fread(&dataImage->header, HEADER_SIZE, 1, fptr) == 0)
        return NULL;

    int width_px = dataImage->header.width_px;
    int height_px = abs(dataImage->header.height_px);

    dataImage->bytes_per_pixel = dataImage->header.bits_per_pixel / BITS;
    dataImage->norm_height = height_px;

    dataImage->pixels = malloc(height_px * sizeof(Pixel *));
    if (dataImage->pixels == NULL)
        return NULL;

    for (int i = 0; i < height_px; i++) {
        if ((dataImage->pixels[i] = malloc(width_px * sizeof(Pixel))) ==
            NULL) {
            for (int j = 0; j < i; j++)
                free(dataImage->pixels[j]);
            free(dataImage->pixels);
            return NULL;
        }
    }

    return dataImage;
}

int
readImageData(FILE *srcFile, BMP_Image *image, int dataSize) {
    for (int i = 0; i < image->norm_height; i++) {
        for (int j = 0; j < image->header.width_px; j++) {
            if (fread(&image->pixels[i][j], dataSize, 1, srcFile) == 0) {
                fprintf(stderr,
                        "Pixel (%d,%d) no se pudo leer desde el archivo\n", i,
                        j);
                return 1;
            }
        }
    }

    return 0;
}

int
readImage(FILE *srcFile, BMP_Image *dataImage) {
    if (createBMPImage(srcFile, dataImage) == NULL)
        return 1;

    return readImageData(srcFile, dataImage, dataImage->bytes_per_pixel);
}

int
writeImage(FILE *destFile, BMP_Image *dataImage) {
    if (fwrite(&dataImage->header, HEADER_SIZE, 1, destFile) == 0)
        return 1;

    for (int i = 0; i < dataImage->norm_height; i++) {
        if (fwrite(dataImage->pixels[i], sizeof(Pixel),
                   dataImage->header.width_px, destFile) == 0) {
            fprintf(stderr,
                    "Fila (%d) no se pudo escribir en la imagen de salida\n",
                    i);
            return 1;
        }
    }
    return 0;
}

int
modify_new_image(BMP_Image *image, BMP_Image *new_image) {
    if (memcpy(&new_image->header, &image->header, sizeof(BMP_Header)) == NULL)
        return 1;

    if (image->header.bits_per_pixel == 24) {
        new_image->header.bits_per_pixel = 32;
        new_image->header.imagesize =
            image->norm_height * image->header.width_px * 4;
        new_image->header.size =
            new_image->header.imagesize + sizeof(BMP_Header);
        new_image->bytes_per_pixel = 4;
    } else if (image->header.bits_per_pixel == 32) {
        new_image->bytes_per_pixel = image->bytes_per_pixel;
    }

    new_image->norm_height = image->norm_height;
    new_image->pixels = malloc(new_image->norm_height * sizeof(Pixel *));
    if (new_image->pixels == NULL)
        return 1;

    for (int i = 0; i < image->norm_height; i++) {
        new_image->pixels[i] = malloc(image->header.width_px * sizeof(Pixel));
        if (new_image->pixels[i] == NULL) {
            for (int j = 0; j < i; j++)
                free(new_image->pixels[j]);
            free(new_image->pixels);
            return 1;
        }
    }

    return 0;
}

void
freeImage(BMP_Image *image) {
    if (!image)
        return;

    for (int i = 0; i < image->norm_height; i++) {
        free(image->pixels[i]);
    }
    free(image->pixels);
    free(image);
}

int
checkBMPValid(BMP_Header *header) {
    return header->type == 0x4d42 &&
           ((header->bits_per_pixel & 31) == 24 ||
            header->bits_per_pixel == 32) &&
           header->planes == 1 && header->compression == 0;
}

void
printBMPHeader(BMP_Header *header) {
    printf("Tipo de archivo (debe ser 0x4d42): %x\n"
           "Tamaño del archivo: %d\n"
           "Desplazamiento al inicio de los datos de la imagen: %d\n"
           "Tamaño del header: %d\n"
           "Ancho en pixeles: %d\n"
           "Alto en pixeles: %d\n"
           "Planes: %d\n"
           "Bits por pixel: %d\n",
           header->type, header->size, header->offset, header->header_size,
           header->width_px, header->height_px, header->planes,
           header->bits_per_pixel);
}

void
printBMPImage(BMP_Image *image) {
    printf("Tamaño de los datos: %ld\n"
           "Tamaño normalizado de la altura: %d\n"
           "Bytes por pixel: %d\n",
           sizeof(image->pixels), image->norm_height, image->bytes_per_pixel);
}
