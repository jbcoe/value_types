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

void Class::do_something() { impl_->do_something(); }
}  //  namespace xyz::testing
