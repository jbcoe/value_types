#include <concepts>
#include <type_traits>

#include "indirect.h"
namespace xyz::testing {

struct number {};

struct Composite {
  xyz::indirect<struct A> data;
  friend bool operator==(const Composite&, const Composite&) = default;
};

struct A {
  friend bool operator==(const A&, const A&) = default;
};

static_assert(std::is_default_constructible_v<Composite>);
static_assert(std::is_copy_constructible_v<Composite>);
static_assert(std::equality_comparable<Composite>);

}  //  namespace xyz::testing
