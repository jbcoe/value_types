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

#include "cloning_ptr.h"

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

TEST(CloningPtrTest, ValueAccessFromInPlaceConstructedObject) {
  xyz::cloning_ptr<A> a(std::in_place_type<A>, 42);
  EXPECT_EQ(*a, 42);
}

TEST(CloningPtrTest, ValueAccessFromDefaultConstructedObject) {
  xyz::cloning_ptr<A> a(std::in_place_type<A>);
  EXPECT_EQ(*a, 0);
}

TEST(CloningPtrTest, CopiesAreDistinct) {
  xyz::cloning_ptr<A> a(std::in_place_type<A>, 42);
  auto aa = a;
  EXPECT_EQ(*a, *aa);
  EXPECT_NE(&*a, &*aa);
}

TEST(CloningPtrTest, MovePreservesOwnedObjectAddress) {
  xyz::cloning_ptr<A> a(std::in_place_type<A>, 42);
  auto address = &*a;
  auto aa = std::move(a);

  EXPECT_TRUE(!a);
  EXPECT_EQ(address, &*aa);
}

TEST(CloningPtrTest, Swap) {
  xyz::cloning_ptr<A> a(std::in_place_type<A>, 42);
  xyz::cloning_ptr<A> b(std::in_place_type<A>, 43);
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

TEST(CloningPtrTest, AccessDerivedObject) {
  xyz::cloning_ptr<Base> a(std::in_place_type<Derived>, 42);
  EXPECT_EQ(a->value(), 42);
}

TEST(CloningPtrTest, CopiesOfDerivedObjectsAreDistinct) {
  xyz::cloning_ptr<Base> a(std::in_place_type<Derived>, 42);
  auto aa = a;
  EXPECT_EQ(a->value(), aa->value());
  EXPECT_NE(&*a, &*aa);
}

TEST(CloningPtrTest, MovePreservesOwnedDerivedObjectAddress) {
  xyz::cloning_ptr<Base> a(std::in_place_type<Derived>, 42);
  auto address = &*a;
  auto aa = std::move(a);
  EXPECT_EQ(address, &*aa);
}

TEST(CloningPtrTest, CopyAssignment) {
  xyz::cloning_ptr<Base> a(std::in_place_type<Derived>, 42);
  xyz::cloning_ptr<Base> b(std::in_place_type<Derived>, 101);
  EXPECT_EQ(a->value(), 42);
  a = b;

  EXPECT_EQ(a->value(), 101);
  EXPECT_NE(&*a, &*b);
}

TEST(CloningPtrTest, MoveAssignment) {
  xyz::cloning_ptr<Base> a(std::in_place_type<Derived>, 42);
  xyz::cloning_ptr<Base> b(std::in_place_type<Derived>, 101);
  EXPECT_EQ(a->value(), 42);
  a = std::move(b);

  EXPECT_TRUE(!b);
  EXPECT_EQ(a->value(), 101);
}

TEST(CloningPtrTest, NonMemberSwap) {
  xyz::cloning_ptr<Base> a(std::in_place_type<Derived>, 42);
  xyz::cloning_ptr<Base> b(std::in_place_type<Derived>, 101);
  using std::swap;
  swap(a, b);
  EXPECT_EQ(a->value(), 101);
  EXPECT_EQ(b->value(), 42);
}

TEST(CloningPtrTest, MemberSwap) {
  xyz::cloning_ptr<Base> a(std::in_place_type<Derived>, 42);
  xyz::cloning_ptr<Base> b(std::in_place_type<Derived>, 101);

  a.swap(b);
  EXPECT_EQ(a->value(), 101);
  EXPECT_EQ(b->value(), 42);
}

TEST(CloningPtrTest, ConstPropagation) {
  struct SomeType {
    enum class Constness { CONST, NON_CONST };
    Constness member() { return Constness::NON_CONST; }
    Constness member() const { return Constness::CONST; }
  };

  xyz::cloning_ptr<SomeType> a(std::in_place_type<SomeType>);
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

TEST(CloningPtrTest, DefaultConstructorWithExceptions) {
  EXPECT_THROW(xyz::cloning_ptr<ThrowsOnConstruction>{ std::in_place_type<ThrowsOnConstruction> },
               ThrowsOnConstruction::Exception);
}

TEST(CloningPtrTest, ConstructorWithExceptions) {
  EXPECT_THROW(xyz::cloning_ptr<ThrowsOnConstruction>(
                   std::in_place_type<ThrowsOnConstruction>, "unused"),
               ThrowsOnConstruction::Exception);
}

TEST(CloningPtrTest, CopyConstructorWithExceptions) {
  auto create_copy = []() {
    auto a = xyz::cloning_ptr<ThrowsOnCopyConstruction>(
        std::in_place_type<ThrowsOnCopyConstruction>);
    auto aa = a;
  };
  EXPECT_THROW(create_copy(), ThrowsOnCopyConstruction::Exception);
}

TEST(CloningPtrTest, InteractionWithOptional) {
  std::optional<xyz::cloning_ptr<Base>> a;
  EXPECT_FALSE(a.has_value());
  a.emplace(std::in_place_type<Derived>, 42);
  EXPECT_TRUE(a.has_value());
  EXPECT_EQ((*a)->value(), 42);
}

TEST(CloningPtrTest, InteractionWithVector) {
  std::vector<xyz::cloning_ptr<Base>> as;
  for (int i = 0; i < 16; ++i) {
    as.push_back(xyz::cloning_ptr<Base>(std::in_place_type<Derived>, i));
  }
  for (int i = 0; i < 16; ++i) {
    EXPECT_EQ(as[i]->value(), i);
  }
}

TEST(CloningPtrTest, InteractionWithMap) {
  std::map<int, xyz::cloning_ptr<Base>> as;
  for (int i = 0; i < 16; ++i) {
    as.emplace(i, xyz::cloning_ptr<Base>(std::in_place_type<Derived>, i));
  }
  for (auto [k, v] : as) {
    EXPECT_EQ(v->value(), k);
  }
}

TEST(CloningPtrTest, InteractionWithUnorderedMap) {
  std::unordered_map<int, xyz::cloning_ptr<Base>> as;
  for (int i = 0; i < 16; ++i) {
    as.emplace(i, xyz::cloning_ptr<Base>(std::in_place_type<Derived>, i));
  }
  for (auto [k, v] : as) {
    EXPECT_EQ(v->value(), k);
  }
}


class B {
public:
    virtual ~B() {}

    double m_value;
};

class Dual : public A, public B {
};


TEST(CloningPtrTest, SecondBase) {
    xyz::cloning_ptr<B> p1(std::in_place_type<Dual>);
    p1->m_value = 3.25;

    xyz::cloning_ptr<B> p2(p1);

    // Check that cloning created a Dual
    auto d = dynamic_cast<Dual*>(&*p2);
    EXPECT_NE(d, nullptr);

    EXPECT_EQ(p2->m_value, 3.25);

    p1->m_value = 14;

    EXPECT_NE(p2->m_value, 14);         // Check that cloning took place.
};


/// Test with virtual bases
struct Left : public virtual A {
    int m_leftVal = 1;
};
struct Right : public virtual A {
    int m_rightVal = 2;
};

struct Both : public Left, public Right {
    Both(int i) : A(i) {}
};

TEST(CloningPtrTest, VirtualBase) {
    xyz::cloning_ptr<A> pa(std::in_place_type<Both>, 17);

    xyz::cloning_ptr<A> pa2(pa);

    EXPECT_EQ(pa2->value(), 17);
};

//// This is a compilability test showing that you can compile a class containing a cloning_ptr to a forward declared class and still
//// copy and destroy it without having seen the definition. What you can't do, just as for indirect, is to construct the cloning_ptr 
//// without having seen its definition (obviously).

// This T type is only forward declared now
//struct ForwardDeclared {};
struct ForwardDeclared;

// Some header file which only sees the forward declaration
class UsingForwardDeclared {
public:
  UsingForwardDeclared();

  xyz::cloning_ptr<ForwardDeclared> m_member;
};

// Some C++ file which includes file above
void testfun() {
  UsingForwardDeclared a = UsingForwardDeclared();
  UsingForwardDeclared b = a;
}

// The header file that defines ForwardDeclared.
struct ForwardDeclared {};

struct FDerived : public ForwardDeclared {
  FDerived() = default;
};

FDerived d;

// The C++ file that defines the UsingForwardDeclared methods after including the definition of FDerived in its header file.
UsingForwardDeclared::UsingForwardDeclared()
    : m_member(std::in_place_type<FDerived>) {}


}  // namespace
