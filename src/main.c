#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"
#include "desenfocador.h"

#define THREAD_NUM 8

static void handle_err(const int error_code) {
  printError(error_code);
  exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
  if (argc != 3)
    handle_err(ARGUMENT_ERROR);

  FILE *const source = fopen(argv[1], "rb");
  if (!source)
    handle_err(FILE_ERROR);

  FILE *const dest = fopen(argv[2], "wb");
  if (!dest)
    handle_err(FILE_ERROR);

  BMP_Image *const imageIn = malloc(sizeof(BMP_Image));
  BMP_Image *const imageOut = malloc(sizeof(BMP_Image));
  if (!imageIn || !imageOut)
    handle_err(MEMORY_ERROR);

  readImage(source, imageIn);
  if (!checkBMPValid(&imageIn->header))
    handle_err(VALID_ERROR);

  transBMP(imageIn, imageOut);
  if (!checkBMPValid(&imageOut->header))
    handle_err(VALID_ERROR);

  applyParallel(imageIn, imageOut, THREAD_NUM);
  writeImage(argv[2], imageOut);

  freeImage(imageIn);
  freeImage(imageOut);

  fclose(source);
  fclose(dest);

  return EXIT_SUCCESS;
}
