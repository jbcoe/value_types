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

#include <array>
#include <stdexcept>

#include "polymorphic_sbo.h"
#include "tracking_allocator.h"

namespace xyz {
using std::in_place_type_t;
}  // namespace xyz

namespace {

// Simple copyable derived classes for testing
// Small derived class that should fit in SBO buffer
class SmallDerived {
 public:
  explicit SmallDerived(int x = 0) : x_(x) {}

  int value() const { return x_; }

 private:
  int x_;  // Should be small enough for SBO
};

// Large derived class that should NOT fit in SBO buffer
class LargeDerived {
 public:
  explicit LargeDerived(int x = 0) : x_(x) { data_.fill(x); }

  int value() const { return x_; }

 private:
  int x_;
  std::array<int, 100>
      data_;  // 400+ bytes (definitely won't fit in 32-byte SBO buffer)
};

// Small class that can throw during move construction
class SmallThrowingMove {
 public:
  explicit SmallThrowingMove(int x = 0) : x_(x) {}

  // Copy constructor is fine
  SmallThrowingMove(const SmallThrowingMove& other) : x_(other.x_) {}

  // Move constructor that can throw (not noexcept)
  SmallThrowingMove(SmallThrowingMove&& other) : x_(other.x_) {
    // Intentionally not marked noexcept, so it can potentially throw
    if (x_ < 0) {
      throw std::runtime_error("Move construction failed");
    }
  }

  SmallThrowingMove& operator=(const SmallThrowingMove& other) {
    x_ = other.x_;
    return *this;
  }

  SmallThrowingMove& operator=(SmallThrowingMove&& other) {
    x_ = other.x_;
    return *this;
  }

  int value() const { return x_; }

 private:
  int x_;  // Small enough to fit in SBO buffer, but move constructor throws
};

}  // namespace

// Test that small objects use SBO (0 allocations)
TEST(PolymorphicSBOAllocationTest, SmallObjectUsesSBO) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  {
    xyz::polymorphic<SmallDerived, xyz::TrackingAllocator<SmallDerived>> p(
        std::allocator_arg,
        xyz::TrackingAllocator<SmallDerived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<SmallDerived>{}, 42);

    // Small objects should use SBO - 0 allocations
    EXPECT_EQ(alloc_counter, 0)
        << "Small objects should use SBO with 0 allocations";
    EXPECT_EQ(dealloc_counter, 0);
    EXPECT_TRUE(p.uses_sbo())
        << "Small objects should report uses_sbo() = true";
    EXPECT_EQ(p->value(), 42);
  }

  // After destruction, still 0 deallocations for SBO
  EXPECT_EQ(alloc_counter, 0);
  EXPECT_EQ(dealloc_counter, 0);
}

// Test that large objects use heap allocation (1 allocation)
TEST(PolymorphicSBOAllocationTest, LargeObjectUsesHeapAllocation) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  {
    xyz::polymorphic<LargeDerived, xyz::TrackingAllocator<LargeDerived>> p(
        std::allocator_arg,
        xyz::TrackingAllocator<LargeDerived>(&alloc_counter, &dealloc_counter),
        xyz::in_place_type_t<LargeDerived>{}, 42);

    // Large objects should use heap allocation - 1 allocation
    EXPECT_EQ(alloc_counter, 1) << "Large objects should use heap allocation";
    EXPECT_EQ(dealloc_counter, 0);
    EXPECT_FALSE(p.uses_sbo())
        << "Large objects should report uses_sbo() = false";
    EXPECT_EQ(p->value(), 42);
  }

  // After destruction, should have 1 deallocation
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

// Test copy construction preserves SBO behavior
TEST(PolymorphicSBOAllocationTest, CopyConstructionPreservesSBO) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  xyz::polymorphic<SmallDerived, xyz::TrackingAllocator<SmallDerived>> p1(
      std::allocator_arg,
      xyz::TrackingAllocator<SmallDerived>(&alloc_counter, &dealloc_counter),
      xyz::in_place_type_t<SmallDerived>{}, 42);

  EXPECT_EQ(alloc_counter, 0);
  EXPECT_TRUE(p1.uses_sbo());

  // Copy construction of SBO object should still use SBO
  auto p2 = p1;
  EXPECT_EQ(alloc_counter, 0) << "Copying SBO object should still use SBO";
  EXPECT_TRUE(p2.uses_sbo());
  EXPECT_EQ(p2->value(), 42);
}

// Test multiple copy levels preserve SBO
TEST(PolymorphicSBOAllocationTest, MultipleCopiesPreserveSBO) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  xyz::polymorphic<SmallDerived, xyz::TrackingAllocator<SmallDerived>> p1(
      std::allocator_arg,
      xyz::TrackingAllocator<SmallDerived>(&alloc_counter, &dealloc_counter),
      xyz::in_place_type_t<SmallDerived>{}, 100);

  // Chain of copies - all should preserve SBO
  auto p2 = p1;
  auto p3 = p2;
  auto p4 = p3;

  EXPECT_EQ(alloc_counter, 0) << "Multiple copies should all use SBO";
  EXPECT_TRUE(p1.uses_sbo());
  EXPECT_TRUE(p2.uses_sbo());
  EXPECT_TRUE(p3.uses_sbo());
  EXPECT_TRUE(p4.uses_sbo());

  EXPECT_EQ(p1->value(), 100);
  EXPECT_EQ(p2->value(), 100);
  EXPECT_EQ(p3->value(), 100);
  EXPECT_EQ(p4->value(), 100);
}

