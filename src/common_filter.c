#include "common_filter.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_COMMON_FILTER 1

void apply_filter_parallel(BMP_Image *imageIn, BMP_Image *imageOut,
                         int numThreads,
                         void (*applyFilter)(BMP_Image *, BMP_Image *, int,
                                             int)) {
#if DEBUG_COMMON_FILTER
  printf("Aplicando filtro con %d threads\n", numThreads);
#endif

  // asignar mem para threads y params
  pthread_t *threads = malloc(numThreads * sizeof(pthread_t));
  filter_parameters *params = malloc(numThreads * sizeof(filter_parameters));

  // calc distribucion de filas entre threads
  const int height_px = imageIn->norm_height;
  const int rowsPerThread = height_px / numThreads;
  int remainingRows = height_px % numThreads;
  int startRow = 0;
  int endRow;

  for (int i = 0; i < numThreads; i++) {
    endRow = startRow + rowsPerThread;

    // si hay resto, se le suma 1 a los ultimos threads
    if (remainingRows > 0) {
      endRow++;
      remainingRows--;
    }

    // settear params para cada thread
    params[i] = (filter_parameters){.imageIn = imageIn,
                                    .imageOut = imageOut,
                                    .startRow = startRow,
                                    .endRow = endRow,
                                    .applyFilter = applyFilter};

    // crear thread y pasarle la fn y params
    pthread_create(&threads[i], NULL, filterThreadWorker, &params[i]);

    startRow = endRow;
  }

  // esperar demas threads
  for (int i = 0; i < numThreads; i++) {
    pthread_join(threads[i], NULL);
  }

  // free mem
  free(threads);
  free(params);
}

void *filterThreadWorker(void *args) {
  filter_parameters *params = (filter_parameters *)args;
  // aplica el filtro (desenfocador, realzador) correspondiente
  params->applyFilter(params->imageIn, params->imageOut, params->startRow,
                      params->endRow);

  return NULL;
}
