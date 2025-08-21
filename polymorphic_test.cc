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

#ifdef XYZ_POLYMORPHIC_CXX_14
#include "polymorphic_cxx14.h"
#endif  // XYZ_POLYMORPHIC_CXX_14

#ifdef XYZ_POLYMORPHIC_NO_VTABLE
#include "polymorphic_no_vtable.h"
#endif  // XYZ_POLYMORPHIC_NO_VTABLE

#ifdef XYZ_POLYMORPHIC_SBO
#include "polymorphic_sbo.h"
#endif  // XYZ_POLYMORPHIC_SBO

#ifndef XYZ_POLYMORPHIC_H_
#include "polymorphic.h"
#endif  // XYZ_POLYMORPHIC_H_

#include <gtest/gtest.h>

#include <array>
#include <map>
#include <utility>

#include "feature_check.h"
#include "tagged_allocator.h"
#include "test_helpers.h"
#include "tracking_allocator.h"
#if defined(XYZ_HAS_STD_IN_PLACE_TYPE_T) && !defined(XYZ_POLYMORPHIC_CXX_14)
namespace xyz {
using std::in_place_type_t;
}  // namespace xyz
#endif  // XYZ_HAS_STD_IN_PLACE_TYPE_T

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

class Base {
 public:
  virtual ~Base() = default;
  virtual int value() const = 0;
  virtual void set_value(int) = 0;
};

class Derived : public Base {
 private:
  int value_;

 public:
  Derived(int v) : value_(v) {}

  Derived() : Derived(0) {}

  int value() const override { return value_; }

  void set_value(int v) override { value_ = v; }
};

TEST(PolymorphicTest, DefaultConstructor) {
  xyz::polymorphic<Derived> p;
  EXPECT_EQ(p->value(), 0);
}

TEST(PolymorphicTest, AllocatorExtendedDefaultConstructor) {
  xyz::TaggedAllocator<Derived> a(42);
  xyz::polymorphic<Derived, xyz::TaggedAllocator<Derived>> p(std::allocator_arg,
                                                             a);
  EXPECT_EQ(p->value(), 0);
  EXPECT_EQ(p.get_allocator(), a);
}

TEST(PolymorphicTest, SingleLValueConstructor) {
  Derived d(42);
  xyz::polymorphic<Base> p(d);
  EXPECT_EQ(p->value(), 42);
}

TEST(PolymorphicTest, AllocatorExtendedSingleLValueConstructor) {
  xyz::TaggedAllocator<Base> a(42);
  Derived d(42);
  xyz::polymorphic<Base, xyz::TaggedAllocator<Base>> p(std::allocator_arg, a,
                                                       d);
  EXPECT_EQ(p->value(), 42);
  EXPECT_EQ(p.get_allocator(), a);
}

TEST(PolymorphicTest, SingleRValueConstructor) {
  xyz::polymorphic<Base> p(Derived(42));
  EXPECT_EQ(p->value(), 42);
}

TEST(PolymorphicTest, AllocatorExtendedSingleRValueConstructor) {
  xyz::TaggedAllocator<Base> a(42);
  xyz::polymorphic<Base, xyz::TaggedAllocator<Base>> p(std::allocator_arg, a,
                                                       Derived(42));
  EXPECT_EQ(p->value(), 42);
  EXPECT_EQ(p.get_allocator(), a);
}

TEST(PolymorphicTest, InPlaceConstructor) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  EXPECT_EQ(p->value(), 42);
}

TEST(PolymorphicTest, AllocatorExtendedInPlaceConstructor) {
  xyz::TaggedAllocator<Base> a(42);
  xyz::polymorphic<Base, xyz::TaggedAllocator<Base>> p(
      std::allocator_arg, a, xyz::in_place_type_t<Derived>{}, 42);
  EXPECT_EQ(p->value(), 42);
  EXPECT_EQ(p.get_allocator(), a);
}

#ifdef XYZ_POLYMORPHIC_HAS_EXTENDED_CONSTRUCTORS
TEST(PolymorphicTest, InitializerListConstructor) {
  xyz::polymorphic<std::vector<int>> p(xyz::in_place_type_t<std::vector<int>>{},
                                       {10, 11});
  EXPECT_EQ(p->size(), 2);
}

