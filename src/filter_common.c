#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "filter_common.h"

const char *IMAGE_INPUT = "/input_image";
const char *IMAGE_OUTPUT = "/output_image";

static void *
get_shared_memory(const char *name, int size) {
    int fd = shm_open(name, O_RDWR, 0666);
    if (fd == -1)
        return NULL;
    return mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
}

void *
get_input_image(int size) {
    return get_shared_memory(IMAGE_INPUT, size);
}

void *
get_output_image(int size) {
    return get_shared_memory(IMAGE_OUTPUT, size);
}

void
apply_filter_parallel(Pixel *image_in, Pixel *image_out, int height, int width,
                      int start_row, int end_row, int num_threads,
                      const int filter[FILTER_SIZE][FILTER_SIZE]) {
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    FilterParameters *args = malloc(num_threads * sizeof(FilterParameters));

    int rows_to_process = end_row - start_row;
    int rows_per_thread = rows_to_process / num_threads;
    int remaining_rows = rows_to_process % num_threads;

    for (int i = 0; i < num_threads; i++) {
        int thread_start_row = start_row + i * rows_per_thread +
                               (i < remaining_rows ? i : remaining_rows);
        int thread_end_row =
            thread_start_row + rows_per_thread + (i < remaining_rows ? 1 : 0);

        args[i] = (FilterParameters){.image_in = image_in,
                                     .image_out = image_out,
                                     .start_row = thread_start_row,
                                     .end_row = thread_end_row,
                                     .columns = width,
                                     .height = height,
                                     .filter = filter};

        pthread_create(&threads[i], NULL, filter_thread_worker, &args[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(args);
    free(threads);
}

Pixel **
add_padding(Pixel *image_in, int height, int width) {
    int new_height = height + 2;
    int new_width = width + 2;

    Pixel **pixels_padding = calloc(new_height, sizeof(Pixel *));
    for (int i = 0; i < new_height; i++)
        pixels_padding[i] = calloc(new_width, sizeof(Pixel));

    for (int j = 0; j < height; j++) {
        for (int k = 0; k < width; k++) {
            pixels_padding[j + 1][k + 1] = image_in[j * width + k];
        }
    }

    return pixels_padding;
}

void
free_padded_image(Pixel **data, int height) {
    for (int i = 0; i < height; i++) {
        free(data[i]);
    }
    free(data);
}

void *
filter_thread_worker(void *args) {
    FilterParameters *data = (FilterParameters *)args;

    for (int i = data->start_row; i < data->end_row; i++) {
        for (int j = 0; j < data->columns; j++) {
            int index = i * data->columns + j;
            data->image_out[index] = (Pixel){
                .red = apply_filter_to_channel(data->image_in, RED, i, j,
                                               data->columns, data->height,
                                               data->filter),
                .green = apply_filter_to_channel(data->image_in, GREEN, i, j,
                                                 data->columns, data->height,
                                                 data->filter),
                .blue = apply_filter_to_channel(data->image_in, BLUE, i, j,
                                                data->columns, data->height,
                                                data->filter),
                .alpha = 255};
        }
    }

    return NULL;
}

int
apply_filter_to_channel(Pixel *pixels_in, Channel c, int x, int y, int width,
                        int height,
                        const int filter[FILTER_SIZE][FILTER_SIZE]) {
    int total = 0;
    int count = 0;
    for (int k = 0; k < FILTER_SIZE; k++) {
        for (int l = 0; l < FILTER_SIZE; l++) {
            int m = x + k - FILTER_SIZE / 2;
            int n = y + l - FILTER_SIZE / 2;
            if (m >= 0 && m < height && n >= 0 && n < width) {
                total += get_pixel_channel(&pixels_in[m * width + n], c) *
                         filter[k][l];
                count++;
            }
        }
    }
    return count > 0 ? total / count : 0;
}