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

#ifdef XYZ_POLYMORPHIC_CXX_14
#include "compatibility/polymorphic_cxx14.h"
#endif  // XYZ_POLYMORPHIC_CXX_14

#ifdef XYZ_POLYMORPHIC_USES_EXPERIMENTAL_INLINE_VTABLE
#include "experimental/polymorphic_inline_vtable.h"
#endif  // XYZ_POLYMORPHIC_USES_EXPERIMENTAL_INLINE_VTABLE

#ifdef XYZ_POLYMORPHIC_USES_EXPERIMENTAL_SMALL_BUFFER_OPTIMIZATION
#include "experimental/polymorphic_sbo.h"
#endif  // XYZ_POLYMORPHIC_USES_EXPERIMENTAL_SMALL_BUFFER_OPTIMIZATION

#ifndef XYZ_POLYMORPHIC_H_
#include "polymorphic.h"
#endif  // XYZ_POLYMORPHIC_H_

namespace {

constexpr size_t LARGE_VECTOR_SIZE = 1 << 20;
constexpr size_t LARGE_ARRAY_SIZE = 1 << 10;

class Base {
 public:
  virtual ~Base() = default;
  virtual size_t value() const = 0;
  virtual Base* clone() const = 0;
};

class Derived : public Base {
 private:
  size_t value_;

 public:
  ~Derived() override = default;

  Derived(size_t v) : value_(v) {}

  size_t value() const override { return value_; }

  Base* clone() const override { return new Derived(*this); }
};

class Derived2 : public Base {
 private:
  size_t value_;

 public:
  ~Derived2() override = default;

  Derived2(size_t v) : value_(v) {}

  size_t value() const override { return 2 * value_; }

  Base* clone() const override { return new Derived2(*this); }
};

class PolyBase {
 protected:
  PolyBase() = default;
  ~PolyBase() = default;
  PolyBase(const PolyBase&) = default;

 public:
  virtual size_t value() const = 0;
};

class PolyDerived : public PolyBase {
 private:
  size_t value_;

 public:
  PolyDerived(size_t v) : value_(v) {}

  PolyDerived(const PolyDerived&) = default;

  size_t value() const override { return value_; }
};

class PolyDerived2 : public PolyBase {
 private:
  size_t value_;

 public:
  PolyDerived2(size_t v) : value_(v) {}

  PolyDerived2(const PolyDerived2&) = default;

  size_t value() const override { return 2 * value_; }
};

static void Polymorphic_BM_Copy_RawPtr(benchmark::State& state) {
  auto p = new Derived(42);
  for (auto _ : state) {
    auto pp = p->clone();
    benchmark::DoNotOptimize(pp);
    delete pp;
  }
  delete p;
}

static void Polymorphic_BM_VectorCopy_RawPointer(benchmark::State& state) {
  std::vector<Base*> v(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v[i] = new Derived(i);
    } else {
      v[i] = new Derived2(i);
    }
  }

  for (auto _ : state) {
    std::vector<Base*> vv(LARGE_VECTOR_SIZE);

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

static void Polymorphic_BM_ArrayCopy_RawPointer(benchmark::State& state) {
  std::array<Base*, LARGE_ARRAY_SIZE> v;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i % 2 == 0) {
      v[i] = new Derived(i);
    } else {
      v[i] = new Derived2(i);
    }
  }

  for (auto _ : state) {
    std::array<Base*, LARGE_ARRAY_SIZE> vv;
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

static void Polymorphic_BM_VectorAccumulate_RawPointer(
    benchmark::State& state) {
  std::vector<Base*> v(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v[i] = new Derived(i);
    } else {
      v[i] = new Derived2(i);
    }
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

static void Polymorphic_BM_Copy_UniquePtr(benchmark::State& state) {
  auto p = std::make_unique<Derived>(42);
  for (auto _ : state) {
    auto pp = std::unique_ptr<Base>(p->clone());
    benchmark::DoNotOptimize(pp);
  }
}

static void Polymorphic_BM_VectorCopy_UniquePointer(benchmark::State& state) {
  std::vector<std::unique_ptr<Base>> v(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v[i] = std::make_unique<Derived>(i);
    } else {
      v[i] = std::make_unique<Derived2>(i);
    }
  }

  for (auto _ : state) {
    std::vector<std::unique_ptr<Base>> vv(LARGE_VECTOR_SIZE);
    for (size_t i = 0; i < v.size(); ++i) {
      vv[i] = std::unique_ptr<Base>(v[i]->clone());
    }
    benchmark::DoNotOptimize(vv);
  }
}

static void Polymorphic_BM_ArrayCopy_UniquePointer(benchmark::State& state) {
  std::array<std::unique_ptr<Base>, LARGE_ARRAY_SIZE> v;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i % 2 == 0) {
      v[i] = std::make_unique<Derived>(i);
    } else {
      v[i] = std::make_unique<Derived2>(i);
    }
  }