TEST(PolymorphicTest, AllocatorExtendedInitializerListConstructor) {
  xyz::TaggedAllocator<std::vector<int>> a(42);
  xyz::polymorphic<std::vector<int>, xyz::TaggedAllocator<std::vector<int>>> p(
      std::allocator_arg, a, xyz::in_place_type_t<std::vector<int>>{},
      {10, 11});
  EXPECT_EQ(p->size(), 2);
  EXPECT_EQ(p.get_allocator(), a);
}
#endif  // XYZ_POLYMORPHIC_HAS_EXTENDED_CONSTRUCTORS

TEST(PolymorphicTest, CopiesAreDistinct) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  auto pp = p;
  EXPECT_EQ(p->value(), pp->value());
  EXPECT_NE(&*p, &*pp);
}

TEST(PolymorphicTest, MoveRendersSourceValueless) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  auto pp = std::move(p);
  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, AllocatorExtendedCopy) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(std::allocator_arg, p.get_allocator(), p);
  EXPECT_EQ(p->value(), pp->value());
  EXPECT_NE(&*p, &*pp);
}

TEST(PolymorphicTest, AllocatorExtendedMove) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(std::allocator_arg, p.get_allocator(),
                            std::move(p));
  EXPECT_EQ(pp->value(), 42);
  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, Swap) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(xyz::in_place_type_t<Derived>{}, 101);
  EXPECT_EQ(p->value(), 42);
  EXPECT_EQ(pp->value(), 101);
  swap(p, pp);
  EXPECT_EQ(p->value(), 101);
  EXPECT_EQ(pp->value(), 42);
}

TEST(PolymorphicTest, SwapWithNoSBOAndSBO) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(xyz::in_place_type_t<Derived>{}, 101);
  EXPECT_EQ(p->value(), 42);
  EXPECT_EQ(pp->value(), 101);
  swap(p, pp);
  EXPECT_EQ(p->value(), 101);
  EXPECT_EQ(pp->value(), 42);
}

TEST(PolymorphicTest, AccessDerivedObject) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  EXPECT_EQ(p->value(), 42);
}

TEST(PolymorphicTest, CopiesOfDerivedObjectsAreDistinct) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  auto pp = p;
  EXPECT_EQ(p->value(), pp->value());
  pp->set_value(101);
  EXPECT_NE(p->value(), pp->value());
}

TEST(PolymorphicTest, CopyAssignment) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(xyz::in_place_type_t<Derived>{}, 101);
  EXPECT_EQ(p->value(), 42);
  p = pp;

  EXPECT_EQ(p->value(), 101);
  EXPECT_NE(&*p, &*pp);
}

TEST(PolymorphicTest, CopyAssignmentSelf) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  p = p;

  EXPECT_FALSE(p.valueless_after_move());
}

TEST(PolymorphicTest, MoveAssignment) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(xyz::in_place_type_t<Derived>{}, 101);
  EXPECT_EQ(p->value(), 42);
  p = std::move(pp);

  EXPECT_TRUE(
      pp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  EXPECT_EQ(p->value(), 101);
}

TEST(PolymorphicTest, MoveAssignmentSelf) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  p = std::move(p);

  EXPECT_FALSE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, NonMemberSwap) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(xyz::in_place_type_t<Derived>{}, 101);
  using std::swap;
  swap(p, pp);
  EXPECT_EQ(p->value(), 101);
  EXPECT_EQ(pp->value(), 42);
}

TEST(PolymorphicTest, MemberSwap) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(xyz::in_place_type_t<Derived>{}, 101);

  p.swap(pp);
  EXPECT_EQ(p->value(), 101);
  EXPECT_EQ(pp->value(), 42);
}

TEST(PolymorphicTest, MemberSwapWithSelf) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);

  p.swap(p);
  EXPECT_FALSE(p.valueless_after_move());
}

