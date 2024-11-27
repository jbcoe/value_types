#include "indirect.h"

namespace xyz::testing {

struct number {};

struct Composite {
  xyz::indirect<struct A> data;
};

struct A {};

static_assert(std::is_copy_constructible_v<Composite>);
static_assert(std::is_default_constructible_v<Composite>);

}  //  namespace xyz::testing
