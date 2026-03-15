#include <iostream>

#include "xyz/value_types/indirect.h"
#include "xyz/value_types/polymorphic.h"

struct A {
  virtual ~A() = default;

  virtual void print() const { std::cout << "A\n"; }
};

struct B : A {
  void print() const override { std::cout << "B\n"; }
};

int main() {
  xyz::indirect<int> i(std::in_place, 42);
  xyz::polymorphic<A> p(std::in_place_type<B>);
  p->print();
  return 0;
}
