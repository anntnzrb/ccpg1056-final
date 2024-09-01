#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "common_filter.h"
#include "desenfocador.h"

// 2D matrix que representa el filtro que se aplica a la imagen
int boxFilter[FILTER_SIZE][FILTER_SIZE] = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};

int calcPixelVal(Pixel **imagePixels, int posX, int posY, int imgWidth,
                 int imgHeight, RGBChannel color) {
  int pixelValue = 0;
  // iterar sobre el filtro (matriz 3x3)
  for (int offsetX = 0; offsetX < FILTER_SIZE; offsetX++) {
    for (int offsetY = 0; offsetY < FILTER_SIZE; offsetY++) {
      int adjustedX = posX - FILTER_SIZE / 2 + offsetX;
      int adjustedY = posY - FILTER_SIZE / 2 + offsetY;

      // asegurar que X e Y estén dentro de los límites
      adjustedX = MAX(0, MIN(adjustedX, imgHeight - 1));
      adjustedY = MAX(0, MIN(adjustedY, imgWidth - 1));

      // sumar valores de píxeles basados en el canal de color
      switch (color) {
      case RED:
        pixelValue +=
            imagePixels[adjustedX][adjustedY].red * boxFilter[offsetX][offsetY];
        break;
      case GREEN:
        pixelValue += imagePixels[adjustedX][adjustedY].green *
                      boxFilter[offsetX][offsetY];
        break;
      case BLUE:
        pixelValue += imagePixels[adjustedX][adjustedY].blue *
                      boxFilter[offsetX][offsetY];
        break;
      }
    }
  }

  return pixelValue;
}

void apply_desenfocador(BMP_Image *imageIn, BMP_Image *imageOut, int startRow, int endRow) {
  int middleRow = imageIn->norm_height / 2;
  
  for (int row = startRow; row < endRow && row < middleRow; row++) {
    for (int col = 0; col < imageIn->header.width_px; col++) {
      // aplicar filtro de desenfoque a la mitad superior
      imageOut->pixels[row][col].red =
          calcPixelVal(imageIn->pixels, row, col, imageIn->header.width_px,
                       imageIn->norm_height, RED) /
          (FILTER_SIZE * FILTER_SIZE);
      imageOut->pixels[row][col].green =
          calcPixelVal(imageIn->pixels, row, col, imageIn->header.width_px,
                       imageIn->norm_height, GREEN) /
          (FILTER_SIZE * FILTER_SIZE);
      imageOut->pixels[row][col].blue =
          calcPixelVal(imageIn->pixels, row, col, imageIn->header.width_px,
                       imageIn->norm_height, BLUE) /
          (FILTER_SIZE * FILTER_SIZE);
      imageOut->pixels[row][col].alpha = 255;
    }
  }
}

void apply_desenfocador_parallel(BMP_Image *imageIn, BMP_Image *imageOut,
                                  int numThreads) {
  apply_filter_parallel(imageIn, imageOut, numThreads, apply_desenfocador);
}
