#ifndef COMMON_FILTER_H
#define COMMON_FILTER_H

#include "bmp.h"

// filtro de  3x3
#define FILTER_SIZE 3

// funciones/macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// struct para pasar params
typedef struct {
  BMP_Image *imageIn;
  BMP_Image *imageOut;
  int startRow;
  int endRow;
  void (*applyFilter)(BMP_Image *, BMP_Image *, int, int);
} filter_parameters;

// enum de RGB
typedef enum { RED, GREEN, BLUE } RGBChannel;

// fn para aplicar filtro en paralelo
void apply_filter_parallel(BMP_Image *imageIn, BMP_Image *imageOut,
                         int numThreads,
                         void (*applyFilter)(BMP_Image *, BMP_Image *, int,
                                             int));

// fn para cada thread independiente
void *filterThreadWorker(void *args);

#endif // COMMON_FILTER_H