// Test that large objects still properly use heap allocation when copied
TEST(PolymorphicSBOAllocationTest, CopyLargeObjectStillUsesHeap) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  xyz::polymorphic<LargeDerived, xyz::TrackingAllocator<LargeDerived>> p1(
      std::allocator_arg,
      xyz::TrackingAllocator<LargeDerived>(&alloc_counter, &dealloc_counter),
      xyz::in_place_type_t<LargeDerived>{}, 42);

  EXPECT_EQ(alloc_counter, 1);
  EXPECT_FALSE(p1.uses_sbo());

  // Copy of large object should also use heap
  auto p2 = p1;
  EXPECT_EQ(alloc_counter, 2)
      << "Copying large object should allocate another heap block";
  EXPECT_FALSE(p2.uses_sbo());
  EXPECT_EQ(p2->value(), 42);
}

// Test that SBO threshold is working correctly around the boundary
TEST(PolymorphicSBOAllocationTest, SBOThresholdBehavior) {
  // Verify the SBO size constant
  static_assert(XYZ_POLYMORPHIC_SBO_SIZE == 32,
                "Expected SBO size of 32 bytes");

  // Verify small object size assumption
  static_assert(sizeof(SmallDerived) <= 32,
                "SmallDerived should fit in SBO buffer");

  // Verify large object size assumption
  static_assert(sizeof(LargeDerived) > 32,
                "LargeDerived should NOT fit in SBO buffer");

  // Verify throwing move class size but not nothrow move constructible
  static_assert(sizeof(SmallThrowingMove) <= 32,
                "SmallThrowingMove should fit size-wise in SBO buffer");
  static_assert(!std::is_nothrow_move_constructible_v<SmallThrowingMove>,
                "SmallThrowingMove should not be nothrow move constructible");
}

// Test that small objects with throwing move constructors use heap allocation
TEST(PolymorphicSBOAllocationTest, SmallObjectWithThrowingMoveUsesHeap) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  {
    xyz::polymorphic<SmallThrowingMove,
                     xyz::TrackingAllocator<SmallThrowingMove>>
        p(std::allocator_arg,
          xyz::TrackingAllocator<SmallThrowingMove>(&alloc_counter,
                                                    &dealloc_counter),
          xyz::in_place_type_t<SmallThrowingMove>{}, 42);

    // Small objects with throwing move constructors should use heap allocation
    // - 1 allocation
    EXPECT_EQ(alloc_counter, 1)
        << "Small objects with throwing move should use heap allocation";
    EXPECT_EQ(dealloc_counter, 0);
    EXPECT_FALSE(p.uses_sbo())
        << "Small objects with throwing move should report uses_sbo() = false";
    EXPECT_EQ(p->value(), 42);
  }

  // After destruction, should have 1 deallocation
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

// Test copy construction of throwing move objects still uses heap
TEST(PolymorphicSBOAllocationTest, CopyThrowingMoveObjectStillUsesHeap) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  xyz::polymorphic<SmallThrowingMove, xyz::TrackingAllocator<SmallThrowingMove>>
      p1(std::allocator_arg,
         xyz::TrackingAllocator<SmallThrowingMove>(&alloc_counter,
                                                   &dealloc_counter),
         xyz::in_place_type_t<SmallThrowingMove>{}, 42);

  EXPECT_EQ(alloc_counter, 1);
  EXPECT_FALSE(p1.uses_sbo());

  // Copy of throwing move object should also use heap
  auto p2 = p1;
  EXPECT_EQ(alloc_counter, 2)
      << "Copying throwing move object should allocate another heap block";
  EXPECT_FALSE(p2.uses_sbo());
  EXPECT_EQ(p2->value(), 42);
}

// Test that nothrow move constructible requirement is properly checked
TEST(PolymorphicSBOAllocationTest, NoThrowMoveConstructibleRequirement) {
  // Verify the requirements for SBO
  static_assert(std::is_nothrow_move_constructible_v<SmallDerived>,
                "SmallDerived should be nothrow move constructible to use SBO");
  static_assert(!std::is_nothrow_move_constructible_v<SmallThrowingMove>,
                "SmallThrowingMove should not be nothrow move constructible");

  // Test the behavior by checking actual allocation patterns
  unsigned alloc_counter1 = 0, dealloc_counter1 = 0;
  unsigned alloc_counter2 = 0, dealloc_counter2 = 0;

  // SmallDerived should use SBO
  {
    xyz::polymorphic<SmallDerived, xyz::TrackingAllocator<SmallDerived>> p(
        std::allocator_arg,
        xyz::TrackingAllocator<SmallDerived>(&alloc_counter1,
                                             &dealloc_counter1),
        xyz::in_place_type_t<SmallDerived>{}, 1);
    EXPECT_EQ(alloc_counter1, 0) << "SmallDerived should use SBO";
    EXPECT_TRUE(p.uses_sbo());
  }

  // SmallThrowingMove should not use SBO
  {
    xyz::polymorphic<SmallThrowingMove,
                     xyz::TrackingAllocator<SmallThrowingMove>>
        p(std::allocator_arg,
          xyz::TrackingAllocator<SmallThrowingMove>(&alloc_counter2,
                                                    &dealloc_counter2),
          xyz::in_place_type_t<SmallThrowingMove>{}, 1);
    EXPECT_EQ(alloc_counter2, 1)
        << "SmallThrowingMove should use heap allocation";
    EXPECT_FALSE(p.uses_sbo());
  }
}