  for (auto _ : state) {
    std::array<std::unique_ptr<Base>, LARGE_ARRAY_SIZE> vv;
    for (size_t i = 0; i < v.size(); ++i) {
      vv[i] = std::unique_ptr<Base>(v[i]->clone());
    }
    benchmark::DoNotOptimize(vv);
  }
}

static void Polymorphic_BM_VectorAccumulate_UniquePointer(
    benchmark::State& state) {
  std::vector<std::unique_ptr<Base>> v(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v[i] = std::make_unique<Derived>(i);
    } else {
      v[i] = std::make_unique<Derived2>(i);
    }
  }

  for (auto _ : state) {
    size_t sum = std::accumulate(
        v.begin(), v.end(), size_t(0),
        [](size_t acc, const auto& p) { return acc + p->value(); });
    benchmark::DoNotOptimize(sum);
  }
}

static void Polymorphic_BM_Copy_Polymorphic(benchmark::State& state) {
  auto p = xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived>, 42);
  for (auto _ : state) {
    auto pp = p;
    benchmark::DoNotOptimize(pp);
  }
}

static void Polymorphic_BM_VectorCopy_Polymorphic(benchmark::State& state) {
  std::vector<xyz::polymorphic<PolyBase>> v;
  v.reserve(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v.push_back(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived>, i));
    } else {
      v.push_back(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived2>, i));
    }
  }

  for (auto _ : state) {
    auto vv = v;
    benchmark::DoNotOptimize(vv);
  }
}

static void Polymorphic_BM_ArrayCopy_Polymorphic(benchmark::State& state) {
  std::array<std::optional<xyz::polymorphic<PolyBase>>, LARGE_ARRAY_SIZE> v;
  for (size_t i = 0; i < v.size(); ++i) {
    if (i % 2 == 0) {
      v[i] = std::optional<xyz::polymorphic<PolyBase>>(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived>, i));
    } else {
      v[i] = std::optional<xyz::polymorphic<PolyBase>>(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived2>, i));
    }
  }

  for (auto _ : state) {
    auto vv = v;
    benchmark::DoNotOptimize(vv);
  }
}

static void Polymorphic_BM_VectorAccumulate_Polymorphic(
    benchmark::State& state) {
  std::vector<xyz::polymorphic<PolyBase>> v;
  v.reserve(LARGE_VECTOR_SIZE);
  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v.push_back(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived>, i));
    } else {
      v.push_back(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived2>, i));
    }
  }

  for (auto _ : state) {
    size_t sum = std::accumulate(
        v.begin(), v.end(), size_t(0),
        [](size_t acc, const auto& p) { return acc + p->value(); });
    benchmark::DoNotOptimize(sum);
  }
}

}  // namespace

BENCHMARK(Polymorphic_BM_Copy_RawPtr);
BENCHMARK(Polymorphic_BM_Copy_UniquePtr);
BENCHMARK(Polymorphic_BM_Copy_Polymorphic);

BENCHMARK(Polymorphic_BM_VectorCopy_RawPointer);
BENCHMARK(Polymorphic_BM_VectorCopy_UniquePointer);
BENCHMARK(Polymorphic_BM_VectorCopy_Polymorphic);

BENCHMARK(Polymorphic_BM_ArrayCopy_RawPointer);
BENCHMARK(Polymorphic_BM_ArrayCopy_UniquePointer);
BENCHMARK(Polymorphic_BM_ArrayCopy_Polymorphic);

BENCHMARK(Polymorphic_BM_VectorAccumulate_RawPointer);
BENCHMARK(Polymorphic_BM_VectorAccumulate_UniquePointer);
BENCHMARK(Polymorphic_BM_VectorAccumulate_Polymorphic);
