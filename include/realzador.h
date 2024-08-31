#ifndef REALZADOR_H
#define REALZADOR_H

#include "bmp.h"

#define FILTER_SIZE 3

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
    BMP_Image *imageIn;
    BMP_Image *imageOut;
    int startRow;
    int endRow;
} realzador_parameters;

// RGB channel enum
typedef enum { RED, GREEN, BLUE } RGBChannel;

void apply_realce(BMP_Image *imageIn, BMP_Image *imageOut, int startRow, int endRow);
void applyRealceParallel(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads);
void *realceThreadWorker(void *args);

#endif // REALZADOR_H
