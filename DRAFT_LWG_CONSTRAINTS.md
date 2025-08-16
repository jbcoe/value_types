# Constraints on incomplete types in `indirect` and `polymorphic`

**Section:** 20.4 `[mem.composite.types]`

**Submitter:** jonathanbcoe@gmail.com

**Opened:** 2025-08-16

## Problem

The class templates `indirect<T>` and `polymorphic<T>` allow the template argument `T`
to be an incomplete type.

Both classes can be instantiated when the type `T` is incomplete: constraints
are written so that requirements on incomplete types are not evaluated at class
instantiation time.

For constructors with additional template parameters, there are currently
constraints written on the potentially incomplete type `T` and the additional
template parameters. Such constraints will not be evaluated at class
instantiation time but could be explicitly evaluated in contexts where support
for an incomplete `T` is required.

```cpp
template <typename U>
class A {
    U u;
 public:
    A(const SomeType&) requires std::is_constructible_v<U, SomeType> {
        // ...
    }
};
```

when `U` is `indirect<T>` or `polymorphic<T>` for some type `T`, the existence
of the `requires` clause will require that `T` is a complete type for
constraints on `indirect` or `polymorphic` to be evaluated.

## Proposed Changes

_Constraints_ on `T` should be converted to _Mandates_ on `T` so that constraint
evaluation does not require `T` to be a complete type.

## Changes to `indirect`

Modify `indirect` constructors `[indirect.ctor]` as indicated:

```cpp
template<class U = T>
constexpr explicit indirect(U&& u);
```

_Constraints_:

* `is_same_v<remove_cvref_t<U>, indirect>` is `false`,
* `is_same_v<remove_cvref_t<U>, in_place_t>` is `false`,
* <s style="background-color: pink;"> `is_constructible_v<T, U>` is `true`</s>, and
* `is_default_constructible_v<Allocator>` is `true`.

<u style="background-color: lightgreen;"> _Mandates_: `is_constructible_v<T, U>` is `true`.</u>

_Effects_: Constructs an owned object of type `T` with `std::forward<U>(u)`, using the allocator `alloc`.


```cpp
template<class U = T>
constexpr explicit indirect(allocator_arg_t, const Allocator& a, U&& u);
```

_Constraints_:
* `is_same_v<remove_cvref_t<U>, indirect>` is `false`,
* `is_same_v<remove_cvref_t<U>, in_place_t>` is `false`, and
* <s style="background-color: pink;"> `is_constructible_v<T, U>` is `true`.</s>

<u style="background-color: lightgreen;"> _Mandates_: `is_constructible_v<T, U>` is `true`.</u>

_Effects_: `alloc` is direct-non-list-initialized with `a`. Constructs an owned object of type `T` with
`std::forward<U>(u)`, using the allocator `alloc`.

```cpp
template<class... Us>
constexpr explicit indirect(in_place_t, Us&&... us);
```

_Constraints_:
* <s style="background-color: pink;">`is_constructible_v<T, Us...>` is `true`, and </s>
* `is_default_constructible_v<Allocator>` is `true`.

<u style="background-color: lightgreen;">_Mandates_: `is_constructible_v<T, Us ... >` is `true`.</u>

_Effects_: Constructs an owned object of type `T` with `std::forward<Us>(us)...`, using the allocator
`alloc`.

```cpp
template<class... Us>
constexpr explicit indirect(allocator_arg_t, const Allocator& a,
in_place_t, Us&& ...us);
```

<s style="background-color: pink;">_Constraints_: `is_constructible_v<T, Us...>` is `true`.</s>

<u style="background-color: lightgreen;">_Mandates_: `is_constructible_v<T, Us...>` is `true`.</u>

_Effects_: `alloc` is direct-non-list-initialized with `a`. Constructs an owned object of type `T` with
`std::forward<Us>(us)...`, using the allocator `alloc`.

```cpp
template<class I, class... Us>
constexpr explicit indirect(in_place_t, initializer_list<I> ilist, Us&&... us);
```

_Constraints_:
* <s style="background-color: pink;"> `is_constructible_v<T, initializer_list<I>&, Us...>` is `true`, and</s>
* `is_default_constructible_v<Allocator>` is `true`.

<u style="background-color: lightgreen;">_Mandates_: `is_constructible_v<T, initializer_list<I>&, Us...>` is `true`.</u>

_Effects_: Constructs an owned object of type `T` with the arguments `ilist`, `std::forward<Us>(us)...`,
using the allocator `alloc`.

```cpp
template<class I, class... Us>
constexpr explicit indirect(allocator_arg_t, const Allocator& a,
in_place_t, initializer_list<I> ilist, Us&&... us);
```

<s style="background-color: pink;">_Constraints_: `is_constructible_v<T, initializer_list<I>&, Us...>` is `true`.</s>

<u style="background-color: lightgreen;">_Mandates_: `is_constructible_v<T, initializer_list<I>&, Us...>` is `true`.</u>

_Effects_: `alloc` is direct-non-list-initialized with `a`. Constructs an owned object of type `T` with the
arguments `ilist`, `std::forward<Us>(us)...`, using the allocator `alloc`.

### Changes to `polymorphic`

Modify `polymorphic` constructors as indicated:

<!-- POLYMORPHIC CHANGES -->
