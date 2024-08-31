#ifndef REALZADOR_H
#define REALZADOR_H

#include "common_filter.h"

// fn para aplicar realzador a una porción de la imagen
void apply_realzador(BMP_Image *imageIn, BMP_Image *imageOut, int startRow,
                  int endRow);

// fn para aplicar realzador en paralelo
void apply_realzador_parallel(BMP_Image *imageIn, BMP_Image *imageOut,
                         int numThreads);

#endif // REALZADOR_H
