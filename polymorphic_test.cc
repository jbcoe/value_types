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

#include "polymorphic.h"

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

TEST(PolymorphicTest, ValueAccessFromInPlaceConstructedObject) {
  xyz::polymorphic<A> a(std::in_place_type<A>, 42);
  EXPECT_EQ(*a, 42);
}

TEST(PolymorphicTest, ValueAccessFromDefaultConstructedObject) {
  xyz::polymorphic<A> a;
  EXPECT_EQ(*a, 0);
}

TEST(PolymorphicTest, CopiesAreDistinct) {
  xyz::polymorphic<A> a(std::in_place_type<A>, 42);
  auto aa = a;
  EXPECT_EQ(*a, *aa);
  EXPECT_NE(&*a, &*aa);
}

TEST(PolymorphicTest, MovePreservesOwnedObjectAddress) {
  xyz::polymorphic<A> a(std::in_place_type<A>, 42);
  auto address = &*a;
  auto aa = std::move(a);

  EXPECT_TRUE(a.valueless_after_move());
  EXPECT_EQ(address, &*aa);
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
  virtual ~Base() = default;
  virtual int value() const = 0;
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

TEST(PolymorphicTest, MovePreservesOwnedDerivedObjectAddress) {
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

  EXPECT_TRUE(b.valueless_after_move());
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

TEST(PolymorphicTest, GetAllocator) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;

  xyz::polymorphic<A, TrackingAllocator<A>> a(
      std::allocator_arg,
      TrackingAllocator<A>(&alloc_counter, &dealloc_counter),
      std::in_place_type<A>, 42);
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 0);

  auto tracking_allocator = a.get_allocator();
  EXPECT_EQ(alloc_counter, *tracking_allocator.alloc_counter_);
  EXPECT_EQ(dealloc_counter, *tracking_allocator.dealloc_counter_);
}

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

TEST(PolymorphicTest, CountAllocationsForCopyConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, TrackingAllocator<Derived>> a(
        std::allocator_arg,
        TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        std::in_place_type<Derived>, 42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
    xyz::polymorphic<Derived, TrackingAllocator<Derived>> b(a);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

TEST(PolymorphicTest, CountAllocationsForCopyAssignment) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, TrackingAllocator<Derived>> a(
        std::allocator_arg,
        TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        std::in_place_type<Derived>, 42);
    xyz::polymorphic<Derived, TrackingAllocator<Derived>> b(
        std::allocator_arg,
        TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        std::in_place_type<Derived>, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = a;
  }
  EXPECT_EQ(alloc_counter, 3);
  EXPECT_EQ(dealloc_counter, 3);
}

TEST(PolymorphicTest, CountAllocationsForMoveAssignment) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, TrackingAllocator<Derived>> a(
        std::allocator_arg,
        TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        std::in_place_type<Derived>, 42);
    xyz::polymorphic<Derived, TrackingAllocator<Derived>> b(
        std::allocator_arg,
        TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        std::in_place_type<Derived>, 101);
    EXPECT_EQ(alloc_counter, 2);
    EXPECT_EQ(dealloc_counter, 0);
    b = std::move(a);
  }
  EXPECT_EQ(alloc_counter, 2);
  EXPECT_EQ(dealloc_counter, 2);
}

TEST(PolymorphicTest, CountAllocationsForMoveConstruction) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  {
    xyz::polymorphic<Derived, TrackingAllocator<Derived>> a(
        std::allocator_arg,
        TrackingAllocator<Derived>(&alloc_counter, &dealloc_counter),
        std::in_place_type<Derived>, 42);
    EXPECT_EQ(alloc_counter, 1);
    EXPECT_EQ(dealloc_counter, 0);
    xyz::polymorphic<Derived, TrackingAllocator<Derived>> b(std::move(a));
  }
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
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

TEST(PolymorphicTest, DefaultConstructorWithExceptions) {
  EXPECT_THROW(xyz::polymorphic<ThrowsOnConstruction>(),
               ThrowsOnConstruction::Exception);
}

TEST(PolymorphicTest, ConstructorWithExceptions) {
  EXPECT_THROW(xyz::polymorphic<ThrowsOnConstruction>(
                   std::in_place_type<ThrowsOnConstruction>, "unused"),
               ThrowsOnConstruction::Exception);
}

TEST(PolymorphicTest, CopyConstructorWithExceptions) {
  auto create_copy = []() {
    auto a = xyz::polymorphic<ThrowsOnCopyConstruction>(
        std::in_place_type<ThrowsOnCopyConstruction>);
    auto aa = a;
  };
  EXPECT_THROW(create_copy(), ThrowsOnCopyConstruction::Exception);
}

