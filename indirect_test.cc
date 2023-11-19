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

#ifdef XYZ_INDIRECT_CXX_14
#include "indirect_cxx14.h"
#endif  // XYZ_INDIRECT_CXX_14

#ifndef XYZ_INDIRECT_H
#include "indirect.h"
#endif  // XYZ_INDIRECT_H

#include <gtest/gtest.h>

#include <array>
#include <map>

#include "feature_check.h"
#include "tracking_allocator.h"
#ifdef XYZ_HAS_STD_MEMORY_RESOURCE
#include <memory_resource>
#endif  // XYZ_HAS_STD_MEMORY_RESOURCE
#ifdef XYZ_HAS_STD_OPTIONAL
#include <optional>
#endif  // XYZ_HAS_STD_OPTIONAL
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

TEST(IndirectTest, ValueAccessFromInPlaceConstructedObject) {
  xyz::indirect<int> a(42);
  EXPECT_EQ(*a, 42);
}

TEST(IndirectTest, ValueAccessFromDefaultConstructedObject) {
  xyz::indirect<int> a;
  EXPECT_EQ(*a, 0);
}

template <typename Allocator = std::allocator<void>>
struct AllocatorAwareType {
  using allocator_type = typename std::allocator_traits<
      Allocator>::template rebind_alloc<AllocatorAwareType>;

  AllocatorAwareType() = default;
  AllocatorAwareType(std::allocator_arg_t, const allocator_type& alloc)
      : children(alloc) {}

  std::vector<AllocatorAwareType, allocator_type> children;
};

template <typename T>
struct DefaultConstructibleTrackingAllocator : xyz::TrackingAllocator<T> {
  using xyz::TrackingAllocator<T>::TrackingAllocator;

  unsigned unused = 0;

  DefaultConstructibleTrackingAllocator()
      : xyz::TrackingAllocator<T>(&unused, &unused) {}

  template <typename Other>
  struct rebind {
    using other = DefaultConstructibleTrackingAllocator<Other>;
  };
};

TEST(IndirectTest, ConstructorDipatchesToAllocatorArgTOverload) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  AllocatorAwareType<DefaultConstructibleTrackingAllocator<void>>::
      allocator_type alloc(&alloc_counter, &dealloc_counter);
  using AllocatorAwareT =
      AllocatorAwareType<DefaultConstructibleTrackingAllocator<void>>;
  xyz::indirect<
      AllocatorAwareT,
      typename std::allocator_traits<DefaultConstructibleTrackingAllocator<
          void>>::template rebind_alloc<AllocatorAwareT>>
      value(std::allocator_arg, alloc);

  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 0);
}

TEST(IndirectTest, CopiesAreDistinct) {
  xyz::indirect<int> a(42);
  auto aa = a;
  EXPECT_EQ(*a, *aa);
  EXPECT_NE(&*a, &*aa);
}

TEST(IndirectTest, MovePreservesIndirectObjectAddress) {
  xyz::indirect<int> a(42);
  auto address = &*a;
  auto aa = std::move(a);

  EXPECT_TRUE(a.valueless_after_move());
  EXPECT_EQ(address, &*aa);
}

TEST(IndirectTest, CopyAssignment) {
  xyz::indirect<int> a(42);
  xyz::indirect<int> b(101);
  EXPECT_EQ(*a, 42);
  a = b;

  EXPECT_EQ(*a, 101);
  EXPECT_NE(&*a, &*b);
}

TEST(IndirectTest, CopyAssignmentSelf) {
  xyz::indirect<int> a(42);
  a = a;

  EXPECT_FALSE(a.valueless_after_move());
}

TEST(IndirectTest, MoveAssignment) {
  xyz::indirect<int> a(42);
  xyz::indirect<int> b(101);
  EXPECT_EQ(*a, 42);
  a = std::move(b);

  EXPECT_TRUE(b.valueless_after_move());
  EXPECT_EQ(*a, 101);
}

TEST(IndirectTest, MoveAssignmentSelf) {
  xyz::indirect<int> a(42);
  a = std::move(a);

  EXPECT_FALSE(a.valueless_after_move());
}

TEST(IndirectTest, NonMemberSwap) {
  xyz::indirect<int> a(42);
  xyz::indirect<int> b(101);
  using std::swap;
  swap(a, b);
  EXPECT_EQ(*a, 101);
  EXPECT_EQ(*b, 42);
}

TEST(IndirectTest, MemberSwap) {
  xyz::indirect<int> a(42);
  xyz::indirect<int> b(101);

  a.swap(b);
  EXPECT_EQ(*a, 101);
  EXPECT_EQ(*b, 42);
}

