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

#include <utility>

#include "polymorphic.h"

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

TEST(PolymorphicTest, CopiesAreDistinct) {
  xyz::polymorphic<A> a(std::in_place_type<A>, 42);
  auto aa = a;
  EXPECT_EQ(*a, *aa);
  EXPECT_NE(&*a, &*aa);
}

TEST(PolymorphicTest, DISABLED_MovePreservesOwnedObjectAddress) {
  xyz::polymorphic<A> a(std::in_place_type<A>, 42);
  auto address = &*a;
  auto aa = std::move(a);
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

TEST(PolymorphicTest, DISABLED_MovePreservesOwnedDerivedObjectAddress) {
  xyz::polymorphic<Base> a(std::in_place_type<Derived>, 42);
  auto address = &*a;
  auto aa = std::move(a);
  EXPECT_EQ(address, &*aa);
}
