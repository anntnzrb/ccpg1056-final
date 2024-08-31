#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"
#include "desenfocador.h"

// 2D array representing the filter that will be applied to the img
int boxFilter[FILTER_SIZE][FILTER_SIZE] = {{1, 1, 1}, {1, 1, 1}, {1, 1, 1}};

// calc val of a pixel in the output image
int calcPixelVal(Pixel **imagePixels, int posX, int posY, int imgWidth,
                 int imgHeight, RGBChannel color) {
  int pixelValue = 0;
  // iterate over the filter
  for (int offsetX = 0; offsetX < FILTER_SIZE; offsetX++) {
    for (int offsetY = 0; offsetY < FILTER_SIZE; offsetY++) {
      int adjustedX = posX - FILTER_SIZE / 2 + offsetX;
      int adjustedY = posY - FILTER_SIZE / 2 + offsetY;

      // ensure X and Y are within bounds
      adjustedX = MAX(0, MIN(adjustedX, imgHeight - 1));
      adjustedY = MAX(0, MIN(adjustedY, imgWidth - 1));

      // accum pixel vals based off color channel
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

void apply(BMP_Image *imageIn, BMP_Image *imageOut, int startRow, int endRow) {
  for (int row = startRow; row < endRow; row++) {
    for (int col = 0; col < imageIn->header.width_px; col++) {
      // calc new pixel vals for each color channel
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

void applyParallel(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads) {
  printf("Applying filter with %d threads\n", numThreads);

  pthread_t *threads = malloc(numThreads * sizeof(pthread_t));
  parameters *params = malloc(numThreads * sizeof(parameters));

  const int height_px = imageIn->norm_height;
  const int rowsPerThread = height_px / numThreads;
  int remainingRows = height_px % numThreads;
  int startRow = 0;
  int endRow;

  // div img rows among the threads
  for (int i = 0; i < numThreads; i++) {
    endRow = startRow + rowsPerThread;

    // distribute remaining rows
    if (remainingRows > 0) {
      endRow = endRow + 1;
      remainingRows = remainingRows - 1;
    }

    // set params for each thread
    params[i] = (parameters){.imageIn = imageIn,
                             .imageOut = imageOut,
                             .startRow = startRow,
                             .endRow = endRow};

    pthread_create(&threads[i], NULL, filterThreadWorker, &params[i]);

    startRow = endRow;
  }

  // wait for all threads to complete
  for (int i = 0; i < numThreads; i++) {
    pthread_join(threads[i], NULL);
  }

  free(threads);
  free(params);
}

void *filterThreadWorker(void *args) {
  parameters *params = (parameters *)args;
  apply(params->imageIn, params->imageOut, params->startRow, params->endRow);

  return NULL;
}
