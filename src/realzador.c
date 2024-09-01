#include "realzador.h"
#include "common_filter.h"
#include <stdlib.h>

// filtro para detectar bordes en el eje X e Y
int edgeFilterX[FILTER_SIZE][FILTER_SIZE] = {
    {-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
int edgeFilterY[FILTER_SIZE][FILTER_SIZE] = {
    {-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};

int calcEdgePixelVal(Pixel **imagePixels, int posX, int posY, int imgWidth,
                     int imgHeight, RGBChannel color) {
  int pixelValueX = 0;
  int pixelValueY = 0;

  for (int offsetX = 0; offsetX < FILTER_SIZE; offsetX++) {
    for (int offsetY = 0; offsetY < FILTER_SIZE; offsetY++) {
      int adjustedX = posX - FILTER_SIZE / 2 + offsetX;
      int adjustedY = posY - FILTER_SIZE / 2 + offsetY;

      // asegurar que las coords estén dentro de la img
      adjustedX = MAX(0, MIN(adjustedX, imgHeight - 1));
      adjustedY = MAX(0, MIN(adjustedY, imgWidth - 1));

      int pixelValue;
      // seleccionar valor del pixel según el canal de color
      switch (color) {
      case RED:
        pixelValue = imagePixels[adjustedX][adjustedY].red;
        break;
      case GREEN:
        pixelValue = imagePixels[adjustedX][adjustedY].green;
        break;
      case BLUE:
        pixelValue = imagePixels[adjustedX][adjustedY].blue;
        break;
      }

      // aplicar filtros de borde en X e Y
      pixelValueX += pixelValue * edgeFilterX[offsetX][offsetY];
      pixelValueY += pixelValue * edgeFilterY[offsetX][offsetY];
    }
  }

  // calcular y limitar valor final del pixel
  // abs para asegurar que el valor sea positivo
  // MAX y MIN para limitar el valor entre 0 y 255
  return MIN(255, MAX(0, abs(pixelValueX) + abs(pixelValueY)));
}

void apply_realzador(BMP_Image *imageIn, BMP_Image *imageOut, int startRow, int endRow) {
  int middleRow = imageIn->norm_height / 2;
  
  for (int row = MAX(startRow, middleRow); row < endRow; row++) {
    for (int col = 0; col < imageIn->header.width_px; col++) {
      // aplicar filtro de realce a la mitad inferior
      imageOut->pixels[row][col].red = calcEdgePixelVal(
          imageIn->pixels, row, col, imageIn->header.width_px,
          imageIn->norm_height, RED);
      imageOut->pixels[row][col].green = calcEdgePixelVal(
          imageIn->pixels, row, col, imageIn->header.width_px,
          imageIn->norm_height, GREEN);
      imageOut->pixels[row][col].blue = calcEdgePixelVal(
          imageIn->pixels, row, col, imageIn->header.width_px,
          imageIn->norm_height, BLUE);
      imageOut->pixels[row][col].alpha = 255;
    }
  }
}

void apply_realzador_parallel(BMP_Image *imageIn, BMP_Image *imageOut,
                         int numThreads) {
  apply_filter_parallel(imageIn, imageOut, numThreads, apply_realzador);
}
