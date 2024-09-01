#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "bmp.h"
#include "publicador.h"
#include "realzador.h"
#include "desenfocador.h"
#include <sys/stat.h>

#define SHM_NAME "/bmp_imagen_compartida"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ruta_entrada> <ruta_salida>\n", argv[0]);
        exit(1);
    }

    const char *ruta_entrada = argv[1];
    const char *ruta_salida = argv[2];

    // Lanzar el proceso Publicador
    pid_t pid_publicador = fork();
    if (pid_publicador == 0) {
        execl("./publicador", "./publicador", ruta_entrada, NULL);
        perror("Error al ejecutar el publicador");
        exit(1);
    }

    // Esperar a que el Publicador termine
    waitpid(pid_publicador, NULL, 0);

    // Abrir la memoria compartida
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Error al abrir la memoria compartida");
        exit(1);
    }

    // Obtener el tamaño de la memoria compartida
    struct stat shm_stat;
    if (fstat(shm_fd, &shm_stat) == -1) {
        perror("Error al obtener el tamaño de la memoria compartida");
        close(shm_fd);
        exit(1);
    }

    // Mapear la memoria compartida
    BMP_Image *imagen_compartida = mmap(NULL, shm_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (imagen_compartida == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        close(shm_fd);
        exit(1);
    }

    // Lanzar el proceso Desenfocador
    pid_t pid_desenfocador = fork();
    if (pid_desenfocador == 0) {
        execl("./desenfocador", "./desenfocador", NULL);
        perror("Error al ejecutar el desenfocador");
        exit(1);
    }

    // Esperar a que el Desenfocador termine
    waitpid(pid_desenfocador, NULL, 0);

    // Lanzar el proceso Realzador
    pid_t pid_realzador = fork();
    if (pid_realzador == 0) {
        execl("./realzador", "./realzador", NULL);
        perror("Error al ejecutar el realzador");
        exit(1);
    }

    // Esperar a que el Realzador termine
    waitpid(pid_realzador, NULL, 0);

    // Guardar la imagen procesada
    writeImage((char *)ruta_salida, imagen_compartida);

    // Limpiar la memoria compartida
    munmap(imagen_compartida, shm_stat.st_size);
    close(shm_fd);
    shm_unlink(SHM_NAME);

    printf("Imagen procesada guardada en: %s\n", ruta_salida);

    return 0;
}
