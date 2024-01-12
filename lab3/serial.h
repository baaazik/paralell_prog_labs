#ifndef __NO_VECTORIZATION_H__
#define __NO_VECTORIZATION_H__

#include <stdlib.h>

template<class T>
void calc_serial(T* a, T* b, T* c, size_t count)
{
    for (size_t i = 0; i < count; i++) {
        c[i] = a[i] * b[i] - b[i];
    }
}

#endif