TEST(PolymorphicTest, CopyFromValueless) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(std::move(p));

  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  xyz::polymorphic<Base> ppp(p);
  EXPECT_TRUE(
      ppp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, MoveFromValueless) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(std::move(p));

  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  xyz::polymorphic<Base> ppp(std::move(p));
  EXPECT_TRUE(
      ppp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, AllocatorExtendedCopyFromValueless) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(std::move(p));

  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  xyz::polymorphic<Base> ppp(std::allocator_arg, p.get_allocator(), p);
  EXPECT_TRUE(
      ppp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, AllocatorExtendedMoveFromValueless) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(std::move(p));

  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  xyz::polymorphic<Base> ppp(std::allocator_arg, p.get_allocator(),
                             std::move(p));
  EXPECT_TRUE(
      ppp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, AssignFromValueless) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(std::move(p));

  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  xyz::polymorphic<Base> ppp(xyz::in_place_type_t<Derived>{}, 101);
  ppp = p;
  EXPECT_TRUE(
      ppp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, MoveAssignFromValueless) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(std::move(p));

  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  xyz::polymorphic<Base> ppp(xyz::in_place_type_t<Derived>{}, 101);
  ppp = std::move(p);
  EXPECT_TRUE(
      ppp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, SwapFromValueless) {
  xyz::polymorphic<Base> p(xyz::in_place_type_t<Derived>{}, 42);
  xyz::polymorphic<Base> pp(std::move(p));

  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  xyz::polymorphic<Base> ppp(xyz::in_place_type_t<Derived>{}, 101);
  EXPECT_TRUE(
      !ppp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)

  using std::swap;
  swap(p, ppp);
  EXPECT_TRUE(
      !p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  EXPECT_TRUE(
      ppp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

TEST(PolymorphicTest, ConstPropagation) {
  struct SomeType {
    enum class Constness { CONST, NON_CONST };

    Constness member() { return Constness::NON_CONST; }

    Constness member() const { return Constness::CONST; }
  };

  xyz::polymorphic<SomeType> a(xyz::in_place_type_t<SomeType>{});
  EXPECT_EQ(a->member(), SomeType::Constness::NON_CONST);
  EXPECT_EQ((*a).member(), SomeType::Constness::NON_CONST);
  const auto& ca = a;
  EXPECT_EQ(ca->member(), SomeType::Constness::CONST);
  EXPECT_EQ((*ca).member(), SomeType::Constness::CONST);
}

TEST(PolymorphicTest, GetAllocator) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  xyz::polymorphic<Base, xyz::TrackingAllocator<Base>> p(
      std::allocator_arg,
      xyz::TrackingAllocator<Base>(&alloc_counter, &dealloc_counter),
      xyz::in_place_type_t<Derived>{}, 42);
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 0);

  auto tracking_allocator = p.get_allocator();
  EXPECT_EQ(alloc_counter, *tracking_allocator.alloc_counter_);
  EXPECT_EQ(dealloc_counter, *tracking_allocator.dealloc_counter_);
}

TEST(PolymorphicTest, TrackingAllocatorDefaultConstructor) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  xyz::polymorphic<Derived, xyz::TrackingAllocator<Derived>> p(
      std::allocator_arg,
      xyz::TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter));
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 0);

  auto tracking_allocator = p.get_allocator();
  EXPECT_EQ(alloc_counter, *tracking_allocator.alloc_counter_);
  EXPECT_EQ(dealloc_counter, *tracking_allocator.dealloc_counter_);
}

