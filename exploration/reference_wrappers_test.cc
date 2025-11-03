#include <gtest/gtest.h>

#include <functional>

#include "indirect.h"

namespace {

TEST(IndirectExploration, ReferenceWrapperAndSwap) {
  // Given two dynamically allocated values managed with `indirect`.
  auto a = xyz::indirect(3);
  auto b = xyz::indirect(4);

  // Values are as expected.
  EXPECT_EQ(*a, 3);
  EXPECT_EQ(*b, 4);

  // Given references to the values.
  auto ar = std::ref(*a);
  auto br = std::ref(*b);

  // Values accessed through the references are as expected.
  EXPECT_EQ(ar, 3);
  EXPECT_EQ(br, 4);

  // When we swap the two indirect values.
  swap(a, b);
  // Then the values themselves have been swapped.
  EXPECT_EQ(*a, 4);
  EXPECT_EQ(*b, 3);

  // But the reference values refer to the original values.
  EXPECT_EQ(ar, 3);
  EXPECT_EQ(br, 4);

  // When we swap the two reference wrappers.
  swap(ar, br);

  // Then the values accessed through references have been swapped.
  EXPECT_EQ(ar, 4);
  EXPECT_EQ(br, 3);
}

TEST(IndirectExploration, ReferenceWrapperAndMove) {
  // Given two dynamically allocated values managed with `indirect`.
  auto a = xyz::indirect(3);
  auto b = xyz::indirect(4);

  // Values are as expected.
  EXPECT_EQ(*a, 3);
  EXPECT_EQ(*b, 4);

  // Given references to the values.
  auto ar = std::ref(*a);
  auto br = std::ref(*b);

  // Values accessed through the references are as expected.
  EXPECT_EQ(ar, 3);
  EXPECT_EQ(br, 4);

  // When we move values and references.
  a = std::move(b);  // this renders `b` valueless.

  // Note: At this point ar is a reference to a value which no longer exists.
  // EXPECT_EQ(ar, 3); - This would lead to heap-use-after-free when run under
  // ASAN.

  ar = std::move(br);  // this does nothing to `br`.

  // The moved-from indirect is valueless.
  EXPECT_TRUE(b.valueless_after_move());
  // b-ref refers to the value it referred to before the move.
  EXPECT_EQ(br, 4);  // Perhaps this is surprising??

  // The `a` indirect and `a` reference-wrapper now refer to the moved-from
  // value.
  EXPECT_EQ(*a, 4);
  EXPECT_EQ(ar, 4);  // observing the lvalue via operator int&()
}

}  // namespace