TEST(PolymorphicTest, ConstructorWithExceptionsTrackingAllocations) {
  unsigned alloc_counter = 0;
  unsigned dealloc_counter = 0;
  auto construct = [&]() {
    return xyz::polymorphic<ThrowsOnConstruction,
                            TrackingAllocator<ThrowsOnConstruction>>(
        std::allocator_arg,
        TrackingAllocator<ThrowsOnConstruction>(&alloc_counter,
                                                &dealloc_counter),
        std::in_place_type<ThrowsOnConstruction>, "unused");
  };
  EXPECT_THROW(construct(), ThrowsOnConstruction::Exception);
  EXPECT_EQ(alloc_counter, 1);
  EXPECT_EQ(dealloc_counter, 1);
}

TEST(PolymorphicTest, InteractionWithOptional) {
  std::optional<xyz::polymorphic<Base>> a;
  EXPECT_FALSE(a.has_value());
  a.emplace(std::in_place_type<Derived>, 42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ((*a)->value(), 42);
}

TEST(PolymorphicTest, InteractionWithVector) {
  std::vector<xyz::polymorphic<Base>> as;
  for (int i = 0; i < 16; ++i) {
    as.push_back(xyz::polymorphic<Base>(std::in_place_type<Derived>, i));
  }
  for (int i = 0; i < 16; ++i) {
    EXPECT_EQ(as[i]->value(), i);
  }
}

TEST(PolymorphicTest, InteractionWithMap) {
  std::map<int, xyz::polymorphic<Base>> as;
  for (int i = 0; i < 16; ++i) {
    as.emplace(i, xyz::polymorphic<Base>(std::in_place_type<Derived>, i));
  }
  for (auto [k, v] : as) {
    EXPECT_EQ(v->value(), k);
  }
}

TEST(PolymorphicTest, InteractionWithUnorderedMap) {
  std::unordered_map<int, xyz::polymorphic<Base>> as;
  for (int i = 0; i < 16; ++i) {
    as.emplace(i, xyz::polymorphic<Base>(std::in_place_type<Derived>, i));
  }
  for (auto [k, v] : as) {
    EXPECT_EQ(v->value(), k);
  }
}

TEST(PolymorphicTest, InteractionWithSizedAllocators) {
  EXPECT_EQ(sizeof(xyz::polymorphic<int>), sizeof(int*));
  EXPECT_EQ(sizeof(xyz::polymorphic<int, TrackingAllocator<int>>),
            (sizeof(int*) + sizeof(TrackingAllocator<int>)));
}

struct BaseA {
  int a_value = 3;
  virtual int value() { return a_value; }
};

struct BaseB {
  int b_value = 4;
  virtual int value() { return b_value; }
};

struct MultipleBases : BaseA, BaseB {
  int d_value = 5;
  virtual int value() { return d_value; }
};

TEST(PolymorphicTest, MultipleBases) {
  xyz::polymorphic<BaseA> a(std::in_place_type<MultipleBases>);

  xyz::polymorphic<BaseB> b(std::in_place_type<MultipleBases>);

  EXPECT_EQ(a->value(), 5);
  EXPECT_EQ(b->value(), 5);
  EXPECT_EQ(a->a_value, 3);
  EXPECT_EQ(b->b_value, 4);
}

#if (__cpp_lib_memory_resource >= 201603L)
TEST(PolymorphicTest, InteractionWithPMRAllocators) {
  std::array<std::byte, 1024> buffer;
  std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
  std::pmr::polymorphic_allocator<Base> pa{&mbr};
  using PolymorphicBase =
      xyz::polymorphic<Base, std::pmr::polymorphic_allocator<Base>>;
  PolymorphicBase a(std::allocator_arg, pa, std::in_place_type<Derived>, 42);
  std::pmr::vector<PolymorphicBase> values{pa};
  values.push_back(a);
  values.push_back(std::move(a));
  EXPECT_EQ(values[0]->value(), 42);
}

TEST(PolymorphicTest, InteractionWithPMRAllocatorsWhenCopyThrows) {
  std::array<std::byte, 1024> buffer;
  std::pmr::monotonic_buffer_resource mbr{buffer.data(), buffer.size()};
  std::pmr::polymorphic_allocator<ThrowsOnCopyConstruction> pa{&mbr};
  using PolymorphicType = xyz::polymorphic<
      ThrowsOnCopyConstruction,
      std::pmr::polymorphic_allocator<ThrowsOnCopyConstruction>>;
  PolymorphicType a(std::allocator_arg, pa,
                    std::in_place_type<ThrowsOnCopyConstruction>);
  std::pmr::vector<PolymorphicType> values{pa};
  EXPECT_THROW(values.push_back(a), ThrowsOnCopyConstruction::Exception);
}
#endif  // (__cpp_lib_memory_resource >= 201603L)

}  // namespace
