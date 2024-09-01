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

  // Asignar memoria para hilos y parámetros
  pthread_t *threads = malloc(numThreads * sizeof(pthread_t));
  filter_parameters *params = malloc(numThreads * sizeof(filter_parameters));

  // Calcular distribución de filas entre hilos
  const int height_px = imageIn->norm_height;
  const int rowsPerThread = height_px / numThreads;
  int remainingRows = height_px % numThreads;
  int startRow = 0;

  for (int i = 0; i < numThreads; i++) {
    int endRow = startRow + rowsPerThread + (remainingRows > 0 ? 1 : 0);
    if (remainingRows > 0) {
      remainingRows--;
    }

    // Configurar parámetros para cada hilo
    params[i] = (filter_parameters){.imageIn = imageIn,
                                    .imageOut = imageOut,
                                    .startRow = startRow,
                                    .endRow = endRow,
                                    .applyFilter = applyFilter};

    // Crear hilo y pasarle la función y parámetros
    pthread_create(&threads[i], NULL, filterThreadWorker, &params[i]);

    startRow = endRow;
  }

  // Esperar a que todos los hilos terminen
  for (int i = 0; i < numThreads; i++) {
    pthread_join(threads[i], NULL);
  }

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
