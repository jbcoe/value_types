// An investigation of recursive variants and an associated helper type.
#include <string>
#include <variant>

#include "indirect.h"

namespace xyz::testing {

template <class... Ts>
struct overload : Ts... {
  using Ts::operator()...;

  overload(Ts&&... ts) : Ts(std::forward<Ts>(ts))... {}
};

template <class... Ts>
overload(Ts...) -> overload<Ts...>;

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
