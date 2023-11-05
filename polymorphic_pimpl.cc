#include "polymorphic_pimpl.h"

namespace xyz::testing {

class Impl {
 public:
  void do_something();
};

ClassWithPolymorphicPimpl::ClassWithPolymorphicPimpl()
    : impl_(xyz::polymorphic<Impl>()) {}

ClassWithPolymorphicPimpl::~ClassWithPolymorphicPimpl() = default;

ClassWithPolymorphicPimpl::ClassWithPolymorphicPimpl(
    const ClassWithPolymorphicPimpl&) = default;

ClassWithPolymorphicPimpl& ClassWithPolymorphicPimpl::operator=(
    const ClassWithPolymorphicPimpl&) = default;

void ClassWithPolymorphicPimpl::do_something() { impl_->do_something(); }
}  //  namespace xyz::testing

void check_methods() {
  xyz::testing::ClassWithPolymorphicPimpl c; // Default construction.
  auto cc = c; // Copy construction.
  auto mc = std::move(c); // Move construction.
  c = cc; // Copy assignment.
  c = xyz::testing::ClassWithPolymorphicPimpl(); // Move assignment.
  c.do_something();
}