TEST(IndirectTest, MemberSwapWithSelf) {
  xyz::indirect<int> a(42);

  a.swap(a);
  EXPECT_FALSE(a.valueless_after_move());
}

TEST(IndirectTest, ConstPropagation) {
  struct SomeType {
    enum class Constness { LV_CONST, LV_NON_CONST, RV_CONST, RV_NON_CONST };
    Constness member() & { return Constness::LV_NON_CONST; }
    Constness member() const& { return Constness::LV_CONST; }
    Constness member() && { return Constness::RV_NON_CONST; }
    Constness member() const&& { return Constness::RV_CONST; }
  };

  xyz::indirect<SomeType> a;
  EXPECT_EQ(a->member(), SomeType::Constness::LV_NON_CONST);
  EXPECT_EQ((*a).member(), SomeType::Constness::LV_NON_CONST);
  const auto& ca = a;
  EXPECT_EQ(ca->member(), SomeType::Constness::LV_CONST);
  EXPECT_EQ((*ca).member(), SomeType::Constness::LV_CONST);

  auto createConstRValueIndirect = [&]() -> xyz::indirect<SomeType> const&& {
    return std::move(a);
  };

  EXPECT_EQ((*xyz::indirect<SomeType>{}).member(),
            SomeType::Constness::RV_NON_CONST);
  EXPECT_EQ((*createConstRValueIndirect()).member(),
            SomeType::Constness::RV_CONST);
}

TEST(IndirectTest, Hash) {
  xyz::indirect<int> a(42);
  EXPECT_EQ(std::hash<xyz::indirect<int>>()(a), std::hash<int>()(*a));
}

