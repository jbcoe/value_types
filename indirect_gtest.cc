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

#include "indirect.h"

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
  EXPECT_EQ(address, &*aa);
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

TEST(IndirectTest, Swap) {
  xyz::indirect<int> a(std::in_place, 42);
  xyz::indirect<int> b(std::in_place, 43);
  auto address_a = &*a;
  auto address_b = &*b;
  swap(a, b);
  EXPECT_EQ(*a, 43);
  EXPECT_EQ(*b, 42);
  EXPECT_EQ(address_a, &*b);
  EXPECT_EQ(address_b, &*a);
}

TEST(IndirectTest, Hash) {
  xyz::indirect<int> a(std::in_place, 42);
  EXPECT_EQ(std::hash<xyz::indirect<int>>()(a), std::hash<int>()(*a));
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
