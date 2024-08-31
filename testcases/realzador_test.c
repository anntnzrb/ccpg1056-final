#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bmp.h"
#include "publicador.h"
#include "realzador.h"

#define THREAD_NUM 8
#define SHM_NAME "/bmp_imagen_compartida"

static void handle_err(const int error_code) {
  printError(error_code);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  if (argc != 3)
    handle_err(ARGUMENT_ERROR);

  BMP_Image *imagen_compartida;
  if (publicar_imagen(argv[1], &imagen_compartida) != 0) {
    handle_err(FILE_ERROR);
  }

  FILE *const dest = fopen(argv[2], "wb");
  if (!dest)
    handle_err(FILE_ERROR);

  BMP_Image *const imageOut = malloc(sizeof(BMP_Image));
  if (!imageOut)
    handle_err(MEMORY_ERROR);

  if (!checkBMPValid(&imagen_compartida->header))
    handle_err(VALID_ERROR);

  transBMP(imagen_compartida, imageOut);
  if (!checkBMPValid(&imageOut->header))
    handle_err(VALID_ERROR);

  apply_realzador_parallel(imagen_compartida, imageOut, THREAD_NUM);
  writeImage(argv[2], imageOut);

  freeImage(imageOut);

  size_t shm_size = sizeof(BMP_Image) + imagen_compartida->header.imagesize;
  munmap(imagen_compartida, shm_size);

  shm_unlink(SHM_NAME);

  fclose(dest);

  return EXIT_SUCCESS;
}