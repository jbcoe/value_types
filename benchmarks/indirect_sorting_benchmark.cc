/* Copyright (c) 2016 The Value Types Authors. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
==============================================================================*/

#include <benchmark/benchmark.h>

#include <algorithm>
#include <random>
#include <vector>

#ifdef XYZ_INDIRECT_VALULESS_COPY
#include "experimental/indirect_valueless_copy.h"
#endif  // XYZ_INDIRECT_VALULESS_COPY

#ifdef XYZ_INDIRECT_VALULESS_COMPARE
#include "experimental/indirect_valueless_compare.h"
#endif  // XYZ_INDIRECT_VALULESS_COMPARE

#ifdef XYZ_INDIRECT_VALULESS_COPY_AND_COMPARE
#include "experimental/indirect_valueless_copy_and_compare.h"
#endif  // XYZ_INDIRECT_VALULESS_COPY_AND_COMPARE

#ifndef XYZ_INDIRECT_H
#include "indirect.h"
#endif  // XYZ_INDIRECT_H
namespace {

constexpr size_t LARGE_VECTOR_SIZE = 1 << 22;

static void Indirect_SortingBenchmark(benchmark::State& state) {
  std::vector<xyz::indirect<int>> values(LARGE_VECTOR_SIZE);
  values.resize(LARGE_VECTOR_SIZE);

  std::random_device r;
  std::default_random_engine e(r());
  std::uniform_int_distribution<int> uniform_dist(1, 1000);

  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    values[i] = xyz::indirect<int>(uniform_dist(e));
  }

  for (auto _ : state) {
    std::stable_sort(values.begin(), values.end());
    benchmark::DoNotOptimize(std::is_sorted(values.begin(), values.end()));
  }
}
}  // namespace

BENCHMARK(Indirect_SortingBenchmark);
