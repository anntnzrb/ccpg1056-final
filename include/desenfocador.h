#ifndef DESENFOCADOR_H
#define DESENFOCADOR_H

#include "common_filter.h"

// calc valor de píxel para un canal de color
int calcPixelVal(Pixel **imagePixels, int posX, int posY, int imgWidth,
                 int imgHeight, RGBChannel color);

// aplica filtro de desenfoque (blur) a una porción de imagen
void apply_desenfocador(BMP_Image *imageIn, BMP_Image *imageOut, int startRow,
                         int endRow);

// aplica filtro de desenfoque en paralelo
void apply_desenfocador_parallel(BMP_Image *imageIn, BMP_Image *imageOut,
                                  int numThreads);

#endif // DESENFOCADOR_H
