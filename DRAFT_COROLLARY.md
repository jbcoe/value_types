# Converting constructors and assignment for `indirect` and `polymorphic`
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
 - [Constructors overview](#constructor-overview)
 - [Assignment overview](#assignment-overview)
 - [Technical specifictions](#technical-specifications)
 - [Reference implementation](#reference-implementation)
 - [Acknowledgements](#acknowledgements)
 - [References](#references)

<=============================================================================>

## Abstract
New vocabulary types `indirect` and `polymorphic` for composite class design are 
proposed in P3019R6 [REF]. This paper is an auxiliary proposal for the addition 
of converting constructors and assignment to the design of `indirect`
and `polymorphic`.

## Introduction

- Briefly describe `indirect` and `polymorphic` [REF]
- Briefly motivate need for converting construction and assignment

//TODO:REVISE better headings please
## Overview of additions ?

### Converting constructors 

### Converting assignment

## Technical specifications

### X.Y Class template indirect [indirect]

#### X.Y.1 Class template indirect synopsis [indirect.syn]

The addition of the above converting constructors and assignment for 
`indrect` gives the following updated class template for `indirect`.

//TODO:ADD new constructors and assignment
//TODO:CHECK does the whole whole class template require including here?

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

template <typename Alloc, typename Value>
indirect(std::allocator_arg_t, Alloc, Value) -> indirect<
    Value, typename std::allocator_traits<Alloc>::template rebind_alloc<Value>>;
```

### X.Y.2 Constructors [indirect.ctor]

//TODO:REVISE add converting constructors for indirect

```c++
explicit constexpr indirect(???);
```

N?. _Constraints_: ? `is_default_constructible_v<T>` is `true`.
  `is_copy_constructible_v<T>` is `true`.

N?. _Mandates_: ? `T` is a complete type.

N?. _Effects_: ...

N?. _Postconditions_: ...

N?. _Throws_: ...

### X.Y.3 Assignment [indirect.assign]

//TODO:REVISE add converting assignment for indirect

```c++
constexpr indirect& operator=(???);
```

//TODO:REVISE copied verbatim from P3019R6 - what's needed?

```c++
constexpr indirect& operator=(const indirect& other);
```

1. _Mandates_: `T` is a complete type.

2. _Effects_: ...

3. _Returns_: ...
 

### X.Y Class template polymorphic [polymorphic]

#### X.Y.1? Class template polymorphic synopsis [polymomrphic.syn]

The addition of the above converting constructors and assignment for 
`polymmorphic` gives the following updated class template for 
`polymorphic`.

//TODO:ADD new constructors and assignment
//TODO:CHECK does the whole whole class template require including here?

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

  constexpr polymorphic(const polymorphic& other);

  constexpr polymorphic(allocator_arg_t, const Allocator& a,
                        const polymorphic& other);

  constexpr polymorphic(polymorphic&& other) noexcept(see below);

  constexpr polymorphic(allocator_arg_t, const Allocator& a,
                        polymorphic&& other) noexcept(see below);

  constexpr ~polymorphic();

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

//TODO:REVISE add converting constructors for polymorphic

```c++
explicit constexpr polymorphic(???)
```

N?. _Constraints_: `is_default_constructible_v<T>` is `true`,
  `is_copy_constructible_v<T>` is `true`.
  `is_default_constructible_v<allocator_type>` is `true`.

N?. Mandates: `T` is a complete type.

N?. _Effects_: ...

N?. _Postconditions_: ...

N?. _Throws_: ...

#### X.Z.3 Assignment [polymorphic.ctor]

//TODO:REVISE add converting assignment for polymorphic

```c++
constexpr polymorphic& operator=(???);
```

1. _Mandates_: `T` is a complete type.

2. _Effects_: ...

3. _Returns_: ...


## Reference implementation

A C++20 reference implementation of the work discussed in this proposal is available on GitHub at [https://www.github.com/jbcoe/value_types].

## Acknowledgements

## References

_`indirect` and `polymorphic`: Vocabulary Types for Composite Class Design, J. B. Coe, A. Peacock, and S. Parent 2024
[https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3019r6.html]
