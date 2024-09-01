#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>

#include "realzador.h"
#include "common_filter.h"

#define SHM_NAME "/bmp_imagen_compartida"
#define DEBUG_REALZADOR 1

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
      // aplicar filtro de realzador a la mitad inferior
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

int main() {
    #if DEBUG_REALZADOR
    printf("Realzador: Iniciando\n");
    #endif

    // Abrir la memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Realzador: Error al abrir la memoria compartida");
        exit(1);
    }

    #if DEBUG_REALZADOR
    printf("Realzador: Memoria compartida abierta\n");
    #endif

    // Obtener el tamaño de la memoria compartida
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("Realzador: Error al obtener el tamaño de la memoria compartida");
        close(shm_fd);
        exit(1);
    }

    #if DEBUG_REALZADOR
    printf("Realzador: Tamaño de memoria compartida obtenido: %ld bytes\n", shm_stat.st_size);
    #endif

    // Mapear la memoria compartida
    BMP_Image *imagen_compartida = mmap(NULL, shm_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (imagen_compartida == MAP_FAILED) {
        perror("Realzador: Error al mapear la memoria compartida");
        close(shm_fd);
        exit(1);
    }

    #if DEBUG_REALZADOR
    printf("Realzador: Memoria compartida mapeada\n");
    #endif

    // Aplicar el filtro de realce
    apply_realzador_parallel(imagen_compartida, imagen_compartida, 4);

    #if DEBUG_REALZADOR
    printf("Realzador: Filtro aplicado\n");
    #endif

    // Limpiar
    if (munmap(imagen_compartida, shm_stat.st_size) == -1) {
        perror("Realzador: Error al desmapear la memoria compartida");
    }
    close(shm_fd);

    #if DEBUG_REALZADOR
    printf("Realzador: Finalizado\n");
    #endif

    return 0;
}
