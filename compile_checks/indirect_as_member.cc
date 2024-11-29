#include <concepts>
#include <type_traits>

#include "indirect.h"
namespace xyz::testing {

struct A;

struct Composite {
  xyz::indirect<A> data;

  Composite();
  Composite(const Composite&);
  Composite(Composite&&);
  Composite& operator=(const Composite&);
  Composite& operator=(Composite&&);

  friend bool operator==(const Composite&, const Composite&);
  friend std::strong_ordering operator<=>(const Composite&, const Composite&);
};

struct A {
  friend bool operator==(const A&, const A&);
  friend std::strong_ordering operator<=>(const A&, const A&);
};

static_assert(std::is_default_constructible_v<Composite>);
static_assert(std::is_copy_constructible_v<Composite>);
static_assert(std::equality_comparable<Composite>);
static_assert(std::three_way_comparable<Composite>);

}  //  namespace xyz::testing
