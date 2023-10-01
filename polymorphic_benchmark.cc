#include <benchmark/benchmark.h>

#include <array>
#include <iostream>
#include <numeric>
#include <optional>
#include <vector>

#include "polymorphic.h"

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

static void Polymorphic_BM_RawPtrClone(benchmark::State& state) {
  auto p = new Derived(42);
  for (auto _ : state) {
    auto pp = p->clone();
    benchmark::DoNotOptimize(pp);
  }
  delete p;
}

static void Polymorphic_BM_RawPointerVectorCopy(benchmark::State& state) {
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

static void Polymorphic_BM_RawPointerArrayCopy(benchmark::State& state) {
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

static void Polymorphic_BM_RawPointerVectorAccumulate(benchmark::State& state) {
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

static void Polymorphic_BM_UniquePtrClone(benchmark::State& state) {
  auto p = std::make_unique<Derived>(42);
  for (auto _ : state) {
    auto pp = std::unique_ptr<Base>(p->clone());
    benchmark::DoNotOptimize(pp);
  }
}

static void Polymorphic_BM_UniquePointerVectorCopy(benchmark::State& state) {
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

static void Polymorphic_BM_UniquePointerArrayCopy(benchmark::State& state) {
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
static void Polymorphic_BM_UniquePointerVectorAccumulate(
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

static void Polymorphic_BM_PolymorphicCopy(benchmark::State& state) {
  auto p = xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived>, 42);
  for (auto _ : state) {
    auto pp = p;
    benchmark::DoNotOptimize(pp);
  }
}

static void Polymorphic_BM_PolymorphicVectorCopy(benchmark::State& state) {
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

static void Polymorphic_BM_PolymorphicArrayCopy(benchmark::State& state) {
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

static void Polymorphic_BM_PolymorphicVectorAccumulate(
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

BENCHMARK(Polymorphic_BM_RawPtrClone);
BENCHMARK(Polymorphic_BM_UniquePtrClone);
BENCHMARK(Polymorphic_BM_PolymorphicCopy);

BENCHMARK(Polymorphic_BM_RawPointerVectorCopy);
BENCHMARK(Polymorphic_BM_UniquePointerVectorCopy);
BENCHMARK(Polymorphic_BM_PolymorphicVectorCopy);

BENCHMARK(Polymorphic_BM_RawPointerArrayCopy);
BENCHMARK(Polymorphic_BM_UniquePointerArrayCopy);
BENCHMARK(Polymorphic_BM_PolymorphicArrayCopy);

BENCHMARK(Polymorphic_BM_RawPointerVectorAccumulate);
BENCHMARK(Polymorphic_BM_UniquePointerVectorAccumulate);
BENCHMARK(Polymorphic_BM_PolymorphicVectorAccumulate);
