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
    // init struct de BMP_Image
    if (!dataImage || !fptr || !memset(dataImage, 0, sizeof(BMP_Image)) ||
        fread(&dataImage->header, HEADER_SIZE, 1, fptr) != 1) {
        return NULL;
    }

    // calc alto y ancho
    int width_px = dataImage->header.width_px;
    int height_px = abs(dataImage->header.height_px);

    // check si es que la img está al revés
    dataImage->is_bottom_up = dataImage->header.height_px > 0;
    dataImage->bytes_per_pixel = dataImage->header.bits_per_pixel / BITS;
    dataImage->norm_height = height_px;
    dataImage->pixels = malloc(height_px * sizeof(Pixel *));

    if (!dataImage->pixels) {
        return NULL;
    }

    // recorrer img y asignar mem para cada pixel
    for (int i = 0; i < height_px; i++) {
        dataImage->pixels[i] = malloc(width_px * sizeof(Pixel));
        if (!dataImage->pixels[i]) {
            for (int j = 0; j < i; j++) {
                free(dataImage->pixels[j]);
            }
            free(dataImage->pixels);
            return NULL;
        }
    }

    return dataImage;
}

int
readImageData(FILE *srcFile, BMP_Image *image) {
    // calc padding
    int padding =
        (4 - (image->header.width_px * image->bytes_per_pixel) % 4) % 4;

    // recorrer img y leer pixeles
    for (int i = 0; i < image->norm_height; i++) {
        for (int j = 0; j < image->header.width_px; j++) {
            Pixel *pixel = &image->pixels[i][j];
            if (fread(pixel, image->bytes_per_pixel, 1, srcFile) != 1) {
                return 1;
            }

            uint8_t temp = pixel->red;
            pixel->red = pixel->blue;
            pixel->blue = temp;
            if (image->bytes_per_pixel == 3) {
                pixel->alpha = 255;
            }
        }
        fseek(srcFile, padding, SEEK_CUR);
    }

    return 0;
}

int
readImage(FILE *srcFile, BMP_Image *dataImage) {
    return createBMPImage(srcFile, dataImage) &&
           readImageData(srcFile, dataImage);
}

int
writeImage(FILE *destFile, BMP_Image *dataImage) {
    // escribir header
    if (fwrite(&dataImage->header, HEADER_SIZE, 1, destFile) != 1)
        return 1;

    // calc padding
    int padding =
        (4 - (dataImage->header.width_px * dataImage->bytes_per_pixel) % 4) %
        4;
    uint8_t padBytes[3] = {0};

    // recorrer img y escribir pixeles
    for (int i = 0; i < dataImage->norm_height; i++) {
        for (int j = 0; j < dataImage->header.width_px; j++) {
            Pixel pixel = dataImage->pixels[i][j];

            uint8_t temp = pixel.red;
            pixel.red = pixel.blue;
            pixel.blue = temp;
            if (fwrite(&pixel, dataImage->bytes_per_pixel, 1, destFile) != 1) {
                return 1;
            }
        }

        // agregar padding
        if (padding > 0) {
            fwrite(padBytes, padding, 1, destFile);
        }
    }

    return 0;
}

int
modify_new_image(BMP_Image *image, BMP_Image *new_image) {
    // cp header
    memcpy(&new_image->header, &image->header, sizeof(BMP_Header));

    // si es 24 bits, set a 32
    if (image->header.bits_per_pixel == 24) {
        new_image->header.bits_per_pixel = 32;
        new_image->header.imagesize =
            image->norm_height * image->header.width_px * 4;
        new_image->header.size =
            new_image->header.imagesize + sizeof(BMP_Header);
        new_image->bytes_per_pixel = 4;
    } else {
        new_image->bytes_per_pixel = image->bytes_per_pixel;
    }

    // asignar mem para cada pixel
    new_image->norm_height = image->norm_height;
    new_image->pixels = malloc(new_image->norm_height * sizeof(Pixel *));
    if (!new_image->pixels)
        return 1;

    // recorrer img y asignar mem para cada pixel
    for (int i = 0; i < image->norm_height; i++) {
        new_image->pixels[i] = malloc(image->header.width_px * sizeof(Pixel));
        if (!new_image->pixels[i]) {
            for (int j = 0; j < i; j++)
                free(new_image->pixels[j]);
            free(new_image->pixels);
            return 1;
        }

        // recorrer img y copiar pixeles
        for (int j = 0; j < image->header.width_px; j++) {
            new_image->pixels[i][j] = image->pixels[i][j];
            new_image->pixels[i][j].alpha = 255;
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
           (header->bits_per_pixel == 24 || header->bits_per_pixel == 32) &&
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
