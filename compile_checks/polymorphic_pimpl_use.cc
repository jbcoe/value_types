#include "polymorphic_pimpl.h"

namespace xyz::testing {
void check_methods() {
  xyz::testing::ClassWithPolymorphicPimpl c;      // Default construction.
  auto cc = c;                                    // Copy construction.
  auto mc = std::move(c);                         // Move construction.
  c = cc;                                         // Copy assignment.
  c = xyz::testing::ClassWithPolymorphicPimpl();  // Move assignment.
  c.do_something();
}
}  // namespace xyz::testing
