---
title: "Constraints on incomplete types in `indirect` and `polymorphic`"
subtitle: ""
document: XXXXXX
date: 16 August 2025
audience:
  - Library Working Groupw
revises: D0000R0
author:
  - name: Joanthan B. Coe
    email: <jonathanbcoe0@gmail.com>
  - name: Antony Peacock
    email: <ant.peacock@gmail.com>
toc: true
toc-depth: 2
---

# Issue summary

This issue addresses...

```cpp
#include <iostream>
#include "foo.h"

__FILE__;

int x = 42'234'234;
const int x = 42ul;
const int x = 0B01011;

bool b = true;

struct process {
  hello @[`constexpr`]{.add}@ detail::foo::template foo;

  [[using CC: opt(1), debug]] x;

  template <typename I>
  [[nodiscard]] auto operator()(I i) -> O<I> { /* ... */ };

  x@~_i_~@ <=> y@~_i_~@;
};

if (x) {
  return ""sv;
  return 5ms;
}

std::printf("%d", x);

std::variant<I1, I2> input = 'h';
std::variant<I1, I2> input = "h";
std::variant<I1, I2> input = "hello";

// mapping from a `variant` of inputs to a `variant` of results:
auto output = std::visit<std::variant<O<I1>, O<I2>>>(process{}, input);

// coercing different results to a common type:
auto result = std::visit<std::common_type_t<O<I1>, O<I2>>>(process{}, input);

// visiting a `variant` for the side-effects, discarding results:
std::visit<void>(process{}, input);

@@[`namespace @_unspecified_@ { struct sender_base {}; }`]{.add}@@
@@[`using @_unspecified_@::sender_base;`]{.add}@@
```

## `diff` Syntax Highlighting

```diff
some things just don't change.

// 20.3.4 tuple-like access to pair:
- constexpr typename tuple_element<I, std::pair<T1, T2> >::type&
+ constexpr tuple_element_t<I, pair<T1, T2> >&
-   get(std::pair<T1, T2>&) noexcept;
+   get(pair<T1, T2>&) noexcept;

@_unspecified_@ detail::foo::template foo;
+ @_unspecified_@ detail::foo::template foo;
- @_unspecified_@ detail::foo::template foo;
```

::: add
[2] _Mandates_: `T` must not be a hedgehog.
:::

::: rm
[2] _Constraints_: `T` must not be a hedgehog.
:::

## Bulk cut and paste of wording from draft

