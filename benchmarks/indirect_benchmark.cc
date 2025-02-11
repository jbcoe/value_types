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

#include <array>
#include <iostream>
#include <numeric>
#include <optional>
#include <vector>

#include "indirect.h"

namespace {

constexpr size_t LARGE_VECTOR_SIZE = 1 << 20;
constexpr size_t LARGE_ARRAY_SIZE = 1 << 10;

class A {
  size_t value_;

 public:
  ~A() = default;

  A(size_t v) : value_(v) {}

  A(const A&) = default;

  A* clone() const { return new A(*this); }

  size_t value() const { return value_; }
};

static void Indirect_BM_Copy_RawPtr(benchmark::State& state) {
  auto p = new A(42);
  for (auto _ : state) {
    auto pp = p->clone();
    benchmark::DoNotOptimize(pp);
  }
  delete p;
}

static void Indirect_BM_VectorCopy_RawPointer(benchmark::State& state) {
  std::vector<A*> v(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    v[i] = new A(i);
  }

  for (auto _ : state) {
    std::vector<A*> vv(LARGE_VECTOR_SIZE);

    for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
      vv[i] = v[i]->clone();
    }
    benchmark::DoNotOptimize(vv);

    for (auto& p : vv) {
      delete p;
    }
  }

  for (auto& p : v) {
    delete p;
  }
}

static void Indirect_BM_ArrayCopy_RawPointer(benchmark::State& state) {
  std::array<A*, LARGE_ARRAY_SIZE> v;
  for (size_t i = 0; i < v.size(); ++i) {
    v[i] = new A(i);
  }

  for (auto _ : state) {
    std::array<A*, LARGE_ARRAY_SIZE> vv;
    for (size_t i = 0; i < v.size(); ++i) {
      vv[i] = v[i]->clone();
    }
    benchmark::DoNotOptimize(vv);
    for (size_t i = 0; i < vv.size(); ++i) {
      delete vv[i];
    }
  }

  for (size_t i = 0; i < v.size(); ++i) {
    delete v[i];
  }
}

static void Indirect_BM_VectorAccumulate_RawPointer(benchmark::State& state) {
  std::vector<A*> v(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    v[i] = new A(i);
  }

  for (auto _ : state) {
    size_t sum = std::accumulate(
        v.begin(), v.end(), size_t(0),
        [](size_t acc, const auto& p) { return acc + p->value(); });
    benchmark::DoNotOptimize(sum);
  }

  for (auto& p : v) {
    delete p;
  }
}

static void Indirect_BM_Copy_UniquePtr(benchmark::State& state) {
  auto p = std::make_unique<A>(42);
  for (auto _ : state) {
    auto pp = std::unique_ptr<A>(p->clone());
    benchmark::DoNotOptimize(pp);
  }
}

static void Indirect_BM_VectorCopy_UniquePointer(benchmark::State& state) {
  std::vector<std::unique_ptr<A>> v(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    v[i] = std::make_unique<A>(i);
  }

  for (auto _ : state) {
    std::vector<std::unique_ptr<A>> vv(LARGE_VECTOR_SIZE);
    for (size_t i = 0; i < v.size(); ++i) {
      vv[i] = std::unique_ptr<A>(v[i]->clone());
    }
    benchmark::DoNotOptimize(vv);
  }
}

static void Indirect_BM_ArrayCopy_UniquePointer(benchmark::State& state) {
  std::array<std::unique_ptr<A>, LARGE_ARRAY_SIZE> v;
  for (size_t i = 0; i < v.size(); ++i) {
    v[i] = std::make_unique<A>(i);
  }

  for (auto _ : state) {
    std::array<std::unique_ptr<A>, LARGE_ARRAY_SIZE> vv;
    for (size_t i = 0; i < v.size(); ++i) {
      vv[i] = std::unique_ptr<A>(v[i]->clone());
    }
    benchmark::DoNotOptimize(vv);
  }
}

static void Indirect_BM_VectorAccumulate_UniquePointer(
    benchmark::State& state) {
  std::vector<std::unique_ptr<A>> v(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    v[i] = std::make_unique<A>(i);
  }

  for (auto _ : state) {
    size_t sum = std::accumulate(
        v.begin(), v.end(), size_t(0),
        [](size_t acc, const auto& p) { return acc + p->value(); });
    benchmark::DoNotOptimize(sum);
  }
}

static void Indirect_BM_Copy_Indirect(benchmark::State& state) {
  auto p = xyz::indirect<A>(std::in_place, 42);
  for (auto _ : state) {
    auto pp = p;
    benchmark::DoNotOptimize(pp);
  }
}

static void Indirect_BM_VectorCopy_Indirect(benchmark::State& state) {
  std::vector<xyz::indirect<A>> v;
  v.reserve(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    v.push_back(xyz::indirect<A>(std::in_place, i));
  }

  for (auto _ : state) {
    auto vv = v;
    benchmark::DoNotOptimize(vv);
  }
}

static void Indirect_BM_ArrayCopy_Indirect(benchmark::State& state) {
  std::array<std::optional<xyz::indirect<A>>, LARGE_ARRAY_SIZE> v;
  for (size_t i = 0; i < v.size(); ++i) {
    v[i] = std::optional<xyz::indirect<A>>(xyz::indirect<A>(std::in_place, i));
  }

  for (auto _ : state) {
    auto vv = v;
    benchmark::DoNotOptimize(vv);
  }
}

static void Indirect_BM_VectorAccumulate_Indirect(benchmark::State& state) {
  std::vector<xyz::indirect<A>> v;
  v.reserve(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    v.push_back(xyz::indirect<A>(std::in_place, i));
  }

  for (auto _ : state) {
    size_t sum = std::accumulate(
        v.begin(), v.end(), size_t(0),
        [](size_t acc, const auto& p) { return acc + p->value(); });
    benchmark::DoNotOptimize(sum);
  }
}

}  // namespace

BENCHMARK(Indirect_BM_Copy_RawPtr);
BENCHMARK(Indirect_BM_Copy_UniquePtr);
BENCHMARK(Indirect_BM_Copy_Indirect);

BENCHMARK(Indirect_BM_VectorCopy_RawPointer);
BENCHMARK(Indirect_BM_VectorCopy_UniquePointer);
BENCHMARK(Indirect_BM_VectorCopy_Indirect);

BENCHMARK(Indirect_BM_ArrayCopy_RawPointer);
BENCHMARK(Indirect_BM_ArrayCopy_UniquePointer);
BENCHMARK(Indirect_BM_ArrayCopy_Indirect);

BENCHMARK(Indirect_BM_VectorAccumulate_RawPointer);
BENCHMARK(Indirect_BM_VectorAccumulate_UniquePointer);
BENCHMARK(Indirect_BM_VectorAccumulate_Indirect);
