# Converting construction and assignment for `indirect` and `polymorphic`
 <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />

ISO/IEC JTC1 SC22 WG21 Programming Language C++

D3512R0

Working Group: Library Evolution, Library

Date: 2024-03-18

_Jonathan Coe \<<jonathanbcoe@gmail.com>\>_

_Antony Peacock \<<ant.peacock@gmail.com>\>_

_Sean Parent \<<sparent@adobe.com>\>_

# Contents
 - [Abstract](#abstract) 
 - [Introduction](#introduction)
 - [Converting construction and assignment](#converting-construction-and-assignment)
 - [Technical specifictions](#technical-specifications)
 - [Reference implementation](#reference-implementation)
 - [Acknowledgements](#acknowledgements)
 - [References](#references)
 - [Appendix A](#appendix-a)

[//]: <> (<=============================================================================>)

## Abstract
New vocabulary types `indirect` and `polymorphic` for composite class design are 
proposed in P3019R6 [REF]. This corollary proposal details adding support 
for converting construction and assignment to `indirect` and `polymorphic`.

## Introduction

//TODO:ADD
- Briefly describe `indirect` and `polymorphic` [P3019]
- Briefly motivate need for converting construction and assignment

## Converting construction and assignment

### Constructors
After reciving feedback from LEWG, converting constructors were added to
support conversion from `T` to an `indirect<T>` or `polymorphic<T>`. Because
these operation allocate memory they are marked explicit so the intent to use
them is clear.

```c++
int i = 42;
indirect<int> i1 = r; // error
indirect<int> i2(r); // supported

polymorphic<Shape> s1 = r; // error
polymorphic<Shape> s2(r); // supported
```

### Assignment

//TODO:ADD

## Technical specifications

We update the technical specifications detailed in [P3019R6] to include 
converting construction and assignment.

### X.Y Class template indirect [indirect]

#### X.Y.1 Class template indirect synopsis [indirect.syn]

The updated class template `indirect` with support for 
converting construction and assignment added is as follows:

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

  template <class... Us>
  explicit constexpr indirect(in_place_t, Us&&... us);

  template <class... Us>
  explicit constexpr indirect(allocator_arg_t, const Allocator& a,
                     in_place_t, Us&&... us);

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

### X.Y.2 Constructors [indirect.ctor]

```c++
template <class U>
explicit constexpr indirect(U&& u);
```

10.1?. _Constraints_: `is_same_v<T, remove_cvref_t<U>>` is `true`.
   `is_copy_constructible_v<T>` is `true`.
   `is_default_constructible_v<allocator_type>` is `true`.
   `is_same_v<remove_cvref_t<U>, in_place_t>` is `false`.
10.2?. _Mandates_: `T` is a complete type.

10.3?. _Effects_: Equivalent to `indirect(allocator_arg_t{}, Allocator(), in_place_t{}
    std::forward<U>(u))`.
    

### X.Y.3 Assignment [indirect.assign]

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

#### X.Y.1? Class template polymorphic synopsis [polymorphic.syn]

The updated class template `polymorphic` with support for 
converting construction and assignment added is as follows:

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

  template <class U, class... Ts>
  explicit constexpr polymorphic(in_place_type_t<U>, Ts&&... ts);

  template <class U, class... Ts>
  explicit constexpr polymorphic(allocator_arg_t, const Allocator& a,
                        in_place_type_t<U>, Ts&&... ts);

  //TODO:CHECK here
  template <class U>
  explicit constexpr polymorphic(U&& u);

  constexpr polymorphic(const polymorphic& other);

  constexpr polymorphic(allocator_arg_t, const Allocator& a,
                        const polymorphic& other);

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

#### X.Z.2 Constructors [polymorphic.ctor]

```c++
template <class U>
explicit constexpr polymorphic(U&& u);
```

16.1? _Constraints_: `is_same_v<polymorphic, remove_cvref_t<U>>` is `false`.
   `is_copy_constructible_v<remove_cvref_t<U>>` is `true`.
   `is_base_of_v<T, std::remove_cvref_t<U>>` is `true`.

16.2? _Mandates_: `T` is a complete type.

16.3? _Effects_: Equivalent to `polymorphic(std::allocator_arg_t{}, A{},
  std::in_place_type_t<std::remove_cvref_t<U>>{}, std::forward<U>(u))`.


#### X.Z.3 Assignment [polymorphic.assign]

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
available on GitHub at [https://www.github.com/jbcoe/value_types].

## Acknowledgements

## References

_`indirect` and `polymorphic`: Vocabulary Types for Composite Class Design, J. B. Coe, 
A. Peacock, and S. Parent 2024
[https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3019r6.html]

## Appendix A: Design choices, alternatives and breaking changes

//TODO:UPDATE 
The table below shows the relevant updated rows of the table in [Appendix C/P3019R6]. 
The table shows the design components, the design decisions made, and the cost and 
impact of alternative design choices. As presented in paper [P3019R6], the design of 
class templates `indirect` and `polymorphic` has been approved by the LEWG. 
The authors have until C++26 is standardized to consider making any breaking changes; 
after C++26, whilst breaking changes will still be possible, the impact of these changes 
on users could be potentially significant and unwelcome.

| Component | Decision | Alternative | Change impact | Breaking change? |
|--|--|--|--|--|
|Previous|--|--|--|--|
|Explicit constructors|Constructors are marked `explicit`|Non-explicit constructors|Conversion for single arguments or braced initializers becomes valid| No |
|Updated|--|--|--|--|
|Explicit constructors|Constructors are marked `explicit`|Non-explicit constructors|Conversion for single arguments or braced initializers becomes valid| No |