#ifdef XYZ_HAS_STD_OPTIONAL
TEST(IndirectTest, Optional) {
  std::optional<xyz::indirect<int>> a;
  EXPECT_FALSE(a.has_value());
  a.emplace(42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(**a, 42);
}
#endif  // XYZ_HAS_STD_OPTIONAL

TEST(IndirectTest, Equality) {
  xyz::indirect<int> a(42);
  xyz::indirect<int> b(42);
  xyz::indirect<int> c(43);
  EXPECT_EQ(a, a);  // Same object.
  EXPECT_EQ(a, b);  // Same value.
  EXPECT_NE(a, c);  // Different value.
}

TEST(IndirectTest, Comparison) {
  xyz::indirect<int> a(42);
  xyz::indirect<int> b(42);
  xyz::indirect<int> c(101);
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
  EXPECT_EQ(xyz::indirect<int>(42), 42);
  EXPECT_EQ(42, xyz::indirect<int>(42));

  EXPECT_NE(xyz::indirect<int>(42), 101);
  EXPECT_NE(101, xyz::indirect<int>(42));

  EXPECT_GT(xyz::indirect<int>(101), 42);
  EXPECT_GT(101, xyz::indirect<int>(42));

  EXPECT_GE(xyz::indirect<int>(42), 42);
  EXPECT_GE(42, xyz::indirect<int>(42));
  EXPECT_GE(xyz::indirect<int>(101), 42);
  EXPECT_GE(101, xyz::indirect<int>(42));

  EXPECT_LT(xyz::indirect<int>(42), 101);
  EXPECT_LT(42, xyz::indirect<int>(101));

  EXPECT_LE(xyz::indirect<int>(42), 42);
  EXPECT_LE(42, xyz::indirect<int>(42));
  EXPECT_LE(xyz::indirect<int>(42), 101);
  EXPECT_LE(42, xyz::indirect<int>(101));
}

TEST(IndirectTest, ComparisonWithIndirectU) {
  EXPECT_EQ(xyz::indirect<int>(42), xyz::indirect<double>(42));
  EXPECT_EQ(xyz::indirect<double>(42), xyz::indirect<int>(42));

  EXPECT_NE(xyz::indirect<int>(42), xyz::indirect<double>(101));
  EXPECT_NE(xyz::indirect<double>(101), xyz::indirect<int>(42));

  EXPECT_GT(xyz::indirect<int>(101), xyz::indirect<double>(42));
  EXPECT_GT(xyz::indirect<double>(101), xyz::indirect<int>(42));

  EXPECT_GE(xyz::indirect<int>(42), xyz::indirect<double>(42));
  EXPECT_GE(xyz::indirect<double>(42), xyz::indirect<int>(42));
  EXPECT_GE(xyz::indirect<int>(101), xyz::indirect<double>(42));
  EXPECT_GE(xyz::indirect<double>(101), xyz::indirect<int>(42));

  EXPECT_LT(xyz::indirect<int>(42), xyz::indirect<double>(101));
  EXPECT_LT(xyz::indirect<double>(42), xyz::indirect<int>(101));

  EXPECT_LE(xyz::indirect<int>(42), xyz::indirect<double>(42));
  EXPECT_LE(xyz::indirect<double>(42), xyz::indirect<int>(42));
  EXPECT_LE(xyz::indirect<int>(42), xyz::indirect<double>(101));
  EXPECT_LE(xyz::indirect<double>(42), xyz::indirect<int>(101));
}

TEST(IndirectTest, GetAllocator) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  xyz::TrackingAllocator<int> allocator(&alloc_counter, &dealloc_counter);

  xyz::indirect<int, xyz::TrackingAllocator<int>> a(std::allocator_arg,
                                                    allocator, 42);
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
    xyz::indirect<int, xyz::TrackingAllocator<int>> a(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 42);
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
    xyz::indirect<int, xyz::TrackingAllocator<int>> a(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
    xyz::indirect<int, xyz::TrackingAllocator<int>> b(a);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

TEST(IndirectTest, CountAllocationsForCopyAssignment) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, xyz::TrackingAllocator<int>> a(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 42);
    xyz::indirect<int, xyz::TrackingAllocator<int>> b(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = a;  // Will not allocate as int is assignable.
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

TEST(IndirectTest, CountAllocationsForMoveAssignment) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, xyz::TrackingAllocator<int>> a(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 42);
    xyz::indirect<int, xyz::TrackingAllocator<int>> b(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = std::move(a);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

template <typename T>
struct NonEqualTrackingAllocator : xyz::TrackingAllocator<T> {
  using xyz::TrackingAllocator<T>::TrackingAllocator;
  using propagate_on_container_move_assignment = std::true_type;

  friend bool operator==(const NonEqualTrackingAllocator&,
                         const NonEqualTrackingAllocator&) noexcept {
    return false;
  }

  friend bool operator!=(const NonEqualTrackingAllocator&,
                         const NonEqualTrackingAllocator&) noexcept {
    return true;
  }
};

TEST(IndirectTest,
     CountAllocationsForMoveAssignmentWhenAllocatorsDontCompareEqual) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, NonEqualTrackingAllocator<int>> a(
        std::allocator_arg,
        NonEqualTrackingAllocator<int>(&alloc_counter, &dealloc_counter), 42);
    xyz::indirect<int, NonEqualTrackingAllocator<int>> b(
        std::allocator_arg,
        NonEqualTrackingAllocator<int>(&alloc_counter, &dealloc_counter), 101);
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
    xyz::indirect<int, xyz::TrackingAllocator<int>> a(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 42);
    xyz::indirect<int, xyz::TrackingAllocator<int>> b(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = std::move(a);
    EXPECT_EQ(dealloc_counter, 1);  // b's value is destroyed.
    xyz::indirect<int, xyz::TrackingAllocator<int>> c(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 404);
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
    xyz::indirect<int, xyz::TrackingAllocator<int>> a(
        std::allocator_arg,
        xyz::TrackingAllocator<int>(&alloc_counter, &dealloc_counter), 42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
    xyz::indirect<int, xyz::TrackingAllocator<int>> b(std::move(a));
  }
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

template <typename T>
struct POCSTrackingAllocator : xyz::TrackingAllocator<T> {
  using xyz::TrackingAllocator<T>::TrackingAllocator;
  using propagate_on_container_swap = std::true_type;
};

TEST(IndirectTest, NonMemberSwapWhenAllocatorsDontCompareEqual) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::indirect<int, POCSTrackingAllocator<int>> a(
        std::allocator_arg,
        POCSTrackingAllocator<int>(&alloc_counter, &dealloc_counter), 42);
    xyz::indirect<int, POCSTrackingAllocator<int>> b(
        std::allocator_arg,
        POCSTrackingAllocator<int>(&alloc_counter, &dealloc_counter), 101);
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
        POCSTrackingAllocator<int>(&alloc_counter, &dealloc_counter), 42);
    xyz::indirect<int, POCSTrackingAllocator<int>> b(
        std::allocator_arg,
        POCSTrackingAllocator<int>(&alloc_counter, &dealloc_counter), 101);
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

  ThrowsOnConstruction() { throw Exception(); }
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
  EXPECT_THROW(xyz::indirect<ThrowsOnConstruction>(),
               ThrowsOnConstruction::Exception);
}

TEST(IndirectTest, CopyConstructorWithExceptions) {
  auto create_copy = []() {
    auto a = xyz::indirect<ThrowsOnCopyConstruction>();
    auto aa = a;
  };
  EXPECT_THROW(create_copy(), ThrowsOnCopyConstruction::Exception);
}

TEST(IndirectTest, ConstructorWithExceptionsTrackingAllocations) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  auto construct = [&]() {
    return xyz::indirect<ThrowsOnConstruction,
                         xyz::TrackingAllocator<ThrowsOnConstruction>>(
        std::allocator_arg, xyz::TrackingAllocator<ThrowsOnConstruction>(
                                &alloc_counter, &dealloc_counter));
  };
  EXPECT_THROW(construct(), ThrowsOnConstruction::Exception);
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

#ifdef XYZ_HAS_STD_OPTIONAL
TEST(IndirectTest, InteractionWithOptional) {
  std::optional<xyz::indirect<int>> a;
  EXPECT_FALSE(a.has_value());
  a.emplace(42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ(**a, 42);
}
#endif  // XYZ_HAS_STD_OPTIONAL

TEST(IndirectTest, InteractionWithVector) {
  std::vector<xyz::indirect<int>> as;
  for (int i = 0; i < 16; ++i) {
    as.push_back(xyz::indirect<int>(i));
  }
  for (int i = 0; i < 16; ++i) {
    EXPECT_EQ(*as[i], i);
  }
}

TEST(IndirectTest, InteractionWithMap) {
  std::map<int, xyz::indirect<int>> as;
  for (int i = 0; i < 16; ++i) {
    as.emplace(i, xyz::indirect<int>(i));
  }
  for (const auto& kv : as) {
    EXPECT_EQ(*kv.second, kv.first);
  }
}

TEST(IndirectTest, InteractionWithUnorderedMap) {
  std::unordered_map<int, xyz::indirect<int>> as;
  for (int i = 0; i < 16; ++i) {
    as.emplace(i, xyz::indirect<int>(i));
  }
  for (const auto& kv : as) {
    EXPECT_EQ(*kv.second, kv.first);
  }
}

TEST(IndirectTest, InteractionWithSizedAllocators) {
  // Admit defeat... gtest does not seem to support STATIC_REQUIRES equivelent
  // functionality.
  EXPECT_EQ(sizeof(xyz::indirect<int>), sizeof(int*));
  EXPECT_EQ(sizeof(xyz::indirect<int, xyz::TrackingAllocator<int>>),
            (sizeof(int*) + sizeof(xyz::TrackingAllocator<int>)));
}

#ifdef XYZ_HAS_STD_MEMORY_RESOURCE
TEST(IndirectTest, InteractionWithPMRAllocators) {
  std::array<std::byte, 1024> buffer;
  std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
  std::pmr::polymorphic_allocator<int> pa{&mbr};
  using IndirectInt = xyz::indirect<int, std::pmr::polymorphic_allocator<int>>;
  IndirectInt a(std::allocator_arg, pa, 42);
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
  IndirectType a(std::allocator_arg, pa);
  std::pmr::vector<IndirectType> values{pa};
  EXPECT_THROW(values.push_back(a), ThrowsOnCopyConstruction::Exception);
}

TEST(IndirectTest, HashCustomAllocator) {
  std::array<std::byte, 1024> buffer;
  std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
  std::pmr::polymorphic_allocator<int> pa{&mbr};
  using IndirectType = xyz::indirect<int, std::pmr::polymorphic_allocator<int>>;
  IndirectType a(std::allocator_arg, pa, 42);
  EXPECT_EQ(std::hash<IndirectType>()(a), std::hash<int>()(*a));
}
#endif  // XYZ_HAS_STD_MEMORY_RESOURCE

#if (__cpp_lib_format >= 201907L)

TEST(IndirectTest, FormatNativeTypesDefaultFormatting) {
  EXPECT_EQ(std::format("{}", xyz::indirect<bool>(true)), "true");
  EXPECT_EQ(std::format("{}", xyz::indirect<int>(100)), "100");
  EXPECT_EQ(std::format("{}", xyz::indirect<float>(50.0f)), "50");
  EXPECT_EQ(std::format("{}", xyz::indirect<double>(25.0)), "25");
}

TEST(IndirectTest, FormatNativeTypesPropagateFormatting) {
  EXPECT_EQ(std::format("{:*<6}", xyz::indirect<bool>(true)), "true**");
  EXPECT_EQ(std::format("{:*^7}", xyz::indirect<int>(100)), "**100**");
  EXPECT_EQ(std::format("{:>7}", xyz::indirect<float>(50.0f)), "     50");
  EXPECT_EQ(std::format("{:+8.3e}", xyz::indirect<double>(25.75)),
            "+2.575e+01");
}

#endif  // __cpp_lib_format >= 201907L
}  // namespace
