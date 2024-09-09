/* Copyright (c) 2016 The Value Types Authors. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
==============================================================================*/

// A set of consteval functions to check that constexpr functions can be
// evaluated at compile time.

#include "indirect.h"

namespace xyz::testing {
struct ConstexprHashable {};
}  // namespace xyz::testing
template <>
struct std::hash<xyz::testing::ConstexprHashable> {
  constexpr std::size_t operator()(
      const xyz::testing::ConstexprHashable&) const {
    return 0;
  }
};

namespace xyz::testing {

consteval bool indirect_default_construction() {
  auto i = xyz::indirect<int>{};
  return true;
}
static_assert(indirect_default_construction(),
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

consteval bool indirect_construction() {
  auto i = xyz::indirect<int>(std::in_place, 42);
  return true;
}
static_assert(indirect_construction(), "constexpr function call failed");

consteval bool indirect_allocator_construction() {
  auto i = xyz::indirect<int>(std::allocator_arg, std::allocator<int>{},
                              std::in_place, 42);
  return true;
}
static_assert(indirect_allocator_construction(),
              "constexpr function call failed");

consteval bool indirect_converting_construction() {
  auto i = xyz::indirect<int>(42);
  return true;
}
static_assert(indirect_converting_construction(),
              "constexpr function call failed");

consteval bool indirect_allocator_converting_construction() {
  auto i = xyz::indirect<int>(std::allocator_arg, std::allocator<int>{}, 42);
  return true;
}
static_assert(indirect_allocator_converting_construction(),
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

consteval bool indirect_converting_assignment() {
  auto i = xyz::indirect<int>{};
  i = 42;
  return true;
}
static_assert(indirect_converting_assignment(),
              "constexpr function call failed");

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
  swap(i, ii);
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
static_assert(indirect_valueless_after_move(),
              "constexpr function call failed");

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
static_assert(indirect_and_value_equality(), "constexpr function call failed");

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

consteval bool indirect_hash() {
  auto i = xyz::indirect<ConstexprHashable>();
  std::hash<xyz::indirect<ConstexprHashable>> h;
  return h(i) == 0;
}
static_assert(indirect_hash(), "constexpr function call failed");

}  // namespace xyz::testing
