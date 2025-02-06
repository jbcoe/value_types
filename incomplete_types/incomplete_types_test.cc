// Tests to ensure that composites containing incomplete types can be
// default-constructed, copied, and compared.
// Tese tests set a higher bar than pure compile checks as they only check that
// a class can be instantiated, not that it can be used in any way.

#include "incomplete_types.h"

#include <gtest/gtest.h>

namespace {

using xyz::testing::ConstrainedWrapperMember;
using xyz::testing::UniquePtrMember;
using xyz::testing::VariantMember;
using xyz::testing::VectorMember;
using xyz::testing::WrapperMember;

TEST(VectorMember, DefaultConstructCopyAndCompare) {
  VectorMember a;
  VectorMember aa(a);

  EXPECT_EQ(a, aa);
}

TEST(UniquePtrMember, DefaultConstructCopyAndCompare) {
  UniquePtrMember a;
  UniquePtrMember aa(a);

  EXPECT_EQ(a, aa);
}

TEST(WrapperMember, DefaultConstructCopyAndCompare) {
  WrapperMember a;
  WrapperMember aa(a);

  EXPECT_EQ(a, aa);
}

TEST(ConstrainedWrapperMember, DefaultConstructCopyAndCompare) {
  ConstrainedWrapperMember a;
  ConstrainedWrapperMember aa(a);

  EXPECT_EQ(a, aa);
}

TEST(VariantMember, DefaultConstructCopyAndCompare) {
  VariantMember a;
  VariantMember aa(a);

  EXPECT_EQ(a, aa);
}
}  // namespace
