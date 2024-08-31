#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"
#include "realzador.h"

// Edge detection filter (Sobel operator)
int edgeFilterX[FILTER_SIZE][FILTER_SIZE] = {{-1, 0, 1},
                                             {-2, 0, 2},
                                             {-1, 0, 1}};

int edgeFilterY[FILTER_SIZE][FILTER_SIZE] = {{-1, -2, -1},
                                             {0, 0, 0},
                                             {1, 2, 1}};

// Calculate value of a pixel in the output image
int calcEdgePixelVal(Pixel **imagePixels, int posX, int posY, int imgWidth,
                     int imgHeight, RGBChannel color) {
    int pixelValueX = 0;
    int pixelValueY = 0;

    for (int offsetX = 0; offsetX < FILTER_SIZE; offsetX++) {
        for (int offsetY = 0; offsetY < FILTER_SIZE; offsetY++) {
            int adjustedX = posX - FILTER_SIZE / 2 + offsetX;
            int adjustedY = posY - FILTER_SIZE / 2 + offsetY;

            adjustedX = MAX(0, MIN(adjustedX, imgHeight - 1));
            adjustedY = MAX(0, MIN(adjustedY, imgWidth - 1));

            int pixelValue;
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

            pixelValueX += pixelValue * edgeFilterX[offsetX][offsetY];
            pixelValueY += pixelValue * edgeFilterY[offsetX][offsetY];
        }
    }

    return MIN(255, MAX(0, abs(pixelValueX) + abs(pixelValueY)));
}

void apply_realce(BMP_Image *imageIn, BMP_Image *imageOut, int startRow, int endRow) {
    int halfHeight = imageIn->norm_height / 2;
    
    for (int row = startRow; row < endRow; row++) {
        for (int col = 0; col < imageIn->header.width_px; col++) {
            if (row >= halfHeight) {  // Only process the second half of the image
                imageOut->pixels[row][col].red =
                    calcEdgePixelVal(imageIn->pixels, row, col, imageIn->header.width_px,
                                     imageIn->norm_height, RED);
                imageOut->pixels[row][col].green =
                    calcEdgePixelVal(imageIn->pixels, row, col, imageIn->header.width_px,
                                     imageIn->norm_height, GREEN);
                imageOut->pixels[row][col].blue =
                    calcEdgePixelVal(imageIn->pixels, row, col, imageIn->header.width_px,
                                     imageIn->norm_height, BLUE);
                imageOut->pixels[row][col].alpha = 255;
            } else {
                // Copy the first half of the image without changes
                imageOut->pixels[row][col] = imageIn->pixels[row][col];
            }
        }
    }
}

void applyRealceParallel(BMP_Image *imageIn, BMP_Image *imageOut, int numThreads) {
    printf("Applying edge enhancement filter with %d threads\n", numThreads);

    pthread_t *threads = malloc(numThreads * sizeof(pthread_t));
    realzador_parameters *params = malloc(numThreads * sizeof(realzador_parameters));

    const int height_px = imageIn->norm_height;
    const int rowsPerThread = height_px / numThreads;
    int remainingRows = height_px % numThreads;
    int startRow = 0;
    int endRow;

    for (int i = 0; i < numThreads; i++) {
        endRow = startRow + rowsPerThread;

        if (remainingRows > 0) {
            endRow++;
            remainingRows--;
        }

        params[i] = (realzador_parameters){
            .imageIn = imageIn,
            .imageOut = imageOut,
            .startRow = startRow,
            .endRow = endRow
        };

        pthread_create(&threads[i], NULL, realceThreadWorker, &params[i]);

        startRow = endRow;
    }

    for (int i = 0; i < numThreads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(params);
}

void *realceThreadWorker(void *args) {
    realzador_parameters *params = (realzador_parameters *)args;
    apply_realce(params->imageIn, params->imageOut, params->startRow, params->endRow);
    return NULL;
}
