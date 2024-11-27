#include <variant>

#include "indirect.h"
namespace xyz::testing {

struct Number {};
struct BinOp;

struct Expression {
  std::variant<Number, xyz::indirect<BinOp>> info;
};

struct BinOp {};

static_assert(std::is_default_constructible_v<Expression>);
static_assert(std::is_copy_constructible_v<Expression>);

}  //  namespace xyz::testing
