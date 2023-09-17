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

#include <gtest/gtest.h>

#include <mutex>
#include <optional>
#include <utility>

#ifndef XYZ_USES_ALLOCATORS
#error "XYZ_USES_ALLOCATORS must be defined"
#endif  // XYZ_USES_ALLOCATORS
#if (XYZ_USES_ALLOCATORS == 1)
#include "polymorphic_with_allocators.h"
#elif (XYZ_USES_ALLOCATORS == 0)
#include "polymorphic.h"
#else
#error "XYZ_USES_ALLOCATORS must be 0 or 1"
#endif  // XYZ_USES_ALLOCATORS == 0 or 1

#if (XYZ_USES_ALLOCATORS == 1)
#define XYZ_ALLOC_TEST(S, N) TEST(S, N)
#else
#define XYZ_ALLOC_TEST(S, N) TEST(S, DISABLED_##N)
#endif  // XYZ_USES_ALLOCATORS == 1

namespace {

class A {
  int value_ = 0;

 public:
  A() = default;
  A(int value) : value_(value) {}
  int value() const { return value_; }
  friend bool operator==(const A& lhs, const A& rhs) {
    return lhs.value_ == rhs.value_;
  }
};

TEST(PolymorphicTest, ValueAccess) {
  xyz::polymorphic<A> a(std::in_place_type<A>, 42);
  EXPECT_EQ(*a, 42);
}

TEST(PolymorphicTest, CopiesAreDistinct) {
  xyz::polymorphic<A> a(std::in_place_type<A>, 42);
  auto aa = a;
  EXPECT_EQ(*a, *aa);
  EXPECT_NE(&*a, &*aa);
}

XYZ_ALLOC_TEST(PolymorphicTest, MovePreservesOwnedObjectAddress) {
  xyz::polymorphic<A> a(std::in_place_type<A>, 42);
  auto address = &*a;
  auto aa = std::move(a);

#if XYZ_USES_ALLOCATORS == 1
  EXPECT_TRUE(a.valueless_after_move());
#endif

  EXPECT_EQ(address, &*aa);
}

TEST(PolymorphicTest, ConstPropagation) {
  struct SomeType {
    enum class Constness { CONST, NON_CONST };
    Constness member() { return Constness::NON_CONST; }
    Constness member() const { return Constness::CONST; }
  };

  xyz::polymorphic<SomeType> a(std::in_place_type<SomeType>);
  EXPECT_EQ(a->member(), SomeType::Constness::NON_CONST);
  EXPECT_EQ((*a).member(), SomeType::Constness::NON_CONST);
  const auto& ca = a;
  EXPECT_EQ(ca->member(), SomeType::Constness::CONST);
  EXPECT_EQ((*ca).member(), SomeType::Constness::CONST);
}

TEST(PolymorphicTest, Swap) {
  xyz::polymorphic<A> a(std::in_place_type<A>, 42);
  xyz::polymorphic<A> b(std::in_place_type<A>, 43);
  auto address_a = &*a;
  auto address_b = &*b;
  swap(a, b);
  EXPECT_EQ(*a, 43);
  EXPECT_EQ(*b, 42);
  EXPECT_EQ(address_a, &*b);
  EXPECT_EQ(address_b, &*a);
}
class Base {
 public:
  virtual int value() const { return -1; }
};
class Derived : public Base {
 private:
  int value_;

