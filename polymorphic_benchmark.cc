#include <benchmark/benchmark.h>

#include <vector>

#include "polymorphic.h"

namespace {

constexpr int LARGE_VECTOR_SIZE = 1 << 20;

static void BM_Clone(benchmark::State& state) {
  class Base {
   public:
    virtual ~Base() = default;
    virtual int value() const = 0;
    virtual std::unique_ptr<Base> clone() const = 0;
  };

  class Derived : public Base {
   private:
    int value_;

   public:
    Derived(int v) : value_(v) {}
    int value() const override { return value_; }
    std::unique_ptr<Base> clone() const override {
      return std::make_unique<Derived>(*this);
    }
  };

  class Derived2 : public Base {
   private:
    int value_;

   public:
    Derived2(int v) : value_(v) {}
    int value() const override { return value_; }
    std::unique_ptr<Base> clone() const override {
      return std::make_unique<Derived2>(*this);
    }
  };

  // Build a large vector of polymorphic objects.
  std::vector<std::unique_ptr<Base>> v;
  v.reserve(LARGE_VECTOR_SIZE);

  for (int i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v.push_back(std::make_unique<Derived>(i));
    } else {
      v.push_back(std::make_unique<Derived2>(i));
    }
  }

  for (auto _ : state) {
    std::vector<std::unique_ptr<Base>> vv;
    vv.reserve(LARGE_VECTOR_SIZE);

    // Clone the vector.
    for (auto& p : v) {
      vv.push_back(p->clone());
    }
  }
}

static void BM_PolymorphicCopy(benchmark::State& state) {
  class PolyBase {
   protected:
    PolyBase() = default;
    ~PolyBase() = default;
    PolyBase(const PolyBase&) = default;

   public:
    virtual int value() const = 0;
  };

  class PolyDerived : public PolyBase {
   private:
    int value_;

   public:
    PolyDerived(int v) : value_(v) {}
    PolyDerived(const PolyDerived&) = default;
    int value() const override { return value_; }
  };

  class PolyDerived2 : public PolyBase {
   private:
    int value_;

   public:
    PolyDerived2(int v) : value_(v) {}
    PolyDerived2(const PolyDerived2&) = default;
    int value() const override { return value_; }
  };

  // Build a large vector of polymorphic objects.
  std::vector<xyz::polymorphic<PolyBase>> v;
  v.reserve(LARGE_VECTOR_SIZE);

  for (int i = 0; i < LARGE_VECTOR_SIZE; ++i) {
    if (i % 2 == 0) {
      v.push_back(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived>, (i)));
    } else {
      v.push_back(
          xyz::polymorphic<PolyBase>(std::in_place_type<PolyDerived2>, (i)));
    }
  }

  for (auto _ : state) {
    std::vector<xyz::polymorphic<PolyBase>> vv;
    vv.reserve(LARGE_VECTOR_SIZE);

    // Clone the vector.
    vv = v;
  }
}

}  // namespace

BENCHMARK(BM_Clone);
BENCHMARK(BM_PolymorphicCopy);
