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

#include "indirect.h"

#include <gtest/gtest.h>

#include <array>
#include <map>
#if __has_include(<memory_resource>)
#include <memory_resource>
#endif  // #if __has_include(<memory_resource>)
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

TEST(IndirectTest, ValueAccessFromInPlaceConstructedObject) {
  xyz::indirect<int> a(std::in_place, 42);
  EXPECT_EQ(*a, 42);
}

TEST(IndirectTest, ValueAccessFromDefaultConstructedObject) {
  xyz::indirect<int> a;
  EXPECT_EQ(*a, 0);
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

  EXPECT_TRUE(a.valueless_after_move());
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

  EXPECT_TRUE(b.valueless_after_move());
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

TEST(IndirectTest, Equality) {
  xyz::indirect<int> a(std::in_place, 42);
  xyz::indirect<int> b(std::in_place, 42);
  xyz::indirect<int> c(std::in_place, 43);
  EXPECT_EQ(a, a);  // Same object.
  EXPECT_EQ(a, b);  // Same value.
  EXPECT_NE(a, c);  // Different value.
}

TEST(IndirectTest, Comparison) {
  xyz::indirect<int> a(std::in_place, 42);
  xyz::indirect<int> b(std::in_place, 42);
  xyz::indirect<int> c(std::in_place, 101);
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

TEST(IndirectTest, ComparisonWithU) {
  EXPECT_EQ(xyz::indirect<int>(std::in_place, 42), 42);
  EXPECT_EQ(42, xyz::indirect<int>(std::in_place, 42));

  EXPECT_NE(xyz::indirect<int>(std::in_place, 42), 101);
  EXPECT_NE(101, xyz::indirect<int>(std::in_place, 42));

  EXPECT_GT(xyz::indirect<int>(std::in_place, 101), 42);
  EXPECT_GT(101, xyz::indirect<int>(std::in_place, 42));

  EXPECT_GE(xyz::indirect<int>(std::in_place, 42), 42);
  EXPECT_GE(42, xyz::indirect<int>(std::in_place, 42));
  EXPECT_GE(xyz::indirect<int>(std::in_place, 101), 42);
  EXPECT_GE(101, xyz::indirect<int>(std::in_place, 42));

  EXPECT_LT(xyz::indirect<int>(std::in_place, 42), 101);
  EXPECT_LT(42, xyz::indirect<int>(std::in_place, 101));

  EXPECT_LE(xyz::indirect<int>(std::in_place, 42), 42);
  EXPECT_LE(42, xyz::indirect<int>(std::in_place, 42));
  EXPECT_LE(xyz::indirect<int>(std::in_place, 42), 101);
  EXPECT_LE(42, xyz::indirect<int>(std::in_place, 101));
}

TEST(IndirectTest, ComparisonWithIndirectU) {
  EXPECT_EQ(xyz::indirect<int>(std::in_place, 42),
            xyz::indirect<double>(std::in_place, 42));
  EXPECT_EQ(xyz::indirect<double>(std::in_place, 42),
            xyz::indirect<int>(std::in_place, 42));

  EXPECT_NE(xyz::indirect<int>(std::in_place, 42),
            xyz::indirect<double>(std::in_place, 101));
  EXPECT_NE(xyz::indirect<double>(std::in_place, 101),
            xyz::indirect<int>(std::in_place, 42));

  EXPECT_GT(xyz::indirect<int>(std::in_place, 101),
            xyz::indirect<double>(std::in_place, 42));
  EXPECT_GT(xyz::indirect<double>(std::in_place, 101),
            xyz::indirect<int>(std::in_place, 42));

  EXPECT_GE(xyz::indirect<int>(std::in_place, 42),
            xyz::indirect<double>(std::in_place, 42));
  EXPECT_GE(xyz::indirect<double>(std::in_place, 42),
            xyz::indirect<int>(std::in_place, 42));
  EXPECT_GE(xyz::indirect<int>(std::in_place, 101),
            xyz::indirect<double>(std::in_place, 42));
  EXPECT_GE(xyz::indirect<double>(std::in_place, 101),
            xyz::indirect<int>(std::in_place, 42));

  EXPECT_LT(xyz::indirect<int>(std::in_place, 42),
            xyz::indirect<double>(std::in_place, 101));
  EXPECT_LT(xyz::indirect<double>(std::in_place, 42),
            xyz::indirect<int>(std::in_place, 101));

  EXPECT_LE(xyz::indirect<int>(std::in_place, 42),
            xyz::indirect<double>(std::in_place, 42));
  EXPECT_LE(xyz::indirect<double>(std::in_place, 42),
            xyz::indirect<int>(std::in_place, 42));
  EXPECT_LE(xyz::indirect<int>(std::in_place, 42),
            xyz::indirect<double>(std::in_place, 101));
  EXPECT_LE(xyz::indirect<double>(std::in_place, 42),
            xyz::indirect<int>(std::in_place, 101));
}

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

  friend bool operator==(const TrackingAllocator& lhs,
                         const TrackingAllocator& rhs) noexcept {
    return lhs.alloc_counter_ == rhs.alloc_counter_ &&
           lhs.dealloc_counter_ == rhs.dealloc_counter_;
  }
};

TEST(IndirectTest, GetAllocator) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  TrackingAllocator<int> allocator(&alloc_counter, &dealloc_counter);

  xyz::indirect<int, TrackingAllocator<int>> a(std::allocator_arg, allocator,
                                               std::in_place, 42);
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 0);

  auto tracking_allocator = a.get_allocator();
  EXPECT_EQ(alloc_counter, *tracking_allocator.alloc_counter_);
  EXPECT_EQ(dealloc_counter, *tracking_allocator.dealloc_counter_);
}

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

