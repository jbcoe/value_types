#include <concepts>
#include <type_traits>
#include <variant>

#include "indirect.h"

namespace xyz::testing {

struct Number {
  friend bool operator==(const Number&, const Number&) = default;
};

struct BinOp;

struct Expression {
  std::variant<xyz::indirect<BinOp>, Number> info;

  friend bool operator==(const Expression&, const Expression&) = default;
};

struct BinOp {
  friend bool operator==(const BinOp&, const BinOp&) = default;
};

static_assert(std::is_default_constructible_v<Expression>);
static_assert(std::is_copy_constructible_v<Expression>);
static_assert(std::equality_comparable<Expression>);

}  //  namespace xyz::testing
