#include "indirect_pimpl.h"

namespace xyz::testing {

class Impl {
 public:
  void do_something();
};

Class::Class() : impl_(xyz::indirect<Impl>()) {}
Class::~Class() = default;
Class::Class(const Class&) = default;
Class& Class::operator=(const Class&) = default;
Class::Class(Class&&) noexcept = default;
Class& Class::operator=(Class&&) noexcept = default;

void Class::do_something() { impl_->do_something(); }
}  //  namespace xyz::testing

void check_methods() {
  xyz::testing::Class c;      // Default construction.
  auto cc = c;                // Copy construction.
  auto mc = std::move(c);     // Move construction.
  c = cc;                     // Copy assignment.
  c = xyz::testing::Class();  // Move assignment.
  c.do_something();
}
