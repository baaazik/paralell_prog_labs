#include "matmul.h"
#include <stdlib.h>
#include <mpi.h>

// Работа 0го процесса - умножение матриц
double matmul(double* a, double* b, double* c, size_t size)
{
    double t0 = MPI_Wtime();

    for (size_t i = 0; i < size; ++i) {
        for (size_t j = 0; j < size; ++j) {
            c[i * size + j] = 0.0;
            for (size_t k = 0; k < size; ++k) {
                c[i * size + j] += a[i * size + k] * b[k * size + j];
            }
        }
    }

    double t1 = MPI_Wtime();
    return t1 - t0;
}

// Создание и заполнение матрицы
double* create_matrix(size_t size)
{
    double* mat = new double[size * size];
    double x0 = rand();

    for (size_t i = 0; i < size * size; i++) {
        mat[i] = x0 + i % 1000;
    }

    return mat;
}