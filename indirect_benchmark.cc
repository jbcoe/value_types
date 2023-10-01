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

static void Indirect_BM_RawPtrClone(benchmark::State& state) {
  auto p = new A(42);
  for (auto _ : state) {
    auto pp = p->clone();
    benchmark::DoNotOptimize(pp);
  }
  delete p;
}

static void Indirect_BM_RawPointerVectorCopy(benchmark::State& state) {
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

static void Indirect_BM_RawPointerArrayCopy(benchmark::State& state) {
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

static void Indirect_BM_RawPointerVectorAccumulate(benchmark::State& state) {
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

static void Indirect_BM_UniquePtrClone(benchmark::State& state) {
  auto p = std::make_unique<A>(42);
  for (auto _ : state) {
    auto pp = std::unique_ptr<A>(p->clone());
    benchmark::DoNotOptimize(pp);
  }
}

static void Indirect_BM_UniquePointerVectorCopy(benchmark::State& state) {
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

static void Indirect_BM_UniquePointerArrayCopy(benchmark::State& state) {
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
static void Indirect_BM_UniquePointerVectorAccumulate(benchmark::State& state) {
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

static void Indirect_BM_IndirectCopy(benchmark::State& state) {
  auto p = xyz::indirect<A>(std::in_place, 42);
  for (auto _ : state) {
    auto pp = p;
    benchmark::DoNotOptimize(pp);
  }
}

static void Indirect_BM_IndirectVectorCopy(benchmark::State& state) {
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

static void Indirect_BM_IndirectArrayCopy(benchmark::State& state) {
  std::array<std::optional<xyz::indirect<A>>, LARGE_ARRAY_SIZE> v;
  for (size_t i = 0; i < v.size(); ++i) {
    v[i] = std::optional<xyz::indirect<A>>(xyz::indirect<A>(std::in_place, i));
  }

  for (auto _ : state) {
    auto vv = v;
    benchmark::DoNotOptimize(vv);
  }
}

static void Indirect_BM_IndirectVectorAccumulate(benchmark::State& state) {
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

BENCHMARK(Indirect_BM_RawPtrClone);
BENCHMARK(Indirect_BM_UniquePtrClone);
BENCHMARK(Indirect_BM_IndirectCopy);

BENCHMARK(Indirect_BM_RawPointerVectorCopy);
BENCHMARK(Indirect_BM_UniquePointerVectorCopy);
BENCHMARK(Indirect_BM_IndirectVectorCopy);

BENCHMARK(Indirect_BM_RawPointerArrayCopy);
BENCHMARK(Indirect_BM_UniquePointerArrayCopy);
BENCHMARK(Indirect_BM_IndirectArrayCopy);

BENCHMARK(Indirect_BM_RawPointerVectorAccumulate);
BENCHMARK(Indirect_BM_UniquePointerVectorAccumulate);
BENCHMARK(Indirect_BM_IndirectVectorAccumulate);