TEST(IndirectTest, CountAllocationsForCopyConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, TrackingAllocator<int>> a(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
    xyz::indirect<int, TrackingAllocator<int>> b(a);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

TEST(IndirectTest, CountAllocationsForCopyAssignment) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, TrackingAllocator<int>> a(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        42);
    xyz::indirect<int, TrackingAllocator<int>> b(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = a;  // Will not allocate as int is assignable.
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

struct NonAssignable {
  int value;
  NonAssignable(int v) : value(v) {}
  NonAssignable(const NonAssignable&) = default;
  NonAssignable& operator=(const NonAssignable&) = delete;
};

TEST(IndirectTest, CountAllocationsForCopyAssignmentForNonAssignableT) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<NonAssignable, TrackingAllocator<NonAssignable>> a(
        std::allocator_arg,
        TrackingAllocator<NonAssignable>(&alloc_counter, &dealloc_counter),
        std::in_place, 42);
    xyz::indirect<NonAssignable, TrackingAllocator<NonAssignable>> b(
        std::allocator_arg,
        TrackingAllocator<NonAssignable>(&alloc_counter, &dealloc_counter),
        std::in_place, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = a;  // Will allocate.
    EXPECT_EQ(a->value, b->value);
  }
  EXPECT_EQ(alloc_counter, 3);
  EXPECT_EQ(dealloc_counter, 3);
}

TEST(IndirectTest, CountAllocationsForMoveAssignment) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, TrackingAllocator<int>> a(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        42);
    xyz::indirect<int, TrackingAllocator<int>> b(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = std::move(a);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

template <typename T>
struct NonEqualTrackingAllocator : TrackingAllocator<T> {
  using TrackingAllocator<T>::TrackingAllocator;
  friend bool operator==(const NonEqualTrackingAllocator&,
                         const NonEqualTrackingAllocator&) noexcept {
    return false;
  }
};

TEST(IndirectTest,
     CountAllocationsForMoveAssignmentWhenAllocatorsDontCompareEqual) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, NonEqualTrackingAllocator<int>> a(
        std::allocator_arg,
        NonEqualTrackingAllocator<int>(&alloc_counter, &dealloc_counter),
        std::in_place, 42);
    xyz::indirect<int, NonEqualTrackingAllocator<int>> b(
        std::allocator_arg,
        NonEqualTrackingAllocator<int>(&alloc_counter, &dealloc_counter),
        std::in_place, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = std::move(a);  // This will copy as allocators don't compare equal.
  }
  EXPECT_EQ(alloc_counter, 3);
  EXPECT_EQ(dealloc_counter, 3);
}

TEST(IndirectTest, CountAllocationsForAssignmentToMovedFromObject) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, TrackingAllocator<int>> a(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        42);
    xyz::indirect<int, TrackingAllocator<int>> b(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = std::move(a);
    EXPECT_EQ(dealloc_counter, 1);  // b's value is destroyed.
    xyz::indirect<int, TrackingAllocator<int>> c(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        404);
    EXPECT_TRUE(a.valueless_after_move());
    a = c;  // This will cause an allocation as a is valueless.
    EXPECT_EQ(alloc_counter, 4);
    EXPECT_EQ(dealloc_counter, 1);
  }
  EXPECT_EQ(alloc_counter, 4);
  EXPECT_EQ(dealloc_counter, 4);
}

TEST(IndirectTest, CountAllocationsForMoveConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, TrackingAllocator<int>> a(
        std::allocator_arg,
        TrackingAllocator<int>(&alloc_counter, &dealloc_counter), std::in_place,
        42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
    xyz::indirect<int, TrackingAllocator<int>> b(std::move(a));
  }
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

template <typename T>
struct POCSTrackingAllocator : TrackingAllocator<T> {
  using TrackingAllocator<T>::TrackingAllocator;
  using propagate_on_container_swap = std::true_type;
};

TEST(IndirectTest, NonMemberSwapWhenAllocatorsDontCompareEqual) {

  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, POCSTrackingAllocator<int>> a(
        std::allocator_arg,
        POCSTrackingAllocator<int>(&alloc_counter, &dealloc_counter),
        std::in_place, 42);
    xyz::indirect<int, POCSTrackingAllocator<int>> b(
        std::allocator_arg,
        POCSTrackingAllocator<int>(&alloc_counter, &dealloc_counter),
        std::in_place, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    swap(a, b);
    EXPECT_EQ(*a, 101);
    EXPECT_EQ(*b, 42);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

TEST(IndirectTest, MemberSwapWhenAllocatorsDontCompareEqual) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, POCSTrackingAllocator<int>> a(
        std::allocator_arg,
        POCSTrackingAllocator<int>(&alloc_counter, &dealloc_counter),
        std::in_place, 42);
    xyz::indirect<int, POCSTrackingAllocator<int>> b(
        std::allocator_arg,
        POCSTrackingAllocator<int>(&alloc_counter, &dealloc_counter),
        std::in_place, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    a.swap(b);
    EXPECT_EQ(*a, 101);
    EXPECT_EQ(*b, 42);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

struct ThrowsOnConstruction {
  class Exception : public std::exception {
    const char* what() const noexcept override {
      return "ThrowsOnConstruction::Exception";
    }
  };

  template <typename... Args>
  ThrowsOnConstruction(Args&&...) {
    throw Exception();
  }
};

struct ThrowsOnCopyConstruction {
  class Exception : public std::runtime_error {
   public:
    Exception() : std::runtime_error("ThrowsOnConstruction::Exception") {}
  };

  ThrowsOnCopyConstruction() = default;

  ThrowsOnCopyConstruction(const ThrowsOnCopyConstruction&) {
    throw Exception();
  }
};

TEST(IndirectTest, DefaultConstructorWithExceptions) {
  EXPECT_THROW(xyz::indirect<ThrowsOnConstruction>(),
               ThrowsOnConstruction::Exception);
}

TEST(IndirectTest, ConstructorWithExceptions) {
  EXPECT_THROW(xyz::indirect<ThrowsOnConstruction>(std::in_place, "unused"),
               ThrowsOnConstruction::Exception);
}

TEST(IndirectTest, CopyConstructorWithExceptions) {
  auto create_copy = []() {
    auto a = xyz::indirect<ThrowsOnCopyConstruction>(std::in_place);
    auto aa = a;
  };
  EXPECT_THROW(create_copy(), ThrowsOnCopyConstruction::Exception);
}

TEST(IndirectTest, ConstructorWithExceptionsTrackingAllocations) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  auto construct = [&]() {
    return xyz::indirect<ThrowsOnConstruction,
                         TrackingAllocator<ThrowsOnConstruction>>(
        std::allocator_arg,
        TrackingAllocator<ThrowsOnConstruction>(&alloc_counter,
                                                &dealloc_counter),
        std::in_place, "unused");
  };
  EXPECT_THROW(construct(), ThrowsOnConstruction::Exception);
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

TEST(IndirectTest, InteractionWithOptional) {
  std::optional<xyz::indirect<int>> a;
  EXPECT_FALSE(a.has_value());
  a.emplace(std::in_place, 42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(**a, 42);
}

TEST(IndirectTest, InteractionWithVector) {
  std::vector<xyz::indirect<int>> as;
  for (int i = 0; i < 16; ++i) {
    as.push_back(xyz::indirect<int>(std::in_place, i));
  }
  for (int i = 0; i < 16; ++i) {
    EXPECT_EQ(*as[i], i);
  }
}

TEST(IndirectTest, InteractionWithMap) {
  std::map<int, xyz::indirect<int>> as;
  for (int i = 0; i < 16; ++i) {
    as.emplace(i, xyz::indirect<int>(std::in_place, i));
  }
  for (auto [k, v] : as) {
    EXPECT_EQ(*v, k);
  }
}

TEST(IndirectTest, InteractionWithUnorderedMap) {
  std::unordered_map<int, xyz::indirect<int>> as;
  for (int i = 0; i < 16; ++i) {
    as.emplace(i, xyz::indirect<int>(std::in_place, i));
  }
  for (auto [k, v] : as) {
    EXPECT_EQ(*v, k);
  }
}

TEST(IndirectTest, InteractionWithSizedAllocators) {
  // Admit defeat... gtest does not seem to support STATIC_REQUIRES equivelent
  // functionality.
  EXPECT_EQ(sizeof(xyz::indirect<int>), sizeof(int*));
  EXPECT_EQ(sizeof(xyz::indirect<int, TrackingAllocator<int>>),
            (sizeof(int*) + sizeof(TrackingAllocator<int>)));
}

#if (__cpp_lib_memory_resource >= 201603L)
TEST(IndirectTest, InteractionWithPMRAllocators) {
  std::array<std::byte, 1024> buffer;
  std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
  std::pmr::polymorphic_allocator<int> pa{&mbr};
  using IndirectInt = xyz::indirect<int, std::pmr::polymorphic_allocator<int>>;
  IndirectInt a(std::allocator_arg, pa, std::in_place, 42);
  std::pmr::vector<IndirectInt> values{pa};
  values.push_back(a);
  values.push_back(std::move(a));
  EXPECT_EQ(*values[0], 42);
}

TEST(IndirectTest, InteractionWithPMRAllocatorsWhenCopyThrows) {
  std::array<std::byte, 1024> buffer;
  std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
  std::pmr::polymorphic_allocator<ThrowsOnCopyConstruction> pa{&mbr};
  using IndirectType =
      xyz::indirect<ThrowsOnCopyConstruction,
                    std::pmr::polymorphic_allocator<ThrowsOnCopyConstruction>>;
  IndirectType a(std::allocator_arg, pa, std::in_place);
  std::pmr::vector<IndirectType> values{pa};
  EXPECT_THROW(values.push_back(a), ThrowsOnCopyConstruction::Exception);
}

TEST(IndirectTest, HashCustomAllocator) {
  std::array<std::byte, 1024> buffer;
  std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
  std::pmr::polymorphic_allocator<int> pa{&mbr};
  using IndirectType = xyz::indirect<int, std::pmr::polymorphic_allocator<int>>;
  IndirectType a(std::allocator_arg, pa, std::in_place, 42);
  EXPECT_EQ(std::hash<IndirectType>()(a), std::hash<int>()(*a));
}
#endif  // (__cpp_lib_memory_resource >= 201603L)

#if (__cpp_lib_format >= 201907L)

TEST(IndirectTest, FormatNativeTypesDefaultFormatting) {
  EXPECT_EQ(std::format("{}", xyz::indirect<bool>(std::in_place, true)), "true");
  EXPECT_EQ(std::format("{}", xyz::indirect<int>(std::in_place, 100)), "100");
  EXPECT_EQ(std::format("{}", xyz::indirect<float>(std::in_place, 50.0f)), "50");
  EXPECT_EQ(std::format("{}", xyz::indirect<double>(std::in_place, 25.0)), "25");
}

TEST(IndirectTest, FormatNativeTypesPropagateFormatting) {
  EXPECT_EQ(std::format("{:*<6}", xyz::indirect<bool>(std::in_place, true)), "true**");
  EXPECT_EQ(std::format("{:*^7}", xyz::indirect<int>(std::in_place, 100)), "**100**");
  EXPECT_EQ(std::format("{:>7}", xyz::indirect<float>(std::in_place, 50.0f)), "     50");
  EXPECT_EQ(std::format("{:+8.3e}", xyz::indirect<double>(std::in_place, 25.75)), "+2.575e+01");
}

#endif  // __cpp_lib_format >= 201907L

}  // namespace
