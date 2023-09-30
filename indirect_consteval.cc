// A set of consteval functions to check that constexpr functions can be
// evaluated at compile time.

#include "indirect.h"

namespace {

consteval bool indirect_default_construction() {
  auto i = xyz::indirect<int>{};
  return true;
}
static_assert(indirect_default_construction(),
              "constexpr function call failed");

consteval bool indirect_in_place_construction() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  return true;
}
static_assert(indirect_in_place_construction(),
              "constexpr function call failed");

consteval bool indirect_allocator_construction() {
  auto i = xyz::indirect<int>(std::allocator_arg, std::allocator<int>{},
                              std::in_place, 42);
  return true;
}
static_assert(indirect_allocator_construction(),
              "constexpr function call failed");

consteval bool indirect_copy_construction() {
  auto i = xyz::indirect<int>{};
  auto ii = i;
  return true;
}
static_assert(indirect_copy_construction(), "constexpr function call failed");

consteval bool indirect_allocator_copy_construction() {
  auto i = xyz::indirect<int>{};
  auto ii = xyz::indirect<int>(std::allocator_arg, i.get_allocator(), i);
  return true;
}
static_assert(indirect_allocator_copy_construction(),
              "constexpr function call failed");

consteval bool indirect_move_construction() {
  auto i = xyz::indirect<int>{};
  auto ii = std::move(i);
  return true;
}
static_assert(indirect_move_construction(), "constexpr function call failed");

consteval bool indirect_allocator_move_construction() {
  auto i = xyz::indirect<int>{};
  auto ii =
      xyz::indirect<int>(std::allocator_arg, i.get_allocator(), std::move(i));
  return true;
}
static_assert(indirect_allocator_move_construction(),
              "constexpr function call failed");

consteval bool indirect_copy_assignment() {
  auto i = xyz::indirect<int>{};
  auto ii = xyz::indirect<int>{};
  i = ii;
  return true;
}
static_assert(indirect_copy_assignment(), "constexpr function call failed");

consteval bool indirect_move_assignment() {
  auto i = xyz::indirect<int>{};
  auto ii = xyz::indirect<int>{};
  i = std::move(ii);
  return true;
}
static_assert(indirect_move_assignment(), "constexpr function call failed");

consteval bool indirect_object_access() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  return *i == 42;
}
static_assert(indirect_object_access(), "constexpr function call failed");

consteval bool indirect_const_object_access() {
  const auto i = xyz::indirect<int>(std::in_place, 42);
  return *i == 42;
}
static_assert(indirect_const_object_access(), "constexpr function call failed");

consteval bool indirect_operator_arrow() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  return *(i.operator->()) == 42;
}
static_assert(indirect_operator_arrow(), "constexpr function call failed");

consteval bool indirect_const_operator_arrow() {
  const auto i = xyz::indirect<int>(std::in_place, 42);
  return *(i.operator->()) == 42;
}
static_assert(indirect_const_operator_arrow(),
              "constexpr function call failed");

consteval bool indirect_swap() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  auto ii = xyz::indirect<int>(std::in_place, 101);
  using std::swap;
  swap(i,ii);
  return *i == 101 && *ii == 42;
}
static_assert(indirect_swap(), "constexpr function call failed");

consteval bool indirect_member_swap() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  auto ii = xyz::indirect<int>(std::in_place, 101);
  i.swap(ii);
  return *i == 101 && *ii == 42;
}
static_assert(indirect_member_swap(), "constexpr function call failed");

consteval bool indirect_valueless_after_move() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  auto ii = std::move(i);
  return i.valueless_after_move() && !ii.valueless_after_move();
}
static_assert(indirect_valueless_after_move(), "constexpr function call failed");

consteval bool indirect_equality() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  auto ii = xyz::indirect<int>(std::in_place, 42);
  return i == ii;
}
static_assert(indirect_equality(), "constexpr function call failed");

consteval bool indirect_inequality() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  auto ii = xyz::indirect<int>(std::in_place, 101);
  return i != ii;
}
static_assert(indirect_inequality(), "constexpr function call failed");

consteval bool indirect_three_way_comparison() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  auto ii = xyz::indirect<int>(std::in_place, 101);
  return (i <=> ii) == (*i <=> *ii);
}
static_assert(indirect_three_way_comparison(),
              "constexpr function call failed");

consteval bool indirect_and_value_equality() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  int ii = 42;
  return (i == ii) && (ii == i);
}
static_assert(indirect_and_value_equality(),
              "constexpr function call failed");

consteval bool indirect_and_value_inequality() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  int ii = 101;
  return (i != ii) && (ii != i);
}
static_assert(indirect_and_value_inequality(),
              "constexpr function call failed");

consteval bool indirect_and_value_three_way_comparison() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  int ii = 101;
  return (i <=> ii) != (ii <=> i);
}

static_assert(indirect_and_value_three_way_comparison(),
              "constexpr function call failed");
}  // namespace
