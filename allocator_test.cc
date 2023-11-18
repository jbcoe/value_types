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

#include <memory>
#include <scoped_allocator>
#include <utility>

#include "indirect.h"
#include "polymorphic.h"

namespace xyz::allocator_testing {

struct SimpleType {};

struct FussyType {
  template <typename... Ts>
  FussyType(Ts&&... ts) {
    throw std::runtime_error("FussyType must be allocator-constructed");
  }

  // Allocator-extended constructor.
  template <typename Allocator, typename... Ts>
  FussyType(std::allocator_arg_t, const Allocator& a, Ts&&...) {}
};

struct FussyOldType {
  FussyOldType(int i) {
    throw std::runtime_error("FussyOldType must be allocator-constructed");
  }

  // Old-style allocator constructor.
  template <typename Allocator>
  FussyOldType(int i, const Allocator& a) {}
};

}  // namespace xyz::allocator_testing

namespace std {
template <typename Allocator>
struct uses_allocator<xyz::allocator_testing::FussyType, Allocator>
    : true_type {};

template <typename Allocator>
struct uses_allocator<xyz::allocator_testing::FussyOldType, Allocator>
    : true_type {};
}  // namespace std

template <typename T>
class TestAllocator {
 public:
  using value_type = T;
  using size_type = std::size_t;

  T* allocate(std::size_t n) {
    std::allocator<T> default_allocator{};
    return default_allocator.allocate(n);
  }

  void deallocate(T* p, std::size_t n) {
    std::allocator<T> default_allocator{};
    default_allocator.deallocate(p, n);
  }
};

namespace xyz::allocator_testing {

TEST(AllocatorTest, SimpleTypeUsesAllocator) {
  static_assert(
      !std::uses_allocator<SimpleType, std::allocator<SimpleType>>::value);
}

TEST(AllocatorTest, FussyTypeUsesAllocator) {
  static_assert(
      std::uses_allocator<FussyType, std::allocator<FussyType>>::value);
}

TEST(AllocatorTest, FussyOldTypeUsesAllocator) {
  static_assert(
      std::uses_allocator<FussyOldType, std::allocator<FussyOldType>>::value);
  static_assert(
      std::is_constructible_v<FussyOldType, int, std::allocator<FussyOldType>>);
}

TEST(AllocatorTest, SimpleTypeAllocatorConstruction) {
  std::allocator<SimpleType> a;
  [[maybe_unused]] auto f = std::make_obj_using_allocator<SimpleType>(a);
}

TEST(AllocatorTest, FussyTypeAllocatorConstruction) {
  std::allocator<FussyType> a;
  [[maybe_unused]] auto f = std::make_obj_using_allocator<FussyType>(a);
}

TEST(AllocatorTest, FussyOldTypeAllocatorConstruction) {
  std::allocator<FussyOldType> a;
  [[maybe_unused]] auto f = std::make_obj_using_allocator<FussyOldType>(a, 42);
}

TEST(AllocatorTestPolymorphic, SimpleTypeMustBeAllocatorConstructed) {
  auto p = xyz::polymorphic<SimpleType>();
  EXPECT_FALSE(p.valueless_after_move());
}

TEST(AllocatorTestPolymorphic, FussyTypeMustBeAllocatorConstructed) {
  auto p = xyz::polymorphic<FussyType>();
  EXPECT_FALSE(p.valueless_after_move());
}

TEST(AllocatorTestPolymorphic, FussyOldTypeMustBeAllocatorConstructed) {
  auto p = xyz::polymorphic<FussyOldType>(std::in_place_type<FussyOldType>, 42);
  EXPECT_FALSE(p.valueless_after_move());
}

TEST(AllocatorTestIndirect, SimpleTypeMustBeAllocatorConstructed) {
  auto p = xyz::indirect<SimpleType>();
  EXPECT_FALSE(p.valueless_after_move());
}

TEST(AllocatorTestIndirect, FussyTypeMustBeAllocatorConstructed) {
  auto p = xyz::indirect<FussyType>();
  EXPECT_FALSE(p.valueless_after_move());
}

TEST(AllocatorTestIndirect, FussyOldTypeMustBeAllocatorConstructed) {
  auto p = xyz::indirect<FussyOldType>(42);
  EXPECT_FALSE(p.valueless_after_move());
}

}  // namespace xyz::allocator_testing
