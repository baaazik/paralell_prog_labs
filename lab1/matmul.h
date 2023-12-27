#ifndef __MATMUL_H__
#define __MATMUL_H__

// Работа 0го процесса - умножение матриц
double matmul(double* a, double* b, double* c, size_t size);

// Создание и заполнение матрицы
double* create_matrix(size_t size);

#endif