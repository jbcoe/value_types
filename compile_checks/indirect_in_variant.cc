#include <concepts>
#include <type_traits>
#include <variant>

#include "indirect.h"

namespace xyz::testing {

struct Number {
  friend bool operator==(const Number&, const Number&);
  friend std::strong_ordering operator<=>(const Number&, const Number&);
};

struct BinOp;

struct Expression {
  std::variant<Number, xyz::indirect<BinOp>> info;

  friend bool operator==(const Expression&, const Expression&);
  friend std::strong_ordering operator<=>(const Expression&, const Expression&);
};

struct BinOp {
  friend bool operator==(const BinOp&, const BinOp&);
  friend std::strong_ordering operator<=>(const BinOp&, const BinOp&);
};

static_assert(std::is_default_constructible_v<Expression>);
static_assert(std::is_copy_constructible_v<Expression>);
static_assert(std::equality_comparable<Expression>);
static_assert(std::three_way_comparable<Expression>);

}  //  namespace xyz::testing
