#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bmp.h"

/* USE THIS FUNCTION TO PRINT ERROR MESSAGES

   DO NOT MODIFY THIS FUNCTION

*/
void printError(int error) {
  switch (error) {
  case ARGUMENT_ERROR:
    printf("Usage: ./<filter> <source> <destination>\n");
    break;
  case FILE_ERROR:
    printf("Unable to open file!\n");
    break;
  case MEMORY_ERROR:
    printf("Unable to allocate memory!\n");
    break;
  case VALID_ERROR:
    printf("BMP file not valid!\n");
    break;
  default:
    break;
  }
}

/* The input argument is the source file pointer. The function will first
 * construct a BMP_Image image by allocating memory to it. Then the function
 * read the header from source image to the image's header. Compute data size,
 * width, height, and bytes_per_pixel of the image and stores them as image's
 * attributes. Finally, allocate menory for image's data according to the image
 * size. Return image;
 */
BMP_Image *createBMPImage(FILE *fptr) {
  BMP_Image *image = malloc(sizeof(BMP_Image));
  if (image == NULL) {
    return NULL;
  }

  if (fread(&(image->header), sizeof(BMP_Header), 1, fptr) != 1) {
    free(image);
    return NULL;
  }

  int width_px = image->header.width_px;
  int height_px = abs(image->header.height_px);

  image->bytes_per_pixel = image->header.bits_per_pixel / 8;
  image->norm_height = height_px;

  image->pixels = malloc(height_px * sizeof(Pixel *));
  if (image->pixels == NULL) {
    free(image);
    return NULL;
  }

  for (int i = 0; i < height_px; i++) {
    image->pixels[i] = malloc(width_px * sizeof(Pixel));
    if (image->pixels[i] == NULL) {
      for (int j = 0; j < i; j++) {
        free(image->pixels[j]);
      }

      free(image->pixels);
      free(image);
      return NULL;
    }
  }

  return image;
}

/* The input arguments are the source file pointer, the image data pointer, and
 * the size of image data. The functions reads data from the source into the
 * image data matriz of pixels.
 */
void readImageData(FILE *srcFile, BMP_Image *image, int dataSize) {
  for (int i = 0; i < image->norm_height; i++) {
    for (int j = 0; j < image->header.width_px; j++) {
      if (fread(&image->pixels[i][j], dataSize, 1, srcFile) != 1) {
        printError(MEMORY_ERROR);
        exit(EXIT_FAILURE);
      }
    }
  }
}

/* The input arguments are the pointer of the binary file, and the image data
 * pointer. The functions open the source file and call to CreateBMPImage to
 * load de data image.
 */
void readImage(FILE *srcFile, BMP_Image *dataImage) {
  BMP_Image *image = createBMPImage(srcFile);
  if (image == NULL) {
    printError(MEMORY_ERROR);
    exit(EXIT_FAILURE);
  }

  *dataImage = *image;

  free(image);

  readImageData(srcFile, dataImage, dataImage->bytes_per_pixel);
}

/* The input arguments are the destination file name, and BMP_Image pointer.
 * The function write the header and image data into the destination file.
 */
void writeImage(char *destFileName, BMP_Image *dataImage) {
  FILE *destFile = fopen(destFileName, "wb");
  if (destFile == NULL) {
    printError(FILE_ERROR);
    exit(EXIT_FAILURE);
  }

  if (fwrite(&(dataImage->header), sizeof(BMP_Header), 1, destFile) != 1) {
    printError(MEMORY_ERROR);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < dataImage->norm_height; i++) {
    if (fwrite(dataImage->pixels[i], sizeof(Pixel), dataImage->header.width_px,
               destFile) != (size_t)dataImage->header.width_px) {
      printError(MEMORY_ERROR);
      exit(EXIT_FAILURE);
    }
  }

  fclose(destFile);
}

/* The input argument is the BMP_Image pointer. The function frees memory of
 * the BMP_Image.
 */
void freeImage(BMP_Image *image) {
  for (int i = 0; i < image->norm_height; i++) {
    free(image->pixels[i]);
  }

  free(image->pixels);
  free(image);
}

/* Transforms (modifies) a BMP image, creating a new image based off an
 * existing one
 */
void transBMP(BMP_Image *image, BMP_Image *new_image) {
  memcpy(&(new_image->header), &(image->header), sizeof(BMP_Header));

  if (image->header.bits_per_pixel == 24) {
    new_image->header.bits_per_pixel = 32;
    new_image->header.imagesize =
        image->norm_height * image->header.width_px * 4;
    new_image->header.size = new_image->header.imagesize + sizeof(BMP_Header);
    new_image->bytes_per_pixel = 4;
  } else if (image->header.bits_per_pixel == 32) {
    new_image->bytes_per_pixel = image->bytes_per_pixel;
  }

  new_image->norm_height = image->norm_height;

  new_image->pixels = malloc(new_image->norm_height * sizeof(Pixel *));
  if (new_image->pixels == NULL) {
    printError(MEMORY_ERROR);
    exit(EXIT_FAILURE);
  }

  for (int i = 0; i < image->norm_height; i++) {
    new_image->pixels[i] = malloc(image->header.width_px * sizeof(Pixel));
    if (new_image->pixels[i] == NULL) {
      for (int j = 0; j < i; j++) {
        free(new_image->pixels[j]);
      }
      free(new_image->pixels);
      printError(MEMORY_ERROR);
      exit(EXIT_FAILURE);
    }
  }
}

/* The functions checks if the source image has a valid format.
 * It returns TRUE if the image is valid, and returns FASLE if the image is not
 * valid. DO NOT MODIFY THIS FUNCTION
 */
int checkBMPValid(BMP_Header *header) {
  // Make sure this is a BMP file
  if (header->type != 0x4d42) {
    return FALSE;
  }
  // Make sure we are getting 24 bits per pixel
  if (header->bits_per_pixel != 24 && header->bits_per_pixel != 32) {
    return FALSE;
  }
  // Make sure there is only one image plane
  if (header->planes != 1) {
    return FALSE;
  }
  // Make sure there is no compression
  if (header->compression != 0) {
    return FALSE;
  }
  return TRUE;
}

/* The function prints all information of the BMP_Header.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPHeader(BMP_Header *header) {
  printf("file type (should be 0x4d42): %x\n", header->type);
  printf("file size: %d\n", header->size);
  printf("offset to image data: %d\n", header->offset);
  printf("header size: %d\n", header->header_size);
  printf("width_px: %d\n", header->width_px);
  printf("height_px: %d\n", header->height_px);
  printf("planes: %d\n", header->planes);
  printf("bits: %d\n", header->bits_per_pixel);
}

/* The function prints information of the BMP_Image.
   DO NOT MODIFY THIS FUNCTION
*/
void printBMPImage(BMP_Image *image) {
  printf("data size is %ld\n", sizeof(image->pixels));
  printf("norm_height size is %d\n", image->norm_height);
  printf("bytes per pixel is %d\n", image->bytes_per_pixel);
}