TEST(PolymorphicTest, CountAllocationsForInPlaceConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Base, xyz::TrackingAllocator<Base>> p(
        std::allocator_arg,
        xyz::TrackingAllocator<Base>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 42);
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
    xyz::polymorphic<Base, xyz::TrackingAllocator<Base>> p(
        std::allocator_arg,
        xyz::TrackingAllocator<Base>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
  }
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

TEST(PolymorphicTest, CountAllocationsForCopyConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, xyz::TrackingAllocator<Derived>> p(
        std::allocator_arg,
        xyz::TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
    xyz::polymorphic<Derived, xyz::TrackingAllocator<Derived>> pp(p);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

TEST(PolymorphicTest, CountAllocationsForCopyAssignment) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, xyz::TrackingAllocator<Derived>> p(
        std::allocator_arg,
        xyz::TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 42);
    xyz::polymorphic<Derived, xyz::TrackingAllocator<Derived>> pp(
        std::allocator_arg,
        xyz::TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    pp = p;
  }
  EXPECT_EQ(alloc_counter, 3);
  EXPECT_EQ(dealloc_counter, 3);
}

TEST(PolymorphicTest, CountAllocationsForMoveAssignment) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, xyz::TrackingAllocator<Derived>> p(
        std::allocator_arg,
        xyz::TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 42);
    xyz::polymorphic<Derived, xyz::TrackingAllocator<Derived>> pp(
        std::allocator_arg,
        xyz::TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    pp = std::move(p);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

template <typename T>
struct NonEqualTrackingAllocator : xyz::TrackingAllocator<T> {
  using xyz::TrackingAllocator<T>::TrackingAllocator;
  using propagate_on_container_move_assignment = std::true_type;

  template <typename Other>
  struct rebind {
    using other = NonEqualTrackingAllocator<Other>;
  };

  friend bool operator==(const NonEqualTrackingAllocator&,
                         const NonEqualTrackingAllocator&) noexcept {
    return false;
  }

  friend bool operator!=(const NonEqualTrackingAllocator&,
                         const NonEqualTrackingAllocator&) noexcept {
    return true;
  }
};

TEST(PolymorphicTest,
     CountAllocationsForMoveAssignmentWhenAllocatorsDontCompareEqual) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, NonEqualTrackingAllocator<Derived>> p(
        std::allocator_arg,
        NonEqualTrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 42);
    xyz::polymorphic<Derived, NonEqualTrackingAllocator<Derived>> pp(
        std::allocator_arg,
        NonEqualTrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    pp = std::move(p);  // This will copy as allocators don't compare equal.
  }
  EXPECT_EQ(alloc_counter, 3);
  EXPECT_EQ(dealloc_counter, 3);
}

TEST(PolymorphicTest, CountAllocationsForMoveConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, xyz::TrackingAllocator<Derived>> p(
        std::allocator_arg,
        xyz::TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
    xyz::polymorphic<Derived, xyz::TrackingAllocator<Derived>> pp(std::move(p));
  }
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

template <typename T>
struct POCSTrackingAllocator : xyz::TrackingAllocator<T> {
  using xyz::TrackingAllocator<T>::TrackingAllocator;
  using propagate_on_container_swap = std::true_type;

  template <typename Other>
  struct rebind {
    using other = POCSTrackingAllocator<Other>;
  };
};

TEST(PolymorphicTest, NonMemberSwapWhenAllocatorsDontCompareEqual) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, POCSTrackingAllocator<Derived>> p(
        std::allocator_arg,
        POCSTrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 42);
    xyz::polymorphic<Derived, POCSTrackingAllocator<Derived>> pp(
        std::allocator_arg,
        POCSTrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    swap(p, pp);
    EXPECT_EQ(p->value(), 101);
    EXPECT_EQ(pp->value(), 42);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

TEST(PolymorphicTest, MemberSwapWhenAllocatorsDontCompareEqual) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, POCSTrackingAllocator<Derived>> p(
        std::allocator_arg,
        POCSTrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 42);
    xyz::polymorphic<Derived, POCSTrackingAllocator<Derived>> pp(
        std::allocator_arg,
        POCSTrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<Derived>{}, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    p.swap(pp);
    EXPECT_EQ(p->value(), 101);
    EXPECT_EQ(pp->value(), 42);
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
    Exception() : std::runtime_error("ThrowsOnCopyConstruction::Exception") {}
  };

  ThrowsOnCopyConstruction() = default;

  ThrowsOnCopyConstruction(const ThrowsOnCopyConstruction&) {
    throw Exception();
  }
};

struct ThrowsOnMoveConstruction {
  class Exception : public std::runtime_error {
   public:
    Exception() : std::runtime_error("ThrowsOnMoveConstruction::Exception") {}
  };

  ThrowsOnMoveConstruction() = default;
  ThrowsOnMoveConstruction(const ThrowsOnMoveConstruction&) = default;

  ThrowsOnMoveConstruction(ThrowsOnMoveConstruction&&) { throw Exception(); }
};

TEST(PolymorphicTest, DefaultConstructorWithExceptions) {
  EXPECT_THROW(xyz::polymorphic<ThrowsOnConstruction>(),
               ThrowsOnConstruction::Exception);
}

TEST(PolymorphicTest, DefaultConstructorWithAllocatorsAndExceptions) {
  EXPECT_THROW(xyz::polymorphic<ThrowsOnConstruction>(
                   std::allocator_arg, std::allocator<ThrowsOnConstruction>()),
               ThrowsOnConstruction::Exception);
}

TEST(PolymorphicTest, ConstructorWithExceptions) {
  EXPECT_THROW(xyz::polymorphic<ThrowsOnConstruction>(
                   xyz::in_place_type_t<ThrowsOnConstruction>{}, "unused"),
               ThrowsOnConstruction::Exception);
}

TEST(PolymorphicTest, CopyConstructorWithExceptions) {
  auto create_copy = []() {
    auto p = xyz::polymorphic<ThrowsOnCopyConstruction>(
        xyz::in_place_type_t<ThrowsOnCopyConstruction>{});
    auto pp = p;
  };
  EXPECT_THROW(create_copy(), ThrowsOnCopyConstruction::Exception);
}

TEST(PolymorphicTest, ConstructorWithExceptionsTrackingAllocations) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  auto construct = [&]() {
    return xyz::polymorphic<ThrowsOnConstruction,
                            xyz::TrackingAllocator<ThrowsOnConstruction>>(
        std::allocator_arg,
        xyz::TrackingAllocator<ThrowsOnConstruction>(&alloc_counter,
                                                     &dealloc_counter),
        xyz::in_place_type_t<ThrowsOnConstruction>{}, "unused");
  };
  EXPECT_THROW(construct(), ThrowsOnConstruction::Exception);
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

#ifdef XYZ_HAS_STD_OPTIONAL
TEST(PolymorphicTest, InteractionWithOptional) {
  std::optional<xyz::polymorphic<Base>> p;
  EXPECT_FALSE(p.has_value());
  p.emplace(xyz::in_place_type_t<Derived>{}, 42);
  EXPECT_TRUE(p.has_value());
  EXPECT_EQ((*p)->value(), 42);
}
#endif  // XYZ_HAS_STD_OPTIONAL

TEST(PolymorphicTest, InteractionWithVector) {
  std::vector<xyz::polymorphic<Base>> ps;
  for (int i = 0; i < 16; ++i) {
    ps.push_back(xyz::polymorphic<Base>(xyz::in_place_type_t<Derived>{}, i));
  }
  for (int i = 0; i < 16; ++i) {
    EXPECT_EQ(ps[i]->value(), i);
  }
}

TEST(PolymorphicTest, InteractionWithMap) {
  std::map<int, xyz::polymorphic<Base>> ps;
  for (int i = 0; i < 16; ++i) {
    ps.emplace(i, xyz::polymorphic<Base>(xyz::in_place_type_t<Derived>{}, i));
  }
  for (const auto& kv : ps) {
    EXPECT_EQ(kv.second->value(), kv.first);
  }
}

TEST(PolymorphicTest, InteractionWithUnorderedMap) {
  std::unordered_map<int, xyz::polymorphic<Base>> ps;
  for (int i = 0; i < 16; ++i) {
    ps.emplace(i, xyz::polymorphic<Base>(xyz::in_place_type_t<Derived>{}, i));
  }
  for (const auto& kv : ps) {
    EXPECT_EQ(kv.second->value(), kv.first);
  }
}

TEST(PolymorphicTest, InteractionWithSizedAllocators) {
  EXPECT_TRUE(xyz::static_test<
              sizeof(xyz::polymorphic<int, xyz::TrackingAllocator<int>>) ==
              (sizeof(xyz::polymorphic<int>) +
               sizeof(xyz::TrackingAllocator<int>))>());
}

struct BaseA {
  int a_value = 3;
  virtual ~BaseA() = default;

  virtual int value() { return a_value; }
};

struct BaseB {
  int b_value = 4;
  virtual ~BaseB() = default;

  virtual int value() { return b_value; }
};

struct MultipleBases : BaseA, BaseB {
  int d_value = 5;

  virtual int value() { return d_value; }
};

TEST(PolymorphicTest, MultipleBases) {
  xyz::polymorphic<BaseA> p(xyz::in_place_type_t<MultipleBases>{});

  xyz::polymorphic<BaseB> pp(xyz::in_place_type_t<MultipleBases>{});

  EXPECT_EQ(p->value(), 5);
  EXPECT_EQ(pp->value(), 5);
  EXPECT_EQ(p->a_value, 3);
  EXPECT_EQ(pp->b_value, 4);
}

#ifdef XYZ_HAS_STD_MEMORY_RESOURCE
TEST(PolymorphicTest, InteractionWithPMRAllocators) {
  std::array<std::byte, 1024> buffer;
  std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
  std::pmr::polymorphic_allocator<Base> pa{&mbr};
  using PolymorphicBase =
      xyz::polymorphic<Base, std::pmr::polymorphic_allocator<Base>>;
  PolymorphicBase p(std::allocator_arg, pa, xyz::in_place_type_t<Derived>{},
                    42);
  std::pmr::vector<PolymorphicBase> values{pa};
  values.push_back(p);
  values.push_back(std::move(p));
  EXPECT_EQ(values[0]->value(), 42);
}

TEST(PolymorphicTest, InteractionWithPMRAllocatorsWhenCopyThrows) {
  std::array<std::byte, 1024> buffer;
  std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
  std::pmr::polymorphic_allocator<ThrowsOnCopyConstruction> pa{&mbr};
  using PolymorphicType = xyz::polymorphic<
      ThrowsOnCopyConstruction,
      std::pmr::polymorphic_allocator<ThrowsOnCopyConstruction>>;
  PolymorphicType p(std::allocator_arg, pa,
                    xyz::in_place_type_t<ThrowsOnCopyConstruction>{});
  std::pmr::vector<PolymorphicType> values{pa};
  EXPECT_THROW(values.push_back(p), ThrowsOnCopyConstruction::Exception);
}
#endif  // XYZ_HAS_STD_MEMORY_RESOURCE

TEST(PolymorphicTest, TaggedAllocatorsEqualMoveConstruct) {
  xyz::TaggedAllocator<Derived> a(42);
  xyz::TaggedAllocator<Derived> aa(42);

  static_assert(!std::allocator_traits<
                    xyz::TaggedAllocator<Derived>>::is_always_equal::value,
                "");

  xyz::polymorphic<Derived, xyz::TaggedAllocator<Derived>> p(
      std::allocator_arg, a, xyz::in_place_type_t<Derived>{}, -1);
  xyz::polymorphic<Derived, xyz::TaggedAllocator<Derived>> pp(
      std::allocator_arg, aa, std::move(p));

  // NOLINT(clang-analyzer-cplusplus.Move)
  EXPECT_TRUE(p.valueless_after_move());
  EXPECT_EQ(pp->value(), -1);
}

TEST(PolymorphicTest, TaggedAllocatorsNotEqualMoveConstruct) {
  xyz::TaggedAllocator<Derived> a(42);
  xyz::TaggedAllocator<Derived> aa(101);

  static_assert(!std::allocator_traits<
                    xyz::TaggedAllocator<Derived>>::is_always_equal::value,
                "");

  xyz::polymorphic<Derived, xyz::TaggedAllocator<Derived>> p(
      std::allocator_arg, a, xyz::in_place_type_t<Derived>{}, -1);
  xyz::polymorphic<Derived, xyz::TaggedAllocator<Derived>> pp(
      std::allocator_arg, aa, std::move(p));

  EXPECT_FALSE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
  EXPECT_EQ(pp->value(), -1);
}

TEST(PolymorphicTest, TaggedAllocatorsNotEqualThrowingMoveConstruct) {
  xyz::TaggedAllocator<ThrowsOnMoveConstruction> a(42);
  xyz::TaggedAllocator<ThrowsOnMoveConstruction> aa(101);

  static_assert(
      !std::allocator_traits<xyz::TaggedAllocator<ThrowsOnMoveConstruction>>::
          is_always_equal::value,
      "");

  xyz::polymorphic<ThrowsOnMoveConstruction,
                   xyz::TaggedAllocator<ThrowsOnMoveConstruction>>
      p(std::allocator_arg, a,
        xyz::in_place_type_t<ThrowsOnMoveConstruction>{});

  auto move_construct = [&]() {
    xyz::polymorphic<ThrowsOnMoveConstruction,
                     xyz::TaggedAllocator<ThrowsOnMoveConstruction>>
        pp(std::allocator_arg, aa, std::move(p));
  };
  EXPECT_THROW(move_construct(), ThrowsOnMoveConstruction::Exception);
}

TEST(PolymorphicTest, TaggedAllocatorsNotEqualMoveConstructFromValueless) {
  xyz::TaggedAllocator<Derived> a(42);
  xyz::TaggedAllocator<Derived> aa(101);

  static_assert(!std::allocator_traits<
                    xyz::TaggedAllocator<Derived>>::is_always_equal::value,
                "");

  xyz::polymorphic<Derived, xyz::TaggedAllocator<Derived>> p(
      std::allocator_arg, a, xyz::in_place_type_t<Derived>{}, -1);

  xyz::polymorphic<Derived, xyz::TaggedAllocator<Derived>> pp(std::move(p));
  EXPECT_TRUE(
      p.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)

  // Move construct from the now valueless p.
  xyz::polymorphic<Derived, xyz::TaggedAllocator<Derived>> ppp(
      std::allocator_arg, aa, std::move(p));

  EXPECT_TRUE(
      ppp.valueless_after_move());  // NOLINT(clang-analyzer-cplusplus.Move)
}

}  // namespace
