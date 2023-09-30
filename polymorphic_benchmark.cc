#include <benchmark/benchmark.h>

#include <numeric>
#include <vector>

#include "polymorphic.h"

namespace {

constexpr size_t LARGE_VECTOR_SIZE = 1 << 20;

static void RawPtrClone(benchmark::State& state) {
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

  // Build a large vector of polymorphic objects.
  std::vector<Base*> v;
  v.reserve(LARGE_VECTOR_SIZE);

  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v.push_back(new Derived(i));
    } else {
      v.push_back(new Derived2(i));
    }
  }

  for (auto _ : state) {
    std::vector<Base*> vv(LARGE_VECTOR_SIZE);

    // Clone the vector.
    for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
      vv[i] = v[i]->clone();
    }

    // Sum the values.
    size_t sum = std::accumulate(
        vv.begin(), vv.end(), 0,
        [](size_t acc, const auto& p) { return acc + p->value(); });
    benchmark::DoNotOptimize(sum);

    // Delete the vector.
    for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
      delete vv[i];
    }
  }
}

static void UniquePtrClone(benchmark::State& state) {
  class Base {
   public:
    virtual ~Base() = default;
    virtual size_t value() const = 0;
    virtual std::unique_ptr<Base> clone() const = 0;
  };

  class Derived : public Base {
   private:
    size_t value_;

   public:
    Derived(size_t v) : value_(v) {}
    size_t value() const override { return value_; }
    std::unique_ptr<Base> clone() const override {
      return std::make_unique<Derived>(*this);
    }
  };

  class Derived2 : public Base {
   private:
    size_t value_;

   public:
    Derived2(size_t v) : value_(v) {}
    size_t value() const override { return 2 * value_; }
    std::unique_ptr<Base> clone() const override {
      return std::make_unique<Derived2>(*this);
    }
  };

  // Build a large vector of polymorphic objects.
  std::vector<std::unique_ptr<Base>> v;
  v.reserve(LARGE_VECTOR_SIZE);

  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v.push_back(std::make_unique<Derived>(i));
    } else {
      v.push_back(std::make_unique<Derived2>(i));
    }
  }

  for (auto _ : state) {
    std::vector<std::unique_ptr<Base>> vv(LARGE_VECTOR_SIZE);

    // Clone the vector.
    for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
      vv[i] = v[i]->clone();
    }

    // Sum the values.
    size_t sum = std::accumulate(
        vv.begin(), vv.end(), 0,
        [](size_t acc, const auto& p) { return acc + p->value(); });
    benchmark::DoNotOptimize(sum);

    // Vector destructor deals with deletion.
  }
}

static void PolymorphicCopy(benchmark::State& state) {
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

  // Build a large vector of polymorphic objects.
  std::vector<xyz::polymorphic<PolyBase>> v;
  v.reserve(LARGE_VECTOR_SIZE);

  for (size_t i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v.push_back(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived>, (i)));
    } else {
      v.push_back(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived2>, (i)));
    }
  }

  for (auto _ : state) {
    // Clone the vector.
    auto vv = v;

    // Sum the values.
    size_t sum = std::accumulate(
        vv.begin(), vv.end(), 0,
        [](size_t acc, const auto& p) { return acc + p->value(); });
    benchmark::DoNotOptimize(sum);

    // Vector destructor deals with deletion.
  }
}

}  // namespace

BENCHMARK(RawPtrClone);
BENCHMARK(UniquePtrClone);
BENCHMARK(PolymorphicCopy);
