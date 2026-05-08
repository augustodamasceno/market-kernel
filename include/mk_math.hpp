/* Market Kernel : Utilities
 *
 * Copyright (c) 2026, Augusto Damasceno. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 * See (https://github.com/augustodamasceno/marketdata)
 */

#pragma once

#include <cstddef>
#include <numeric>
#include <vector>

#if defined(__AVX__)
#    include <immintrin.h>
#endif

namespace marketkernel {

/*
 * simd_sum - sum all elements of a contiguous vector.
 *
 * Generic fallback uses std::accumulate (scalar, compiler auto-vectorizable).
 * Explicit specializations for float and double use AVX vectorized reduction
 * when compiled with AVX support, with a scalar remainder loop.
 *
 * Pre-condition: data must be non-empty; summing an empty vector returns Num{0}.
 */
template<typename Num>
Num simd_sum(const std::vector<Num>& data)
{
    return std::accumulate(data.begin(), data.end(), Num{0});
}

template<>
[[gnu::always_inline]]
inline float simd_sum<float>(const std::vector<float>& data)
{
    const float* ptr = data.data();
    const std::size_t n = data.size();
    float result = 0.0F;

#if defined(__AVX__)
    __m256 acc0 = _mm256_setzero_ps();
    __m256 acc1 = _mm256_setzero_ps();
    __m256 acc2 = _mm256_setzero_ps();
    __m256 acc3 = _mm256_setzero_ps();
    std::size_t i = 0;
    for (; i + 32 <= n; i += 32)
    {
        acc0 = _mm256_add_ps(acc0, _mm256_loadu_ps(ptr + i));
        acc1 = _mm256_add_ps(acc1, _mm256_loadu_ps(ptr + i + 8));
        acc2 = _mm256_add_ps(acc2, _mm256_loadu_ps(ptr + i + 16));
        acc3 = _mm256_add_ps(acc3, _mm256_loadu_ps(ptr + i + 24));
    }
    __m256 acc = _mm256_add_ps(_mm256_add_ps(acc0, acc1),
                               _mm256_add_ps(acc2, acc3));
    for (; i + 8 <= n; i += 8)
    {
        acc = _mm256_add_ps(acc, _mm256_loadu_ps(ptr + i));
    }

    __m128 lo = _mm256_castps256_ps128(acc);
    __m128 hi = _mm256_extractf128_ps(acc, 1);
    lo = _mm_add_ps(lo, hi);

    // Prefer shuffle-based horizontal reduction to avoid hadd throughput limits.
    __m128 shuf = _mm_movehl_ps(lo, lo);
    lo = _mm_add_ps(lo, shuf);
    shuf = _mm_shuffle_ps(lo, lo, _MM_SHUFFLE(1, 1, 1, 1));
    lo = _mm_add_ss(lo, shuf);
    result = _mm_cvtss_f32(lo);

    for (; i < n; ++i)
    {
        result += ptr[i];
    }
#else
    std::size_t i = 0;
    for (; i + 4 <= n; i += 4)
    {
        result += ptr[i] + ptr[i + 1] + ptr[i + 2] + ptr[i + 3];
    }
    for (; i < n; ++i)
    {
        result += ptr[i];
    }
#endif

    return result;
}

template<>
[[gnu::always_inline]]
inline double simd_sum<double>(const std::vector<double>& data)
{
    const double* ptr = data.data();
    const std::size_t n = data.size();
    double result = 0.0;

#if defined(__AVX__)
    __m256d acc0 = _mm256_setzero_pd();
    __m256d acc1 = _mm256_setzero_pd();
    __m256d acc2 = _mm256_setzero_pd();
    __m256d acc3 = _mm256_setzero_pd();
    std::size_t i = 0;
    for (; i + 16 <= n; i += 16)
    {
        acc0 = _mm256_add_pd(acc0, _mm256_loadu_pd(ptr + i));
        acc1 = _mm256_add_pd(acc1, _mm256_loadu_pd(ptr + i + 4));
        acc2 = _mm256_add_pd(acc2, _mm256_loadu_pd(ptr + i + 8));
        acc3 = _mm256_add_pd(acc3, _mm256_loadu_pd(ptr + i + 12));
    }
    __m256d acc = _mm256_add_pd(_mm256_add_pd(acc0, acc1),
                                _mm256_add_pd(acc2, acc3));
    for (; i + 4 <= n; i += 4)
    {
        acc = _mm256_add_pd(acc, _mm256_loadu_pd(ptr + i));
    }

    __m128d lo = _mm256_castpd256_pd128(acc);
    __m128d hi = _mm256_extractf128_pd(acc, 1);
    lo = _mm_add_pd(lo, hi);

    const __m128d hi_lane = _mm_unpackhi_pd(lo, lo);
    const __m128d reduced = _mm_add_sd(lo, hi_lane);
    result = _mm_cvtsd_f64(reduced);

    for (; i < n; ++i)
    {
        result += ptr[i];
    }
#else
    std::size_t i = 0;
    for (; i + 4 <= n; i += 4)
    {
        result += ptr[i] + ptr[i + 1] + ptr[i + 2] + ptr[i + 3];
    }
    for (; i < n; ++i)
    {
        result += ptr[i];
    }
#endif

    return result;
}

} // namespace marketkernel