20.4.1.3 Constructors [indirect.ctor]
1 The following element applies to all functions in 20.4.1.3:
2 Throws: Nothing unless allocator_traits<Allocator>::allocate or allocator_traits<Alloca-
tor>::construct throws.
constexpr explicit indirect();
3 Constraints: is_default_constructible_v<Allocator> is true.
4 Mandates: is_default_constructible_v<T> is true.
5 Effects: Constructs an owned object of type T with an empty argument list, using the allocator alloc.
constexpr explicit indirect(allocator_arg_t, const Allocator& a);
6 Mandates: is_default_constructible_v<T> is true.
7 Effects: alloc is direct-non-list-initialized with a. Constructs an owned object of type T with an empty
argument list, using the allocator alloc.
constexpr indirect(const indirect& other);
8 Mandates: is_copy_constructible_v<T> is true.
9 Effects: alloc is direct-non-list-initialized with allocator_traits<Allocator>::select_on_contai-
ner_copy_construction(other.alloc ). If other is valueless, *this is valueless. Otherwise, con-
structs an owned object of type T with *other, using the allocator alloc.
constexpr indirect(allocator_arg_t, const Allocator& a, const indirect& other);
10 Mandates: is_copy_constructible_v<T> is true.
11 Effects: alloc is direct-non-list-initialized with a. If other is valueless, *this is valueless. Otherwise,
constructs an owned object of type T with *other, using the allocator alloc.
constexpr indirect(indirect&& other) noexcept;
12 Effects: alloc is direct-non-list-initialized from std::move(other.alloc ). If other is valueless,
*this is valueless. Otherwise *this takes ownership of the owned object of other.
13 Postconditions: other is valueless.
constexpr indirect(allocator_arg_t, const Allocator& a, indirect&& other)
noexcept(allocator_traits<Allocator>::is_always_equal::value);
14 Mandates: If allocator_traits<Allocator>::is_always_equal::value is false then T is a com-
plete type.
15 Effects: alloc is direct-non-list-initialized with a. If other is valueless, *this is valueless. Otherwise,
if alloc == other.alloc is true, constructs an object of type indirect that takes ownership of the
owned object of other. Otherwise, constructs an owned object of type T with *std::move(other),
using the allocator alloc.
16 Postconditions: other is valueless.
template<class U = T>
constexpr explicit indirect(U&& u);
17 Constraints:
(17.1) is_same_v<remove_cvref_t<U>, indirect> is false,
—
—
(17.2) is_same_v<remove_cvref_t<U>, in_place_t> is false,
(17.3) —
is_constructible_v<T, U> is true, and
(17.4) is_default_constructible_v<Allocator> is true.
—
18 Effects: Constructs an owned object of type T with std::forward<U>(u), using the allocator alloc.
template<class U = T>
constexpr explicit indirect(allocator_arg_t, const Allocator& a, U&& u);
19 Constraints:
(19.1) is_same_v<remove_cvref_t<U>, indirect> is false,
—
(19.2) —
is_same_v<remove_cvref_t<U>, in_place_t> is false, and
(19.3) is_constructible_v<T, U> is true.
—
20 Effects: alloc is direct-non-list-initialized with a. Constructs an owned object of type T with
std::forward<U>(u), using the allocator alloc.
template<class... Us>
constexpr explicit indirect(in_place_t, Us&&... us);
21 Constraints:
(21.1) —
is_constructible_v<T, Us...> is true, and
(21.2) is_default_constructible_v<Allocator> is true.
—
22 Effects: Constructs an owned object of type T with std::forward<Us>(us)..., using the allocator
alloc.
23 template<class... Us>
constexpr explicit indirect(allocator_arg_t, const Allocator& a,
in_place_t, Us&& ...us);
Constraints: is_constructible_v<T, Us...> is true.
24 Effects: alloc is direct-non-list-initialized with a. Constructs an owned object of type T with
std::forward<Us>(us)..., using the allocator alloc.
template<class I, class... Us>
constexpr explicit indirect(in_place_t, initializer_list<I> ilist, Us&&... us);
25 Constraints:
(25.1) —
is_constructible_v<T, initializer_list<I>&, Us...> is true, and
(25.2) is_default_constructible_v<Allocator> is true.
—
26 Effects: Constructs an owned object of type T with the arguments ilist, std::forward<Us>(us)...,
using the allocator alloc.
27 28 template<class I, class... Us>
constexpr explicit indirect(allocator_arg_t, const Allocator& a,
in_place_t, initializer_list<I> ilist, Us&&... us);
Constraints: is_constructible_v<T, initializer_list<I>&, Us...> is true.
Effects: alloc is direct-non-list-initialized with a. Constructs an owned object of type T with the
arguments ilist, std::forward<Us>(us)..., using the allocator alloc.

