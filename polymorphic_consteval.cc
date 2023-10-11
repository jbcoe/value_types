// A set of consteval functions to check that constexpr functions can be
// evaluated at compile time.

#include "polymorphic.h"

namespace {

class A {
  int value_ = 0;

 public:
  constexpr A() = default;
  constexpr A(int value) : value_(value) {}
  constexpr int value() const { return value_; }
};

consteval bool polymorphic_default_construction() {
  auto p = xyz::polymorphic<A>{};
  return true;
}
static_assert(polymorphic_default_construction(),
              "constexpr function call failed");

consteval bool polymorphic_in_place_construction() {
  auto p = xyz::polymorphic<A>(std::in_place_type<A>, 42);
  return true;
}
static_assert(polymorphic_in_place_construction(),
              "constexpr function call failed");

consteval bool polymorphic_allocator_construction() {
  auto p = xyz::polymorphic<A>(std::allocator_arg, std::allocator<int>{},
                               std::in_place_type<A>, 42);
  return true;
}
static_assert(polymorphic_allocator_construction(),
              "constexpr function call failed");

consteval bool polymorphic_copy_construction() {
  auto p = xyz::polymorphic<A>{};
  auto pp = p;
  return true;
}
static_assert(polymorphic_copy_construction(),
              "constexpr function call failed");

consteval bool polymorphic_allocator_copy_construction() {
  auto p = xyz::polymorphic<A>{};
  auto pp = xyz::polymorphic<A>(std::allocator_arg, p.get_allocator(), p);
  return true;
}
static_assert(polymorphic_allocator_copy_construction(),
              "constexpr function call failed");

consteval bool polymorphic_move_construction() {
  auto p = xyz::polymorphic<A>{};
  auto pp = std::move(p);
  return true;
}
static_assert(polymorphic_move_construction(),
              "constexpr function call failed");

consteval bool polymorphic_allocator_move_construction() {
  auto p = xyz::polymorphic<A>{};
  auto pp =
      xyz::polymorphic<A>(std::allocator_arg, p.get_allocator(), std::move(p));
  return true;
}
static_assert(polymorphic_allocator_move_construction(),
              "constexpr function call failed");

consteval bool polymorphic_copy_assignment() {
  auto p = xyz::polymorphic<A>{};
  auto pp = xyz::polymorphic<A>{};
  p = pp;
  return true;
}
static_assert(polymorphic_copy_assignment(), "constexpr function call failed");

consteval bool polymorphic_move_assignment() {
  auto p = xyz::polymorphic<A>{};
  auto pp = xyz::polymorphic<A>{};
  p = std::move(pp);
  return true;
}
static_assert(polymorphic_move_assignment(), "constexpr function call failed");

consteval bool polymorphic_object_access() {
  auto p = xyz::polymorphic<A>(std::in_place_type<A>, 42);
  return p->value() == 42;
}
static_assert(polymorphic_object_access(), "constexpr function call failed");

consteval bool polymorphic_const_object_access() {
  const auto p = xyz::polymorphic<A>(std::in_place_type<A>, 42);
  return p->value() == 42;
}
static_assert(polymorphic_const_object_access(),
              "constexpr function call failed");

consteval bool polymorphic_operator_arrow() {
  auto p = xyz::polymorphic<A>(std::in_place_type<A>, 42);
  return p->value() == 42;
}
static_assert(polymorphic_operator_arrow(), "constexpr function call failed");

consteval bool polymorphic_const_operator_arrow() {
  const auto p = xyz::polymorphic<A>(std::in_place_type<A>, 42);
  return p->value() == 42;
}
static_assert(polymorphic_const_operator_arrow(),
              "constexpr function call failed");

consteval bool polymorphic_swap() {
  auto p = xyz::polymorphic<A>(std::in_place_type<A>, 42);
  auto pp = xyz::polymorphic<A>(std::in_place_type<A>, 101);
  using std::swap;
  swap(p, pp);
  return p->value() == 101 && pp->value() == 42;
}
static_assert(polymorphic_swap(), "constexpr function call failed");

consteval bool polymorphic_member_swap() {
  auto p = xyz::polymorphic<A>(std::in_place_type<A>, 42);
  auto pp = xyz::polymorphic<A>(std::in_place_type<A>, 101);
  p.swap(pp);
  return p->value() == 101 && pp->value() == 42;
}
static_assert(polymorphic_member_swap(), "constexpr function call failed");

consteval bool polymorphic_valueless_after_move() {
  auto p = xyz::polymorphic<A>(std::in_place_type<A>, 42);
  auto pp = std::move(p);
  return p.valueless_after_move() && !pp.valueless_after_move();
}
static_assert(polymorphic_valueless_after_move(),
              "constexpr function call failed");
}  // namespace
