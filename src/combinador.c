#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#include "bmp.h"
#include "desenfocador.h"
#include "publicador.h"
#include "realzador.h"

#define SHM_NAME "/bmp_imagen_compartida"

// Add this line to declare ruta_salida as a global variable
const char *ruta_salida;
int shm_fd = -1;
void *shm_ptr = NULL;
size_t shm_size = 0;
pid_t pub_pid, des_pid, real_pid;

static void
handle_err(const int error_code) {
    printError(error_code);
    exit(EXIT_FAILURE);
}

void cleanup_shared_memory(int signum);
int all_children_exited(void);

void signal_handler(int signum) {
    static int completed_filters = 0;
    printf("Received signal: %d\n", signum);
    
    if (signum == SIGUSR1) {
        if (all_children_exited()) {
            printf("All child processes have exited. Terminating parent process.\n");
            cleanup_shared_memory(0);
            exit(0);
        }
        printf("Image ready, starting filters\n");
        kill(des_pid, SIGUSR1);
        kill(real_pid, SIGUSR1);
    } else if (signum == SIGUSR2) {
        completed_filters++;
        printf("Filter completed. Total completed: %d\n", completed_filters);
        if (completed_filters == 2) {
            printf("Both filters completed. Saving image.\n");
            BMP_Image *imagen_compartida = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (imagen_compartida == MAP_FAILED) {
                handle_err(MEMORY_ERROR);
            }

            writeImage((char *)ruta_salida, imagen_compartida);
            completed_filters = 0;
            munmap(imagen_compartida, shm_size);
            printf("Imagen procesada y guardada en: %s\n", ruta_salida);
            kill(pub_pid, SIGUSR1);
        }
    }
}

void cleanup_shared_memory(int signum) {
    if (shm_ptr != NULL && shm_size > 0) {
        munmap(shm_ptr, shm_size);
    }
    if (shm_fd != -1) {
        close(shm_fd);
        shm_unlink(SHM_NAME);
    }
    exit(signum);
}

int all_children_exited(void) {
    int status;
    pid_t wpid;
    
    wpid = waitpid(pub_pid, &status, WNOHANG);
    if (wpid == -1 && errno != ECHILD) return 0;
    
    wpid = waitpid(des_pid, &status, WNOHANG);
    if (wpid == -1 && errno != ECHILD) return 0;
    
    wpid = waitpid(real_pid, &status, WNOHANG);
    if (wpid == -1 && errno != ECHILD) return 0;
    
    return 1;
}

int
main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <ruta_salida> <num_threads>\n", argv[0]);
        exit(1);
    }

    ruta_salida = argv[1];
    int num_threads = atoi(argv[2]);

    if (num_threads <= 0) {
        fprintf(stderr, "El número de hilos debe ser mayor que 0\n");
        exit(1);
    }

    // Set up signal handlers
    signal(SIGINT, cleanup_shared_memory);
    signal(SIGTERM, cleanup_shared_memory);
    signal(SIGUSR1, signal_handler);
    signal(SIGUSR2, signal_handler);

    // Create shared memory
    shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        handle_err(FILE_ERROR);
    }

    // Get the system page size
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size == -1) {
        perror("Error getting system page size");
        exit(1);
    }

    // Set initial size for shared memory (round up to the nearest multiple of page size)
    shm_size = ((1 * 1024 * 1024 + page_size - 1) / page_size) * page_size; // Approximately 1 MB, rounded up
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("Error al establecer el tamaño inicial de la memoria compartida");
        fprintf(stderr, "Errno: %d\n", errno);
        cleanup_shared_memory(1);
    }

    printf("Tamaño inicial de memoria compartida: %zu bytes\n", shm_size);

    // Map the shared memory
    shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("Error al mapear la memoria compartida");
        cleanup_shared_memory(1);
    }

    // Publicador process
    pub_pid = fork();
    if (pub_pid == 0) {
        char input_path[256];
        while (1) {
            printf("Ingrese la ruta de la imagen de entrada (o 'q' para salir): ");
            if (fgets(input_path, sizeof(input_path), stdin) == NULL) {
                exit(1);
            }
            input_path[strcspn(input_path, "\n")] = 0; // Remove newline

            if (strcmp(input_path, "q") == 0) {
                printf("Publicador process exiting...\n");
                kill(getppid(), SIGUSR1); // Signal parent to check for child exits
                exit(0);
            }

            BMP_Image *imagen_compartida;
            int result = publicar_imagen(input_path, &imagen_compartida);
            if (result != 0) {
                fprintf(stderr, "Error al publicar la imagen. Por favor, verifique la ruta e intente nuevamente.\n");
                continue;
            }

            printf("Imagen publicada exitosamente: %s\n", input_path);

            // Signal that the image is ready
            kill(getppid(), SIGUSR1);

            // Wait for the processing to complete
            pause();
        }
    }

    // Desenfocador process
    des_pid = fork();
    if (des_pid == 0) {
        while (1) {
            pause(); // Wait for signal
            BMP_Image *imagen_compartida = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (imagen_compartida == MAP_FAILED) {
                handle_err(MEMORY_ERROR);
            }
            apply_desenfocador_parallel(imagen_compartida, imagen_compartida, num_threads);
            munmap(imagen_compartida, shm_size);
            kill(getppid(), SIGUSR2);
        }
    }

    // Realzador process
    real_pid = fork();
    if (real_pid == 0) {
        while (1) {
            pause(); // Wait for signal
            BMP_Image *imagen_compartida = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            if (imagen_compartida == MAP_FAILED) {
                handle_err(MEMORY_ERROR);
            }
            apply_realzador_parallel(imagen_compartida, imagen_compartida, num_threads);
            munmap(imagen_compartida, shm_size);
            kill(getppid(), SIGUSR2);
        }
    }

    // Parent process
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    while (1) {
        printf("Parent process waiting for signals...\n");
        int sig;
        sigwait(&mask, &sig);
        signal_handler(sig);
    }

    // Clean up
    cleanup_shared_memory(0);

    return 0;
}
