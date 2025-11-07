// An investigation of recursive variants and an associated helper type.

#ifndef XYZ_EXPLORATION_RECURSIVE_VARIANT_H_
#define XYZ_EXPLORATION_RECURSIVE_VARIANT_H_

#include <string>
#include <variant>

#include "indirect.h"

namespace xyz::testing {

template <typename T>
struct deref {
  T t_;

  using U = decltype(*std::declval<T>());

  operator U&() { return *t_; }

  operator const U&() const { return *t_; }
};

struct ASTNode;
using ASTNodeRecursiveStorage = deref<xyz::indirect<ASTNode>>;
using ASTNodeData = std::variant<int, std::string, ASTNodeRecursiveStorage>;

struct ASTNode {
  ASTNodeData data_;
};

}  // namespace xyz::testing

#endif  // XYZ_EXPLORATION_RECURSIVE_VARIANT_H_
