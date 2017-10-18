#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "mpi.h"

#define ROOT 0
#define ALIVE 'X'
#define DEAD '.'

int toindex(int row, int col, int N)
{
    if (row < 0) {
        row = row + N;
    } else if (row >= N) {
        row = row - N;
    }
    if (col < 0) {
        col = col + N;
    } else if (col >= N) {
        col = col - N;
    }
    return row * N + col;
}

void printgrid(char* grid, FILE* f, int N)
{
    char* buf = (char*)malloc((N + 1) * sizeof(char));
    for (int i = 0; i < N; ++i) {
        strncpy(buf, grid + i * N, N);
        buf[N] = 0;
        fprintf(f, "%s\n", buf);
    }
    free(buf);
}

char get_state(char* buf, int i, int j, int N)
{
    int current = N * i + j;
    int alive_count = 0;
    for (int di = -1; di <= 1; ++di) {
        for (int dj = -1; dj <= 1; ++dj) {
            if ((di != 0 || dj != 0) && buf[toindex(i + di, j + dj, N)] == ALIVE) {
                ++alive_count;
            }
        }
    }
    if (alive_count == 3 || (alive_count == 2 && buf[current] == ALIVE)) {
        return ALIVE;
    } else {
        return DEAD;
    }
}

void fill_front(char* front, int next_front, int i, int j, int N)
{
    for (int di = -1; di <= 1; ++di) {
        for (int dj = -1; dj <= 1; ++dj) {
            front[toindex(i + di, j + dj, N)] = next_front;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 5) {
        fprintf(stderr, "Usage: %s N input_file iterations output_file\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    int iterations = atoi(argv[3]);

    int rank, size, i;
    int block_size;
    MPI_Status statuses[4];
    MPI_Request requests[4];

    char* grid = NULL;

    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int up = (size + rank - 1) % size;
    int down = (rank + 1) % size;

    block_size = N * (N / size);

    if (rank == ROOT) {
        grid = (char*)malloc(N * N * sizeof(char));
        FILE* input = fopen(argv[2], "r");
        for (int i = 0; i < N; ++i) {
            fscanf(input, "%s", grid + i * N);
        }
        fclose(input);

    }
    MPI_Barrier(MPI_COMM_WORLD);

    char* block = (char*)malloc((block_size + 2 * N) * sizeof(char));
    char* buf = (char*)malloc((block_size + 2 * N) * sizeof(char));
    char* front = (char*)malloc((block_size + 2 * N) * sizeof(char));
    memset(front, 1, block_size + 2 * N);

    MPI_Scatter(grid, block_size, MPI_CHAR,
        block + N * sizeof(char), block_size, MPI_CHAR,
        ROOT, MPI_COMM_WORLD);

    for (int iter = 0; iter < iterations; ++iter) {
        MPI_Isend(block + N, N, MPI_CHAR, up, 0, MPI_COMM_WORLD, &requests[0]);
        MPI_Irecv(block + N + block_size, N, MPI_CHAR, down, 0, MPI_COMM_WORLD, &requests[1]);
        MPI_Isend(block + block_size, N, MPI_CHAR, down, 1, MPI_COMM_WORLD, &requests[2]);
        MPI_Irecv(block, N, MPI_CHAR, up, 1, MPI_COMM_WORLD, &requests[3]);

        int next_front = (iter + 1) % 2 + 1;
        for (int i = 2; i < block_size / N; ++i) {
            for (int j = 0; j < N; ++j) {
                int current = i * N + j;
                if (front[current] != 0) {
                    buf[current] = get_state(block, i, j, N);
                    if (buf[current] != block[current]) {
                        fill_front(front, next_front, i, j, N);
                    } else {
                        if (front[current] != next_front) {
                            front[current] = 0;
                        }
                    }
                }
            }
        }

        MPI_Waitall(4, requests, statuses);

        for (int i = 1; i < block_size / N + 1; i += block_size / N - 1) {
            for (int j = 0; j < N; ++j) {
                int current = i * N + j;
                buf[current] = get_state(block, i, j, N);
                if (buf[current] != block[current]) {
                    fill_front(front, next_front, i, j, N);
                } else {
                    if (front[current] != next_front) {
                        front[current] = 0;
                    }
                }
            }
        }
        char* tmp = block; block = buf; buf = tmp;
    }

    MPI_Gather(block + N, block_size, MPI_CHAR, grid, block_size, MPI_CHAR, ROOT, MPI_COMM_WORLD);

    if (rank == ROOT) {
        FILE* output = fopen(argv[4], "w");
        printgrid(grid, output, N);
        fclose(output);
        free(grid);
    }
    free(block);
    free(buf);
    free(front);

    MPI_Finalize();

    return 0;
}