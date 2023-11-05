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

ClassWithPolymorphicPimpl::ClassWithPolymorphicPimpl(
    ClassWithPolymorphicPimpl&&) noexcept = default;

ClassWithPolymorphicPimpl& ClassWithPolymorphicPimpl::operator=(
    ClassWithPolymorphicPimpl&&) noexcept = default;

void ClassWithPolymorphicPimpl::do_something() { impl_->do_something(); }
}  //  namespace xyz::testing
