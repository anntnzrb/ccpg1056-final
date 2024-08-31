#ifndef PUBLICADOR_H
#define PUBLICADOR_H

#include "bmp.h"
#include <stdio.h>

int publicar_imagen(const char *nombre_archivo, BMP_Image **imagen_compartida);

#endif // PUBLICADOR_H
