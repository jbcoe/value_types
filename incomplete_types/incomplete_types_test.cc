#include "incomplete_types.h"

#include <gtest/gtest.h>

namespace {

using xyz::testing::VectorMember;
// using xyz::testing::ConstrainedWrapperMember;
// using xyz::testing::UniquePtrMember;
// using xyz::testing::WrapperMember;

TEST(VectorMember, DefaultConstructCopyAndCompare) {
  VectorMember a;
  VectorMember aa(a);

  EXPECT_EQ(a, aa);
}

// TEST(UniquePtrMember, DefaultConstructCopyAndCompare) {
//   UniquePtrMember a;
//   UniquePtrMember aa(a);

//   EXPECT_EQ(a, aa);
// }

// TEST(WrapperMember, DefaultConstructCopyAndCompare) {
//   WrapperMember a;
//   WrapperMember aa(a);

//   EXPECT_EQ(a, aa);
// }

// TEST(ConstrainedWrapperMember, DefaultConstructCopyAndCompare) {
//   ConstrainedWrapperMember a;
//   ConstrainedWrapperMember aa(a);

//   EXPECT_EQ(a, aa);
// }
}  // namespace
