// Tests to ensure that composites containing incomplete types can be
// default-constructed, copied, and compared.
// Tese tests set a higher bar than pure compile checks as they only check that
// a class can be instantiated, not that it can be used in any way.

#include "incomplete_types.h"

#include <gtest/gtest.h>

namespace {

using xyz::testing::ConstrainedWrapperMember;
using xyz::testing::UniquePtrMember;
using xyz::testing::VariantCWrapperMember;
using xyz::testing::VariantVectorMember;
using xyz::testing::VariantWrapperMember;
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

TEST(VariantWrapperMember, DefaultConstructCopyAndCompare) {
  VariantWrapperMember a;
  VariantWrapperMember aa(a);

  EXPECT_EQ(a, aa);
}

TEST(VariantCWrapperMember, DefaultConstructCopyAndCompare) {
  VariantCWrapperMember a;
  VariantCWrapperMember aa(a);

  EXPECT_EQ(a, aa);
}

TEST(VariantVectorMember, DefaultConstructCopyAndCompare) {
  VariantVectorMember a;
  VariantVectorMember aa(a);

  EXPECT_EQ(a, aa);
}
}  // namespace
