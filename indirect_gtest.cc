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

#include <optional>
#include <utility>

#ifndef XYZ_USES_ALLOCATORS
#error "XYZ_USES_ALLOCATORS must be defined"
#endif  // XYZ_USES_ALLOCATORS
#if XYZ_USES_ALLOCATORS == 1
#include "indirect_with_allocators.h"
#elif XYZ_USES_ALLOCATORS == 0
#include "indirect.h"
#else
#error "XYZ_USES_ALLOCATORS must be 0 or 1"
#endif  // XYZ_USES_ALLOCATORS == 0 or 1

TEST(IndirectTest, ValueAccess) {
  xyz::indirect<int> a(std::in_place, 42);
  EXPECT_EQ(*a, 42);
}

TEST(IndirectTest, CopiesAreDistinct) {
  xyz::indirect<int> a(std::in_place, 42);
  auto aa = a;
  EXPECT_EQ(*a, *aa);
  EXPECT_NE(&*a, &*aa);
}

TEST(IndirectTest, MovePreservesIndirectObjectAddress) {
  xyz::indirect<int> a(std::in_place, 42);
  auto address = &*a;
  auto aa = std::move(a);

#if XYZ_USES_ALLOCATORS == 1
  EXPECT_TRUE(a.valueless_after_move());
#endif

  EXPECT_EQ(address, &*aa);
}

TEST(IndirectTest, CopyAssignment) {
  xyz::indirect<int> a(std::in_place, 42);
  xyz::indirect<int> b(std::in_place, 101);
  EXPECT_EQ(*a, 42);
  a = b;

  EXPECT_EQ(*a, 101);
  EXPECT_NE(&*a, &*b);
}

TEST(IndirectTest, MoveAssignment) {
  xyz::indirect<int> a(std::in_place, 42);
  xyz::indirect<int> b(std::in_place, 101);
  EXPECT_EQ(*a, 42);
  a = std::move(b);

#if XYZ_USES_ALLOCATORS == 1
  EXPECT_TRUE(b.valueless_after_move());
#endif

  EXPECT_EQ(*a, 101);
}

TEST(IndirectTest, NonMemberSwap) {
  xyz::indirect<int> a(std::in_place, 42);
  xyz::indirect<int> b(std::in_place, 101);
  using std::swap;
  swap(a, b);
  EXPECT_EQ(*a, 101);
  EXPECT_EQ(*b, 42);
}

TEST(IndirectTest, MemberSwap) {
  xyz::indirect<int> a(std::in_place, 42);
  xyz::indirect<int> b(std::in_place, 101);

  a.swap(b);
  EXPECT_EQ(*a, 101);
  EXPECT_EQ(*b, 42);
}

TEST(IndirectTest, ConstPropagation) {
  struct SomeType {
    enum class Constness { CONST, NON_CONST };
    Constness member() { return Constness::NON_CONST; }
    Constness member() const { return Constness::CONST; }
  };

  xyz::indirect<SomeType> a(std::in_place);
  EXPECT_EQ(a->member(), SomeType::Constness::NON_CONST);
  EXPECT_EQ((*a).member(), SomeType::Constness::NON_CONST);
  const auto& ca = a;
  EXPECT_EQ(ca->member(), SomeType::Constness::CONST);
  EXPECT_EQ((*ca).member(), SomeType::Constness::CONST);
}

TEST(IndirectTest, Hash) {
  xyz::indirect<int> a(std::in_place, 42);
  EXPECT_EQ(std::hash<xyz::indirect<int>>()(a), std::hash<int>()(*a));
}

TEST(IndirectTest, Optional) {
  std::optional<xyz::indirect<int>> a;
  EXPECT_FALSE(a.has_value());
  a.emplace(std::in_place, 42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(**a, 42);
}

#if false  // Indirect does not support == and != yet.
TEST(IndirectTest, Equality) {
  xyz::indirect<int> a(std::in_place, 42);
  xyz::indirect<int> b(std::in_place, 42);
  xyz::indirect<int> c(std::in_place, 43);
  EXPECT_EQ(a, a); // Same object.
  EXPECT_NE(a, b); // Same value.
  EXPECT_NE(a, c); // Different value.
}
#endif     // Indirect does not support == and != yet.

#if false  // Indirect does not support >, >=, <, <= yet.
TEST(IndirectTest, Comparison) {
  xyz::indirect<int> a(std::in_place, 42);
  xyz::indirect<int> b(std::in_place, 42);
  xyz::indirect<int> c(std::in_place, 43);
  EXPECT_FALSE(a < a);
  EXPECT_FALSE(a > a);
  EXPECT_TRUE(a <= a);
  EXPECT_TRUE(a >= a);

  EXPECT_FALSE(a < b);
  EXPECT_FALSE(a > b);
  EXPECT_TRUE(a <= b);
  EXPECT_TRUE(a >= b);

  EXPECT_FALSE(b < a);
  EXPECT_FALSE(b > a);
  EXPECT_TRUE(b <= a);
  EXPECT_TRUE(b >= a);

  EXPECT_TRUE(a < c);
  EXPECT_FALSE(a > c);
  EXPECT_TRUE(a <= c);
  EXPECT_FALSE(a >= c);

  EXPECT_FALSE(c < a);
  EXPECT_TRUE(c > a);
  EXPECT_FALSE(c <= a);
  EXPECT_TRUE(c >= a);
}
#endif     // Indirect does not support >, >=, <, <= yet.

#if (XYZ_USES_ALLOCATORS == 1)

template <typename T>
struct TrackingAllocator {
  unsigned* alloc_counter_;
  unsigned* dealloc_counter_;

  TrackingAllocator(unsigned* alloc_counter, unsigned* dealloc_counter)
      : alloc_counter_(alloc_counter), dealloc_counter_(dealloc_counter) {}

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

TEST(IndirectTest, CountAllocationsForInPlaceConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, TrackingAllocator<int>> a(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
  }
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

#endif  // XYZ_USES_ALLOCATORS == 1
