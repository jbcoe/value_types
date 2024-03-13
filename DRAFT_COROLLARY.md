# Add missing constructors and assignment for `indirect` and `polymorphic`
 <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

ISO/IEC JTC1 SC22 WG21 Programming Language C++

D3152R0

Working Group: Library Evolution, Library

Date: 2024-03-18

_Jonathan Coe \<<jonathanbcoe@gmail.com>\>_

_Antony Peacock \<<ant.peacock@gmail.com>\>_

_Sean Parent \<<sparent@adobe.com>\>_

# Contents
 - [Introduction](#introduction)
 - [Additional constructors](#additional-constructors)
 - [Converting assignment](#converting-assignment)
 - [Technical specifications](#technical-specifications)
 - [Reference implementation](#reference-implementation)
 - [Acknowledgements](#acknowledgements)
 - [References](#references)
 - [Appendix A: complete class template specifications](
   #appendix-a-complete-class-template-specifications)
 - [Appendix B: design choices, alternatives and breaking changes](
   #appendix-b-design-choices-alternatives-and-breaking-changes)

[//]: <> (<=============================================================================>)

## Introduction
New vocabulary types `indirect` and `polymorphic` for composite class design are
proposed in [P3019R6]. Based on recommendations from LEWG, we follow up this work by
adding support for converting construction, initializer-list construction and
converting assignment to `indirect` and  `polymorphic`. These additions are the
focus of this corollary proposal. Please refer to [P3019R6] for details on `indirect`
and `polymorphic`.

## Additional constructors

### Converting constructors

We add converting constructors to support conversion from `T` to `indirect<T>`
or `polymorphic<T>`. Since these operations allocate memory, the constructors
are marked explicit so the intent to use them is clear.

//TODO:ADD examples before/after with/without allocator

#### Before and after converting constructors

```c++
int i = 42;
indirect<int> i1 = r; // before, error
indirect<int> i2(r); // after, supported

polymorphic<Shape> s1 = r; // before, error
polymorphic<Shape> s2(r); // after, supported
```

### Initializer-list constructors

//TODO:ADD before/after examples with/without allocator

As stated in [P3019R6], class templates `indirect` and `polymorphic` have strong
similarities to existing class templates by design. To ensure consistency with
existing library types, we add support for list-initialized constructors to
`indirect` and `polymorphic`.

#### Before and after initializer-list constructors

```c++

indirect<> ??? ; // before, error
indirect<> ??? ; // after, supported

polymorphic<Shape> ???; // before, error
polymorphic<Shape> ???; // after, supported
```

## Converting assignment

//TODO:ADD motivation and examples before/after

#### Before and after converting assignment

```c++

indirect<> ??? ; // before, error
indirect<> ??? ; // after, supported

polymorphic<Shape> ???; // before, error
polymorphic<Shape> ???; // after, supported
```

## Technical specifications

We update the technical specifications detailed in [P3019R6] to include
the constructors and assignment operators discussed above. Please see
[Appendix A](#appendix-a) for the complete class templates.

### X.Y Class template indirect [indirect]

#### X.Y.1 Class template indirect additions [indirect.add???]

```c++
template <class T, class Allocator = allocator<T>>
class indirect {
 public:
  using value_type = T;
  using allocator_type = Allocator;
  using pointer = typename allocator_traits<Allocator>::pointer;
  using const_pointer = typename allocator_traits<Allocator>::const_pointer;

  ...

  //TODO:CHECK here
  template <class U>
  explicit constexpr indirect(U&& u);

  //TODO:CHECK here
  template <class U>
  explicit constexpr indirect(allocator_arg_t, const Allocator& a,
                              in_place_t, U&& u);
  ...

  //TODO:CHECK here
  template<class U, class... Us>
  explicit constexpr indirect(in_place_t, std::initializer_list<U> ilist,
                              Us&&... us);

  //TODO:CHECK here
  template<class U, class... Us>
  explicit constexpr indirect(allocator_arg_t, const Allocator& a,
                              in_place_t, std::initializer_list<U> ilist,
                              Us&&... us);
  ...

  //TODO:CHECK here
  template <class U>
  constexpr indirect& operator=(U&& u);

  ...

private:
  pointer p; // exposition only
  Allocator alloc; // exposition only
};

...

//TODO:CHECK here
template <typename Value>
indirect(std::in_place_t, Value) -> indirect<Value>;

...
```

### X.Y.2 Constructors [indirect.ctor]

//TODO:CHECK

```c++
template <class U = T>
explicit constexpr indirect(U&& u);
```

10.1?. _Constraints_: `is_same_v<T, remove_cvref_t<U>>` is `true`.
   `is_constructible_v<T, U>`.
   `is_copy_constructible_v<U>` is `true`.
   `is_same_v<remove_cvref_t<U>, in_place_t>` is `false`.
   `is_same_v<remove_cvref_t<U>, indirect>` is `false`.
   `is_default_constructible_v<allocator_type>` is `true`.

10.2?. _Mandates_: `T` is a complete type.

10.3?. _Effects_: Equivalent to `indirect(allocator_arg_t{}, Allocator(),
   in_place_t{}, std::forward<U>(u))`.

```c++
template <class U = T>
explicit constexpr indirect(allocator_arg_t, const Allocator& a,
                            in_place_t, U&& u);
```

10.4?. _Constraints_: `is_same_v<T, remove_cvref_t<U>>` is `true`.
   `is_constructible_v<T, U>`.
   `is_copy_constructible_v<U>` is `true`.
   `is_same_v<remove_cvref_t<U>, in_place_t>` is `false`.
   `is_same_v<remove_cvref_t<U>, indirect>` is `false`.

10.5?. _Mandates_: `T` is a complete type.

10.6?. _Effects_: `alloc` is direct-non-list-initialized with `a`.

```c++
template<class U = T, class... Us>
explicit constexpr indirect(in_place_t, std::initializer_list<U> ilist,
                            Us&&... us);
```

13.1?. _Constraints_: `is_same_v<T, remove_cvref_t<U>>` is `true`.
   `is_copy_constructible_v<U>` is `true`.
   `is_constructible_v<T, initializer_list<I>&, Us...>` is `true`.
   `is_default_constructible_v<allocator_type>` is `true`.

13.2?. _Mandates_: `T` is a complete type.

13.3?. _Effects_: Value direct-non-list-initializes an owned object
   of type `T` with arguments `ilist`, `std::forward<Us>(us...)`.
   Equivalent to `indirect(allocator_arg_t{}, Allocator(),
   in_place_t{}, std::forward<initializer_list<U>>(ilist),
   std::forward<Us>(us))`.

```c++
template<class U = T, class... Us>
explicit constexpr indirect(allocator_arg_t, const Allocator& a,
                            in_place_t, std::initializer_list<U> ilist,
                            Us&&... us);
```

13.4?. _Constraints_: `is_same_v<T, remove_cvref_t<U>>` is `true`.
   `is_copy_constructible_v<U>` is `true`.
   `is_constructible_v<T, initializer_list<I>&, Us...>` is `true`.

13.5?. _Mandates_: `T` is a complete type.

13.6?. _Effects_: Value direct-non-list-initializes an owned object
   of type `T` with arguments `ilist`, `std::forward<Us>(us)`.
   `alloc` is direct-non-list-initialized with `a`.


### X.Y.3 Assignment [indirect.assign]

//TODO:REVISE

```c++
template <class U>
constexpr indirect& operator=(U&& u);
```

0.0? _Mandates_: `T` is a complete type.

0.1? _Effects_: If `valueless_after_move()` then move assigns from
   `polymorphic<T>(std::in_place_type_t<std::remove_cvref_t<U>>{}, u)`.
   If not valueless, then call the underlying move assignment on `U`.
   No effects if an exception is thrown.

0.2? _Returns_: A reference to `*this`.


### X.Y Class template polymorphic [polymorphic]

#### X.Y.1 Class template polymorphic additions [polymorphic.add??]

```c++
template <class T, class Allocator = allocator<T>>
class polymorphic {
  Allocator alloc; // exposition only
 public:
  using value_type = T;
  using allocator_type = Allocator;
  using pointer = typename allocator_traits<Allocator>::pointer;
  using const_pointer  = typename allocator_traits<Allocator>::const_pointer;

  ...

  //TODO:CHECK here
  template <class U>
  explicit constexpr polymorphic(U&& u);

  //TODO:CHECK here
  template <class U>
  explicit constexpr polymorphic(allocator_arg_t, const Allocator& a,
                                 in_place_type_t<U>, U&& u);
  ...

  //TODO:CHECK here
  template <class U, class I, class... Us>
  explicitconstexpr polymorphic(in_place_type_t<U>,
                                initializer_list<I> ilist, Us&&... us)

  //TODO:CHECK here
  template <class U, class I, class... Us>
  explicit constexpr polymorphic(allocator_arg_t, const Allocator& a,
                                 in_place_type_t<U>,
                                 initializer_list<I> ilist, Us&&... us)
  ...

  //TODO:CHECK here
  template <class U>
  constexpr polymorphic& operator=(U&& u);

  ...
};
```

#### X.Z.2 Constructors [polymorphic.ctor]

//TODO:CHECK

```c++
template <class U>
explicit constexpr polymorphic(U&& u);
```

16.1? _Constraints_: `is_same_v<polymorphic, remove_cvref_t<U>>` is `false`.
   `is_copy_constructible_v<remove_cvref_t<U>>` is `true`.
   `is_base_of_v<T, std::remove_cvref_t<U>>` is `true`.
   `is_default_constructible_v<allocator_type>` is `true`.

16.2? _Mandates_: `T` is a complete type.

16.3? _Effects_: Equivalent to `polymorphic(std::allocator_arg_t{}, A{},
   std::in_place_type_t<std::remove_cvref_t<U>>{}, std::forward<U>(u))`.


```c++
template <class U>
explicit constexpr polymorphic(allocator_arg_t, const Allocator& a,
                               in_place_type_t<U>, U&& u);
```

16.4? _Constraints_: `is_same_v<polymorphic, remove_cvref_t<U>>` is `false`.
   `is_copy_constructible_v<remove_cvref_t<U>>` is `true`.
   `is_base_of_v<T, std::remove_cvref_t<U>>` is `true`.

16.5? _Mandates_: `T` is a complete type.

16.6? _Effects_: `alloc` is direct-non-list-initialized with `a`.


```c++
  template <class U = T, class I, class... Us>
  explicit constexpr polymorphic(in_place_type_t<U>,
                                 initializer_list<I> ilist, Us&&... us)
```
19.1? _Constraints_: ?? `is_base_of_v<T, U>` is `true`.
  `is_constructible_v<U, Us...>` is `true`.
  `is_copy_constructible_v<U>` is `true`.
  `is_constructible_v<T, initializer_list<I>&, Us...>` is `true`.
  `is_default_constructible_v<allocator_type>` is `true`.

19.2? _Mandates_: `T` is a complete type.

19.3? _Effects_: Equivalent to `polymorphic(std::allocator_arg_t{}, A{},
  std::in_place_type_t<std::remove_cvref_t<U>>{}, std::forward<U>(u),
  std::forward<initializer_list<I>>(ilist), std::forward<Us>(us))`.
  ??? Value direct-non-list-initializes an owned object of
  type `T` with arguments `ilist`, `std::forward<Us>(us)` using
  default-constructed allocator `A{}`. ???


```c++
  template <class U = T, class I, class... Us>
  explicit constexpr polymorphic(allocator_arg_t, const Allocator& a,
                                 in_place_type_t<U>,
                                 initializer_list<I> ilist, Us&&... us)
```

19.4? _Constraints_: ?? `is_base_of_v<T, U>` is `true`.
  `is_constructible_v<U, Us...>` is `true`.
  `is_copy_constructible_v<U>` is `true`.
  `is_constructible_v<T, initializer_list<I>&, Us...>` is `true`.

19.5? _Mandates_: `T` is a complete type.

19.6? _Effects_: ??? Value direct-non-list-initializes an owned object of
  type `T` with arguments `ilist`, `std::forward<Us>(us)` using the
  specified allocator `a`. ???
  `alloc` is direct-non-list-initialized with `a`.


19.7? _Postconditions_: `*this` is not valueless.


#### X.Z.3 Assignment [polymorphic.assign]

//TODO:REVISE

```c++
template <class U>
constexpr polymorphic& operator=(U&& u);
```

0.0? _Mandates_: `T` is a complete type.

0.1? _Effects_: Move assigns from
  `polymorphic<T>(std::in_place_type_t<std::remove_cvref_t<U>>{}, u)`.

  No effects if an exception is thrown.

0.2? _Returns_: A reference to `*this`.


## Reference implementation

A C++20 reference implementation of the work discussed in this proposal is
available on GitHub at [https://www.github.com/jbcoe/valuei_types].

## Acknowledgements

## References

_`indirect` and `polymorphic`: Vocabulary Types for Composite Class Design_, J. B. Coe,
A. Peacock, and S. Parent, 2024
[https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3019r6.html]

_An allocator-aware optional type_, P. Halpern, N. D. Ranns, V. Voutilainen, 2024
[https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2047r7.html]

## Appendix A: complete class template specifications

### X.Y.1 Class template indirect synopsis [indirect.syn]

```c++
template <class T, class Allocator = allocator<T>>
class indirect {
 public:
  using value_type = T;
  using allocator_type = Allocator;
  using pointer = typename allocator_traits<Allocator>::pointer;
  using const_pointer = typename allocator_traits<Allocator>::const_pointer;

  constexpr indirect();

  explicit constexpr indirect(allocator_arg_t, const Allocator& a);

  //TODO:CHECK here
  template <class U>
  explicit constexpr indirect(U&& u);

  //TODO:CHECK here
  template <class U>
  explicit constexpr indirect(allocator_arg_t, const Allocator& a,
                              in_place_t, U&& u);

  template <class... Us>
  explicit constexpr indirect(in_place_t, Us&&... us);

  template <class... Us>
  explicit constexpr indirect(allocator_arg_t, const Allocator& a,
                              in_place_t, Us&&... us);

  //TODO:CHECK here
  template<class U, class... Us>
  explicit constexpr indirect(in_place_t, std::initializer_list<U> ilist,
                              Us&&... us);

  //TODO:CHECK here
  template<class U, class... Us>
  explicit constexpr indirect(allocator_arg_t, const Allocator& a,
                              in_place_t, std::initializer_list<U> ilist,
                              Us&&... us);

  constexpr indirect(const indirect& other);

  constexpr indirect(allocator_arg_t, const Allocator& a,
                     const indirect& other);

  constexpr indirect(indirect&& other) noexcept(see below);

  constexpr indirect(allocator_arg_t, const Allocator& a,
                     indirect&& other) noexcept(see below);

  constexpr ~indirect();

  //TODO:CHECK here
  template <class U>
  constexpr indirect& operator=(U&& u);

  constexpr indirect& operator=(const indirect& other);

  constexpr indirect& operator=(indirect&& other) noexcept(see below);

  constexpr const T& operator*() const & noexcept;

  constexpr T& operator*() & noexcept;

  constexpr const T&& operator*() const && noexcept;

  constexpr T&& operator*() && noexcept;

  constexpr const_pointer operator->() const noexcept;

  constexpr pointer operator->() noexcept;

  constexpr bool valueless_after_move() const noexcept;

  constexpr allocator_type get_allocator() const noexcept;

  constexpr void swap(indirect& other) noexcept(see below);

  friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept(see below);

  template <class U, class AA>
  friend constexpr bool operator==(
    const indirect& lhs, const indirect<U, AA>& rhs) noexcept(see below);

  template <class U>
  friend constexpr bool operator==(
    const indirect& lhs, const U& rhs) noexcept(see below);

  template <class U, class AA>
  friend constexpr auto operator<=>(
    const indirect& lhs, const indirect<U, AA>& rhs) noexcept(see below)
    -> compare_three_way_result_t<T, U>;

  template <class U>
  friend constexpr auto operator<=>(
    const indirect& lhs, const U& rhs) noexcept(see below)
    -> compare_three_way_result_t<T, U>;

private:
  pointer p; // exposition only
  Allocator alloc; // exposition only
};

template <typename Value>
indirect(Value) -> indirect<Value>;

//TODO:CHECK here
template <typename Value>
indirect(std::in_place_t, Value) -> indirect<Value>;

template <typename Alloc, typename Value>
indirect(std::allocator_arg_t, Alloc, Value) -> indirect<
    Value, typename std::allocator_traits<Alloc>::template rebind_alloc<Value>>;
```

### X.Y.1 Class template polymorphic synopsis [polymorphic.syn]

```c++
template <class T, class Allocator = allocator<T>>
class polymorphic {
  Allocator alloc; // exposition only
 public:
  using value_type = T;
  using allocator_type = Allocator;
  using pointer = typename allocator_traits<Allocator>::pointer;
  using const_pointer  = typename allocator_traits<Allocator>::const_pointer;

  explicit constexpr polymorphic();

  explicit constexpr polymorphic(allocator_arg_t, const Allocator& a);

  template <class U, class... Us>
  explicit constexpr polymorphic(in_place_type_t<U>, Us&&... us);

  template <class U, class... Us>
  explicit constexpr polymorphic(allocator_arg_t, const Allocator& a,
                                 in_place_type_t<U>, Us&&... us);

  //TODO:CHECK here
  template <class U>
  explicit constexpr polymorphic(U&& u);

  //TODO:CHECK here
  template <class U>
  explicit constexpr polymorphic(allocator_arg_t, const Allocator& a,
                                 in_place_type_t<U>, U&& u);

  constexpr polymorphic(const polymorphic& other);

  constexpr polymorphic(allocator_arg_t, const Allocator& a,
                        const polymorphic& other);

  //TODO:CHECK here
  template <class U, class I, class... Us>
  explicitconstexpr polymorphic(in_place_type_t<U>,
                                initializer_list<I> ilist, Us&&... us)

  //TODO:CHECK here
  template <class U, class I, class... Us>
  explicit constexpr polymorphic(allocator_arg_t, const Allocator& a,
                                 in_place_type_t<U>,
                                 initializer_list<I> ilist, Us&&... us)

  constexpr polymorphic(polymorphic&& other) noexcept(see below);

  constexpr polymorphic(allocator_arg_t, const Allocator& a,
                        polymorphic&& other) noexcept(see below);

  constexpr ~polymorphic();

  //TODO:CHECK here
  template <class U>
  constexpr polymorphic& operator=(U&& u);

  constexpr polymorphic& operator=(const polymorphic& other);

  constexpr polymorphic& operator=(polymorphic&& other) noexcept(see below);

  constexpr const T& operator*() const noexcept;

  constexpr T& operator*() noexcept;

  constexpr const_pointer operator->() const noexcept;

  constexpr pointer operator->() noexcept;

  constexpr bool valueless_after_move() const noexcept;

  constexpr allocator_type get_allocator() const noexcept;

  constexpr void swap(polymorphic& other) noexcept(see below);

  friend constexpr void swap(polymorphic& lhs,
                             polymorphic& rhs) noexcept(see below);
};
```

## Appendix B: design choices, alternatives and breaking changes

//TODO:UPDATE

The design components, design decisions, and the cost and impact of alternative
design choices for `indirect` and `polymorphc` are detailed in [Appendix C:P3019R6].
Whilst breaking changes to the design of `indirect` and `polymorphic` would be
possible after C++26 standardization, such changes are likely to negatively impact
users.

The relevant design components affected by the additions discussed in this proposal
are shown below.

| Component | Decision | Alternative | Change impact | Breaking change? |
|--|--|--|--|--|
|*Original*|--|--|--|--|
|Explicit constructors|Constructors are marked `explicit`|Non-explicit constructors|Conversion for single arguments or braced initializers becomes valid| No |
|*Updated*|--|--|--|--|
|Explicit constructors|Constructors are marked `explicit`|Non-explicit constructors|Conversion for single arguments or braced initializers becomes valid| No |