20.4.2.3 Constructors [polymorphic.ctor]
1 The following element applies to all functions in 20.4.2.3:
2 Throws: Nothing unless allocator_traits<Allocator>::allocate or allocator_traits<Alloca-
tor>::construct throws.
constexpr explicit polymorphic();
3 Constraints: is_default_constructible_v<Allocator> is true.
4 Mandates:
(4.1) is_default_constructible_v<T> is true, and
—
—
(4.2) is_copy_constructible_v<T> is true.
5 Effects: Constructs an owned object of type T with an empty argument list using the allocator alloc.
constexpr explicit polymorphic(allocator_arg_t, const Allocator& a);
6 Mandates:
(6.1) is_default_constructible_v<T> is true, and
—
—
(6.2) is_copy_constructible_v<T> is true.
7 Effects: alloc is direct-non-list-initialized with a. Constructs an owned object of type T with an empty
argument list using the allocator alloc.
constexpr polymorphic(const polymorphic& other);
8 Effects: alloc is direct-non-list-initialized with allocator_traits<Allocator>::select_on_contai-
ner_copy_construction(other.alloc ). If other is valueless, *this is valueless. Otherwise, con-
structs an owned object of type U, where U is the type of the owned object in other, with the owned
object in other using the allocator alloc.
constexpr polymorphic(allocator_arg_t, const Allocator& a, const polymorphic& other);
9 Effects: alloc is direct-non-list-initialized with a. If other is valueless, *this is valueless. Otherwise,
constructs an owned object of type U, where U is the type of the owned object in other, with the owned
object in other using the allocator alloc.
10 constexpr polymorphic(polymorphic&& other) noexcept;
Effects: alloc is direct-non-list-initialized with std::move(other.alloc ). If other is valueless,
*this is valueless. Otherwise, either *this takes ownership of the owned object of other or, owns an
object of the same type constructed from the owned object of other considering that owned object as
an rvalue, using the allocator alloc.
11 constexpr polymorphic(allocator_arg_t, const Allocator& a, polymorphic&& other)
noexcept(allocator_traits<Allocator>::is_always_equal::value);
Effects: alloc is direct-non-list-initialized with a. If other is valueless, *this is valueless. Otherwise,
if alloc == other.alloc is true, either constructs an object of type polymorphic that owns the
owned object of other, making other valueless; or, owns an object of the same type constructed
from the owned object of other considering that owned object as an rvalue. Otherwise, if alloc
!= other.alloc is true, constructs an object of type polymorphic, considering the owned object in
other as an rvalue, using the allocator alloc.
template<class U = T>
constexpr explicit polymorphic(U&& u);
12 Constraints: Where UU is remove_cvref_t<U>,
—
(12.1) is_same_v<UU, polymorphic> is false,
(12.2) derived_from<UU, T> is true,
—
§ 20.4.2.3 © ISO/IEC
716
Dxxxx
(12.3) is_constructible_v<UU, U> is true,
—
—
(12.4) is_copy_constructible_v<UU> is true,
(12.5) —
UU is not a specialization of in_place_type_t, and
(12.6) is_default_constructible_v<Allocator> is true.
—
13 Effects: Constructs an owned object of type UU with std::forward<U>(u) using the allocator alloc.
template<class U = T>
constexpr explicit polymorphic(allocator_arg_t, const Allocator& a, U&& u);
14 Constraints: Where UU is remove_cvref_t<U>,
—
(14.1) is_same_v<UU, polymorphic> is false,
(14.2) derived_from<UU, T> is true,
—
(14.3) is_constructible_v<UU, U> is true,
—
—
(14.4) is_copy_constructible_v<UU> is true, and
(14.5) —
UU is not a specialization of in_place_type_t.
15 Effects: alloc is direct-non-list-initialized with a. Constructs an owned object of type UU with
std::forward<U>(u) using the allocator alloc.
template<class U, class... Ts>
constexpr explicit polymorphic(in_place_type_t<U>, Ts&&... ts);
16 Constraints:
(16.1) is_same_v<remove_cvref_t<U>, U> is true,
—
(16.2) derived_from<U, T> is true,
—
(16.3) is_constructible_v<U, Ts...> is true,
—
—
(16.4) is_copy_constructible_v<U> is true, and
(16.5) is_default_constructible_v<Allocator> is true.
—
17 Effects: Constructs an owned object of type U with std::forward<Ts>(ts)... using the allocator
alloc.
template<class U, class... Ts>
constexpr explicit polymorphic(allocator_arg_t, const Allocator& a,
in_place_type_t<U>, Ts&&... ts);
18 Constraints:
(18.1) is_same_v<remove_cvref_t<U>, U> is true,
—
(18.2) derived_from<U, T> is true,
—
(18.3) —
is_constructible_v<U, Ts...> is true, and
—
(18.4) is_copy_constructible_v<U> is true.
19 Effects: alloc is direct-non-list-initialized with a. Constructs an owned object of type U with
std::forward<Ts>(ts)... using the allocator alloc.
template<class U, class I, class... Us>
constexpr explicit polymorphic(in_place_type_t<U>, initializer_list<I> ilist, Us&&... us);
20 Constraints:
(20.1) is_same_v<remove_cvref_t<U>, U> is true,
—
(20.2) derived_from<U, T> is true,
—
(20.3) —
is_constructible_v<U, initializer_list<I>&, Us...> is true,
—
(20.4) is_copy_constructible_v<U> is true, and
(20.5) is_default_constructible_v<Allocator> is true.
—
21 Effects: Constructs an owned object of type U with the arguments ilist, std::forward<Us>(us)...
using the allocator alloc.
§ 20.4.2.3 © ISO/IEC
717
Dxxxx
template<class U, class I, class... Us>
constexpr explicit polymorphic(allocator_arg_t, const Allocator& a,
in_place_type_t<U>, initializer_list<I> ilist, Us&&... us);
22 Constraints:
(22.1) is_same_v<remove_cvref_t<U>, U> is true,
—
(22.2) derived_from<U, T> is true,
—
(22.3) —
is_constructible_v<U, initializer_list<I>&, Us...> is true, and
—
(22.4) is_copy_constructible_v<U> is true.
23 Effects: alloc is direct-non-list-initialized with a. Constructs an owned object of type U with the
arguments ilist, std::forward<Us>(us)... using the allocator alloc.
