#ifndef __INTRINSIC_H__
#define __INTRINSIC_H__

#include <immintrin.h>

// Обобщенная функция - нет реализации
template<class T>
void calc_intrinsic(T* a, T* b, T* c, size_t count)
{
}

// версия для double
template<>
void calc_intrinsic<double>(double* a, double* b, double* c, size_t count)
{
    for (size_t i = 0; i < count; i += 4)
    {
        __m256d va = _mm256_load_pd(&a[i]);     // Загрузка значений из a
        __m256d vb = _mm256_load_pd(&b[i]);     // Загрузка значений из b
        __m256d vc = _mm256_mul_pd(va, vb);     // Умножение a * b
        vc = _mm256_sub_pd(vc, vb);             // Вычитание (a * b) - b
        _mm256_store_pd(&c[i], vc);             // Сохранение значений в c 
    }
}

// версия для float
template<>
void calc_intrinsic<float>(float* a, float* b, float* c, size_t count)
{
    for (size_t i = 0; i < count; i += 8)
    {
        __m256 va = _mm256_load_ps(&a[i]);      // Загрузка значений из a
        __m256 vb = _mm256_load_ps(&b[i]);      // Загрузка значений из b
        __m256 vc = _mm256_mul_ps(va, vb);      // Умножение a * b
        vc = _mm256_sub_ps(vc, vb);             // Вычитание (a * b) - b
        _mm256_store_ps(&c[i], vc);             // Сохранение значений в c 
    }
}

// версия для int64
template<>
void calc_intrinsic<int64_t>(int64_t* a, int64_t* b, int64_t* c, size_t count)
{
    for (size_t i = 0; i < count; i += 4)
    {
        __m256i va = _mm256_load_si256((__m256i*)&a[i]);        // Загрузка значений из a
        __m256i vb = _mm256_load_si256((__m256i*)&b[i]);        // Загрузка значений из b
        
        // AVX не содержит инструкции умножения двух 64-битных целых чисел
        // Поэтому число разбивается на две части, происходит умножение 32х-битных чисел
        // и соединяется обратно
        // https://stackoverflow.com/a/37322570
        __m256i bswap = _mm256_shuffle_epi32(vb, 0xB1);         // swap H<->L
        __m256i prodlh = _mm256_mullo_epi32(va, bswap);         // 32 bit L*H products
        __m256i zero = _mm256_setzero_si256();                  // 0
        __m256i prodlh2 = _mm256_hadd_epi32(prodlh, zero);      // a0Lb0H+a0Hb0L,a1Lb1H+a1Hb1L,0,0
        __m256i prodlh3 = _mm256_shuffle_epi32(prodlh2, 0x73);  // 0, a0Lb0H+a0Hb0L, 0, a1Lb1H+a1Hb1L
        __m256i prodll = _mm256_mul_epu32(va, vb);              // a0Lb0L,a1Lb1L, 64 bit unsigned products
        __m256i prod = _mm256_add_epi64(prodll, prodlh3);       // a0Lb0L+(a0Lb0H+a0Hb0L)<<32, a1Lb1L+(a1Lb1H+a1Hb1L)<<32

        __m256i vc = _mm256_sub_epi64(prod, vb);                // Вычитание (a * b) - b

        _mm256_store_si256((__m256i*) &c[i], vc);               // Сохранение значений в c 
    }
}

// версия для int32
template<>
void calc_intrinsic<int32_t>(int32_t* a, int32_t* b, int32_t* c, size_t count)
{
    for (size_t i = 0; i < count; i += 8)
    {
        __m256i va = _mm256_load_si256((__m256i*) & a[i]);      // Загрузка значений из a
        __m256i vb = _mm256_load_si256((__m256i*) & b[i]);      // Загрузка значений из b
        __m256i vc = _mm256_mullo_epi32(va, vb);                // Умножение a * b
        vc = _mm256_sub_epi32(vc, vb);                          // Вычитание (a * b) - b
        _mm256_store_si256((__m256i*) & c[i], vc);              // Сохранение значений в c 
    }
}

// версия для int16
template<>
void calc_intrinsic<int16_t>(int16_t* a, int16_t* b, int16_t* c, size_t count)
{
    for (size_t i = 0; i < count; i += 16)
    {
        __m256i va = _mm256_load_si256((__m256i*) & a[i]);      // Загрузка значений из a
        __m256i vb = _mm256_load_si256((__m256i*) & b[i]);      // Загрузка значений из b
        __m256i vc = _mm256_mullo_epi16(va, vb);                // Умножение a * b
        vc = _mm256_sub_epi16(vc, vb);                          // Вычитание (a * b) - b
        _mm256_store_si256((__m256i*) & c[i], vc);              // Сохранение значений в c 
    }
}

// версия для int8
template<>
void calc_intrinsic<int8_t>(int8_t* a, int8_t* b, int8_t* c, size_t count)
{
    for (size_t i = 0; i < count; i += 32)
    {
        __m256i va = _mm256_load_si256((__m256i*) & a[i]);      // Загрузка значений из a
        __m256i vb = _mm256_load_si256((__m256i*) & b[i]);      // Загрузка значений из b
        
        // AVX не содержит инструкции для умножения двух 8-битных чисел
        // Разделим на два 16-битных умножения
        // https://github.com/vectorclass/version2/blob/master/vectori256.h#L748
        __m256i aodd = _mm256_srli_epi16(va, 8);                // odd numbered elements of a
        __m256i bodd = _mm256_srli_epi16(vb, 8);                // odd numbered elements of b
        __m256i muleven = _mm256_mullo_epi16(va, vb);           // product of even numbered elements
        __m256i mulodd = _mm256_mullo_epi16(aodd, bodd);        // product of odd  numbered elements
        mulodd = _mm256_slli_epi16(mulodd, 8);                  // put odd numbered elements back in place
        __m256i mask = _mm256_set1_epi32(0x00FF00FF);           // mask for even positions
        __m256i product = _mm256_blendv_epi8(mask, muleven, mulodd);  // interleave even and odd

        __m256i vc = _mm256_sub_epi8(product, vb);              // Вычитание (a * b) - b
        _mm256_store_si256((__m256i*) & c[i], vc);              // Сохранение значений в c 
    }
}

#endif