#include "recursive_variant.h"

#include <gtest/gtest.h>

namespace {

using xyz::testing::ASTNode;
using xyz::testing::ASTNodeRecursiveStorage;
using xyz::testing::deref;

template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;

  overload(Ts&&... ts) : Ts(std::forward<Ts>(ts))... {}
};

TEST(RecursiveVariant, ExplicitAccess) {
  ASTNode node;

  // This is a pain to write and exposes implementation details.
  int result =
      std::visit(overload([](const int&) { return 0; },          //
                          [](const std::string&) { return 1; },  //
                          [](const ASTNodeRecursiveStorage&) { return 2; }),
                 node.data_);

  EXPECT_EQ(result, 0);
}

TEST(RecursiveVariant, DerefAccess) {
  ASTNode node;

  // This is nicer to write.
  int result = std::visit(overload([](const int&) { return 0; },          //
                                   [](const std::string&) { return 1; },  //
                                   [](const ASTNode&) { return 2; }),
                          node.data_);

  EXPECT_EQ(result, 0);
}

TEST(RecursiveVariant, LazyAccess) {
  ASTNode node;

  // This is lazy.
  int result = std::visit(overload([](const int&) { return 0; },          //
                                   [](const std::string&) { return 1; },  //
                                   [](const auto&) { return 2; }),
                          node.data_);

  EXPECT_EQ(result, 0);
}

}  // namespace