 public:
  Derived(int v) : value_(v) {}
  int value() const override { return value_; }
};

TEST(PolymorphicTest, AccessDerivedObject) {
  xyz::polymorphic<Base> a(std::in_place_type<Derived>, 42);
  EXPECT_EQ(a->value(), 42);
}

TEST(PolymorphicTest, CopiesOfDerivedObjectsAreDistinct) {
  xyz::polymorphic<Base> a(std::in_place_type<Derived>, 42);
  auto aa = a;
  EXPECT_EQ(a->value(), aa->value());
  EXPECT_NE(&*a, &*aa);
}

XYZ_ALLOC_TEST(PolymorphicTest, MovePreservesOwnedDerivedObjectAddress) {
  xyz::polymorphic<Base> a(std::in_place_type<Derived>, 42);
  auto address = &*a;
  auto aa = std::move(a);
  EXPECT_EQ(address, &*aa);
}

TEST(PolymorphicTest, CopyAssignment) {
  xyz::polymorphic<Base> a(std::in_place_type<Derived>, 42);
  xyz::polymorphic<Base> b(std::in_place_type<Derived>, 101);
  EXPECT_EQ(a->value(), 42);
  a = b;

  EXPECT_EQ(a->value(), 101);
  EXPECT_NE(&*a, &*b);
}

TEST(PolymorphicTest, MoveAssignment) {
  xyz::polymorphic<Base> a(std::in_place_type<Derived>, 42);
  xyz::polymorphic<Base> b(std::in_place_type<Derived>, 101);
  EXPECT_EQ(a->value(), 42);
  a = std::move(b);

#if XYZ_USES_ALLOCATORS == 1
  EXPECT_TRUE(b.valueless_after_move());
#endif

  EXPECT_EQ(a->value(), 101);
}

TEST(PolymorphicTest, NonMemberSwap) {
  xyz::polymorphic<Base> a(std::in_place_type<Derived>, 42);
  xyz::polymorphic<Base> b(std::in_place_type<Derived>, 101);
  using std::swap;
  swap(a, b);
  EXPECT_EQ(a->value(), 101);
  EXPECT_EQ(b->value(), 42);
}

TEST(PolymorphicTest, MemberSwap) {
  xyz::polymorphic<Base> a(std::in_place_type<Derived>, 42);
  xyz::polymorphic<Base> b(std::in_place_type<Derived>, 101);

  a.swap(b);
  EXPECT_EQ(a->value(), 101);
  EXPECT_EQ(b->value(), 42);
}

TEST(IndirectTest, ConstPropagation) {
  struct SomeType {
    enum class Constness { CONST, NON_CONST };
    Constness member() { return Constness::NON_CONST; }
    Constness member() const { return Constness::CONST; }
  };

  xyz::polymorphic<SomeType> a(std::in_place_type<SomeType>);
  EXPECT_EQ(a->member(), SomeType::Constness::NON_CONST);
  EXPECT_EQ((*a).member(), SomeType::Constness::NON_CONST);
  const auto& ca = a;
  EXPECT_EQ(ca->member(), SomeType::Constness::CONST);
  EXPECT_EQ((*ca).member(), SomeType::Constness::CONST);
}

TEST(PolymorphicTest, Optional) {
  std::optional<xyz::polymorphic<Base>> a;
  EXPECT_FALSE(a.has_value());
  a.emplace(std::in_place_type<Derived>, 42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ((*a)->value(), 42);
}

#if (XYZ_USES_ALLOCATORS == 1)

template <typename T>
struct TrackingAllocator {
  unsigned* alloc_counter_;
  unsigned* dealloc_counter_;

  TrackingAllocator(unsigned* alloc_counter, unsigned* dealloc_counter)
      : alloc_counter_(alloc_counter),
        dealloc_counter_(dealloc_counter) {}

  template <typename U>
  TrackingAllocator(const TrackingAllocator<U>& other)
      : alloc_counter_(other.alloc_counter_),
        dealloc_counter_(other.dealloc_counter_) {}

  using value_type = T;

  template <typename Other>
  struct rebind {
    using other = TrackingAllocator<Other>;
  };

  constexpr T* allocate(std::size_t n) {
    ++(*alloc_counter_);
    std::allocator<T> default_allocator{};
    return default_allocator.allocate(n);
  }
  constexpr void deallocate(T* p, std::size_t n) {
    ++(*dealloc_counter_);
    std::allocator<T> default_allocator{};
    default_allocator.deallocate(p, n);
  }
};

TEST(PolymorphicTest, CountAllocationsForInPlaceConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<A, TrackingAllocator<A>> a(
        std::allocator_arg,
        TrackingAllocator<A>(&alloc_counter, &dealloc_counter),
        std::in_place_type<A>, 42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
  }
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

TEST(PolymorphicTest, CountAllocationsForDerivedTypeConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Base, TrackingAllocator<Base>> a(
        std::allocator_arg,
        TrackingAllocator<A>(&alloc_counter, &dealloc_counter),
        std::in_place_type<Derived>, 42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
  }
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}
#endif  // (XYZ_USES_ALLOCATORS == 1)
}  // namespace
