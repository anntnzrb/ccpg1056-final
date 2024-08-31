#ifndef DESENFOCADOR_H
#define DESENFOCADOR_H
#include "bmp.h"

#define FILTER_SIZE 3

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
  BMP_Image *imageIn;
  BMP_Image *imageOut;
  int startRow;
  int endRow;
} parameters;

// RGB channel enum
typedef enum { RED, GREEN, BLUE } RGBChannel;

int calcPixelVal(Pixel **imagePixels, int posX, int posY, int imgWidth,
                 int imgHeight, RGBChannel color);

void apply(BMP_Image *imageIn, BMP_Image *imageOut, int startRow, int endRow);
void applyParallel(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads);
void *filterThreadWorker(void *args);
#endif // DESENFOCADOR_H
