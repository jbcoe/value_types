# Vocabulary Types for Composite Class Design

ISO/IEC JTC1 SC22 WG21 Programming Language C++

DXXXXX.0

Working Group: Library Evolution, Library

Date: 2023-09-16

_Jonathan Coe \<<jonathanbcoe@gmail.com>\>_

_Antony Peacock \<<ant.peacock@gmail.com>\>_

_Sean Parent \<<sparent@adobe.com>\>_

## Abstract

We propose the addition of two new class-templates to the C++ Standard Library:
`indirect<T>` and `polymorphic<T>`.

These class-templates have value semantics and compose well with other standard
library types (such as vector) allowing the compiler to correctly generate
special member functions.

The class template, `indirect`, confers value-like semantics on a
free-store-allocated object. An `indirect` may hold an object of a class `T`,
copying the `indirect` will copy the object `T`. When a parent object contains a
member of type `indirect<T>` and is accessed through a const access path,
`const`ness will propagate from the parent object to the instance of `T` owned
by the `indirect` member.

The class template, `polymorphic`, confers value-like semantics on a free-store
allocated object.  A `polymorphic<T>` may hold an object of a class publicly
derived from `T`, copying the `polymorphic<T>` will copy the object of the
derived type. When a parent object contains a member of type `polymorphic<T>`
and is accessed through a const access path, `const`ness will propagate from the
parent object to the instance of `T` owned by the `polymorphic` member.

This proposal is a fusion of two older individual proposals, P1950 and P0201.
The design of the two class-templates is sufficiently similar that they should
not be considered in isolation.

## Motivation

C++ lets associated data and functions that act upon that data be grouped
together into a record type: a struct or a class. Classes (or structs, we stick
with classes for brevity) can contain instances of other classes as members.
This allows us to build up complex data structures from simple building blocks.

The standard library has many class template types that are useful in building
composite classes. Compiler generated special member functions work with these
standard library types to provide constructors, destructors, copy and move
without the need for user to write code.

The vocabulary of types useful for defining composite classes is not limited to,
but includes: `std::array`, `std::vector`, `std::map`, `std::unordered_map`,
`std::string`, `std::optional`, `std::variant`. All of these types are value
types: the data they own is copied when the type is copied; the data is
destroyed when the type is destroyed; the data is const when accessed through a
const-access-path.

TODO: Discuss limitations of existing types.

## Design requirements

We review the fundamental design requirements of `indirect` and `polymorphic`
that make them suitable for composite class design.

### Special member functions

Both class templates should be suitable for use as members of composite classes
where the compiler will generate special member functions. This means that the
class templates should provide the special member functions where they are
supported by the owned object type `T`.

* `indirect<T>` and `polymorphic<T>` are default constructible in cases where
  `T` is default constructible.

* `indirect<T>` is copy constructible where `T` is copy constructible and
  assignable.

* `polymorphic<T>` is unconditionally copy constructible and assignable.

* `indirect<T>` and `polymorphic<T>` are unconditionally move constructible and
  assignable.

### Deep copies

Copies of `indirect<T>` and `polymorphic<T>` should own copies of the owned
object. In the case of `polymorphic<T>` this means that the copy should own a
copy of a potentially derived type object.

### `const` propagation

When a parent object contains a member of type `indirect<T>` or
`polymorphic<T>`, access to the owned object (of type `T`) through a const
access path should be `const` qualified.

```c++
struct A {
    enum class Constness { CONST, NON_CONST };
    Constness foo() { return Constness::NON_CONST; }
    Constness foo() const { return Constness::CONST; };
};

class Composite {
    indirect<A> a_;
    
    Constness foo() { return a_.foo(); }
    Constness foo() const { return a_.foo(); };
};

int main() {
    Composite c;
    assert(c.foo() == A::Constness::NON_CONST);
    assert(c.bar() == A::Constness::NON_CONST);
    const Composite& cc = c;
}
```

### Value semantics

Both `indirect` and `polymorphic` are value-types whose owned object is
free-store-allocated (or some other memory-resource controlled by the specified
allocator).

When a value type is copied it gives rise to two independent objects that can be
modified separately.

The owned object is part of the logical state of `indirect` and `polymorphic`.
Operations on a const-qualified object do not make changes to the object's
logical state nor to the logical state of other object.

`indirect<T>` and `polymporphic<T>` are default constructible in cases where `T`
is default constructible. Moving a value type onto the free-store should not add
or remove the ability to be default constructed.

Pairwise-comparison operators, which are defined only for `indirect`, compare
the owned objects where the owned objects can be compared.

The hash operation, which is defined only for `indirect`, hashes the owned
object where the owned object can be hashed.

### Unobservable null state and interaction with `std::optional`

Both `indirect` and `polymorphic` have a null state which is used to implement
move. The null state is not intended to be observable to the user, there is no
`operator bool` or `has_value` member function. Accessing the value of a
`indirect` or `polymorphic` after it has been moved from is erroneous behaviour.
We provide a `valueless_after_move` member function to allow explicit checks for
the valueless state in cases where it cannot be verified statically.

Without a null state, moving `indirect` or `polymorphic` would require
allocation and moving from the owned object. This would be expensive and would
require the owned object to be moveable. The existance of a null state allows
move to be implemented cheaply without requiring the owned object to be
moveable.

## Prior work

This proposal is a continuation of the work started in [P0201] and [P1950].

There was previous work on a cloned pointer type [N3339] which met with
opposition becuse of the mixing of value and pointer semantics. We feel that the
pure value-semantics of `indirect` and `polymorphic` address these concerns.

## Impact on the standard

This proposal is a pure library extension. It requires additions to be made to
the standard library header `<memory>`.

## Technical specifications

### X.Y Class template indirect [indirect]

#### X.Y.1 Class template indirect general [indirect.general]

An _indirect value_ is an object that manages the lifetime of an owned object
using an allocator.  The owned object (if any) is copied or destroyed using the
specified allocator when the indirect value is copied or destroyed. An indirect
value object is _valueless_ if it has no owned object. A indirect value may only
become valueless after it has been moved from.

The template parameter `T` of `indirect<T>` must be a non-union class type.

The template parameter T of `indirect<T>` may be an incomplete type.

#### X.Z.1 Class template indirect synopsis [indirect.syn]

```c++
template <class T, class Allocator = std::allocator<T>>
class indirect {
  T* p_; // exposition only
  Allocator allocator_; // exposition only 
 public:
  using value_type = T;
  using allocator_type = A;

  indirect();

  template <class... Ts>
  indirect(std::in_place_t, Ts&&... ts);

  template <class... Ts>
  indirect(std::allocator_arg_t, const Allocator& alloc, std::in_place_t, Ts&&... ts);

  indirect(const indirect& other);

  indirect(const indirect& other, const Allocator& alloc);

  indirect(indirect&& other) noexcept;
  
  indirect(indirect&& other, const Allocator& alloc) noexcept;

  ~indirect();

  indirect& operator=(const indirect& other);

  indirect& operator=(indirect&& other) noexcept;

  constexpr const T& operator*() const noexcept;

  constexpr T& operator*() noexcept;

  constexpr const T* operator->() const noexcept;

  constexpr T* operator->() noexcept;

  constexpr bool valueless_after_move() const noexcept;

  constexpr void swap(indirect& other) noexcept;

  friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept;

  template <class U, class AA>
  friend constexpr bool operator==(const indirect<T, A>& lhs, const indirect<U, AA>& rhs);

  template <class U, class AA>
  friend constexpr bool operator!=(const indirect<T, A>& lhs, const indirect<U, AA>& rhs);

  template <class U, class AA>
  friend constexpr auto operator<=>(const indirect<T, A>& lhs, const indirect<U, AA>& rhs);

  template <class U>
  friend constexpr bool operator==(const indirect<T, A>& lhs, const U& rhs);

  template <class U>
  friend constexpr bool operator==(const U& lhs, const indirect<T, A>& rhs);

  template <class U>
  friend constexpr bool operator!=(const indirect<T, A>& lhs, const U& rhs);

  template <class U>
  friend constexpr bool operator!=(const U& lhs, const indirect<T, A>& rhs);

  template <class U>
  friend constexpr auto operator<=>(const indirect<T, A>& lhs, const U& rhs);

  template <class U>
  friend constexpr auto operator<=>(const U& lhs, const indirect<T, A>& rhs);
};

template <class T, class Alloc>
struct std::uses_allocator<indirect<T>, Alloc> : true_type {};
```

#### Constructors [indirect.ctor]

```c++
indirect()
```

* _Constraints_: `is_default_constructible_v<T>` is true.

* _Effects_: Constructs an indirect owning a default constructed `T` created
using the specified allocator.

* _Postconditions_: `*this` is not valueless.

```c++
template <class... Ts>
indirect(std::in_place_t, Ts&&... ts);
```

* _Constraints_: `is_constructible_v<T, Ts...>` is true.

* _Effects_: Constructs an indirect owning an instance of `T` created with the
arguments `Ts` using the specified allocator.

* _Postconditions_: `*this` is not valueless.

```c++
template <class... Ts>
indirect(std::allocator_arg_t, const Allocator& alloc, std::in_place_t, Ts&&... ts);
```

* _Constraints_: `is_constructible_v<T, Ts...>` is true.

* _Preconditions_: `Allocator` meets the _Cpp17Allocator_ requirements.

* _Effects_: Constructs an indirect owning an instance of `T` created with the
arguments `ts...` using the specified allocator.

* _Postconditions_: `*this` is not valueless.

```c++
indirect(const indirect& other);
```

* _Constraints_: `is_copy_constructible_v<T>` is true.

* _Preconditions_: `other` is not valueless.

* _Effects_: Constructs an indirect owning an instance of `T` created with the
copy constructor of the object owned by `other` using the specified allocator.

* _Postconditions_: `*this` is not valueless.

```c++
indirect(const indirect& other, const Allocator& alloc);
```

* _Constraints_: `is_copy_constructible_v<T>` is true and 
  uses_allocator<T, Allocator> is true;.

* _Preconditions_: `other` is not valueless and `Allocator` meets the
  _Cpp17Allocator_ requirements.

* _Effects_: Equivalent to the preceding constructors except that the allocator is
  initialized with alloc.

* _Postconditions_: `*this` is not valueless.

```c++
indirect(indirect&& other) noexcept;
```

* _Preconditions_: `other` is not valueless.

* _Effects_: Constructs an indirect owning the object owned by `other`.

* _Postconditions_: `other` is valueless.

* _Remarks_: This constructor does not require that `is_move_constructible<T>_v`
  is true.

```c++
indirect(indirect&& other, const Allocator& alloc) noexcept;
```

* _Constraints_: `is_copy_constructible_v<T>` is true and 
  uses_allocator<T, Allocator> is true;.

* _Preconditions_: `other` is not valueless and `Allocator` meets the
  _Cpp17Allocator_ requirements.

* _Effects_: Equivalent to the preceding constructors except that the allocator is
  initialized with alloc.

* _Postconditions_: `other` is valueless.

* _Remarks_: This constructor does not require that `is_move_constructible<T>_v`
  is true.

#### Destructor [indirect.dtor]

```c++
~indirect();
```

* _Effects_: If `*this` is not valueless, destroys the owned object with the
specified allocator.

#### Assignment [indirect.assign]

```c++
indirect& operator=(const indirect& other) noexcept;
```

* _Preconditions_: `other` is not valueless.

* _Effects_: If `*this` is not valueless, destroys the owned object with the
specified allocator. Then, constructs an owned object using the copy constructor
of the object owned by `other` using the specified allocator.

* _Postconditions_: `*this` is not valueless.

```c++
indirect& operator=(indirect&& other) noexcept;
```

* _Preconditions_: `other` is not valueless.

* _Effects_: If `*this` is not valueless, destroys the owned object with the
  specified allocator. Then takes ownership of the object owned by `other`.

* _Postconditions_: `*this` is not valueless. `other` is valueless.

#### Observers [indirect.observers]

```c++
constexpr const T& operator*() const noexcept;
constexpr T& operator*() noexcept;
```

* _Preconditions_: `*this` is not valueless.

* _Effects_: Returns a reference to the owned object.

* _Remarks_: These functions are constexpr functions.

```c++
constexpr const T* operator->() const noexcept;
constexpr T* operator->() noexcept;
```

* _Preconditions_: `*this` is not valueless.

* _Effects_: Returns a pointer to the owned object.

* _Remarks_: These functions are constexpr functions.

```c++
constexpr bool valueless_after_move() const noexcept;
```

* _Returns_: `true` if `*this` is valueless, otherwise `false`.

#### Swap [indirect.swap]

```c++
constexpr void swap(indirect& other) noexcept;
```

* _Preconditions_: `*this` is not valueless, `other` is not valueless.

* _Effects_: Swaps the objects owned by `*this` and `other` by swapping
  pointers.

* _Remarks_: Does not call `swap` on the owned objects directly.

```c++
constexpr void swap(indirect& lhs, indirect& rhs) noexcept;
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: Swaps the objects owned by `lhs` and `rhs` by swapping pointers

* _Remarks_: Does not call `swap` on the owned objects directly.

#### Relational operators [indirect.rel]

```c++
template <class U, class AA>
constexpr bool operator==(const indirect<T, A>& lhs, const indirect<U, AA>& rhs);
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: returns  `*lhs == *rhs`.

* _Remarks_: Specializations of this function template for which `*lhs == *rhs`
  is a core constant expression are constexpr functions.

```c++
template <class U, class AA>
constexpr bool operator!=(const indirect<T, A>& lhs, const indirect<U, AA>& rhs);
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: returns  `*lhs != *rhs`.

* _Remarks_: Specializations of this function template for which `*lhs != *rhs`
  is a core constant expression are constexpr functions.

```c++
template <class U, class AA>
constexpr auto operator<=>(const indirect<T, A>& lhs, const indirect<U, AA>& rhs);
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: returns  `*lhs <=> *rhs`.

* _Remarks_: Specializations of this function template for which `*lhs <=> *rhs`
  is a core constant expression are constexpr functions.

#### Comparison with T [indirect.comp.with.t]

```c++
template <class T, class A, class U>
constexpr bool operator==(const indirect<T, A>& lhs, const U& rhs);
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: returns  `*lhs == *rhs`.

* _Remarks_: Specializations of this function template for which `*lhs == *rhs`
  is a core constant expression are constexpr functions.

```c++
template <class T, class A, class U>
constexpr bool operator==(const U& lhs, const indirect<T, A>& rhs);
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: returns  `*lhs == *rhs`.

* _Remarks_: Specializations of this function template for which `*lhs == *rhs`
  is a core constant expression are constexpr functions.

```c++
template <class T, class A, class U>
constexpr bool operator!=(const indirect<T, A>& lhs, const U& rhs)
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: returns  `*lhs != *rhs`.

* _Remarks_: Specializations of this function template for which `*lhs != *rhs`
  is a core constant expression are constexpr functions.

```c++
template <class T, class A, class U>
constexpr bool operator!=(const U& lhs, const indirect<T, A>& rhs);
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: returns  `*lhs != *rhs`.

* _Remarks_: Specializations of this function template for which `*lhs != *rhs`
  is a core constant expression are constexpr functions.

```c++
template <class T, class A, class U>
constexpr auto operator<=>(const indirect<T, A>& lhs, const U& rhs);
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: returns  `*lhs <=> *rhs`.

* _Remarks_: Specializations of this function template for which `*lhs <=> *rhs`
  is a core constant expression are constexpr functions.

```c++
template <class T, class A, class U>
constexpr auto operator<=>(const U& lhs, const indirect<T, A>& rhs);
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: returns  `*lhs <=> *rhs`.

* _Remarks_: Specializations of this function template for which `*lhs <=> *rhs`
  is a core constant expression are constexpr functions.

```c++
template <class T, class Alloc>
struct std::uses_allocator<xyz::polymorphic<T>, Alloc> : true_type {};
```

#### Allocator related traits

TODO: Copied from https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2047r3.html but I'm unsure why this recusively inherits from its self?

```c++
template<class T, class Allocator>
struct uses_allocator<T, Allocator> : uses_allocator<T, Allocator> { };
```

### X.Z Class template polymorphic [polymorphic]

#### X.Z.1 Class template polymorphic general [polymorphic.general]

A _polymorphic value_ is an object that manages the lifetime of an owned object
using an allocator. A polymorphic value object may own objects of different
types at different points in its lifetime. The owned object (if any) is copied
or destroyed using the specified allocator when the polymorphic value is copied
or destroyed. A polymorphic value object is _valueless_ if it has no owned
object. A polymorphic value may only become valueless after it has been moved
from.

The template parameter `T` of `polymorphic<T>` must be a non-union class type.

The template parameter `T` of `polymorphic<T>` may be an incomplete type.

#### X.Z.1 Class template polymorphic synopsis [polymorphic.syn]

```c++
template <class T, class Allocator = std::allocator<T>>
class polymorphic {
  control_block* control_block_; // exposition only
  Allocator allocator_; // exposition only  
 public:
  using value_type = T;
  using allocator_type = Allocator;

  polymorphic();

  template <class U, class... Ts>
  polymorphic(std::in_place_type_t<U>, Ts&&... ts);

  template <class U, class... Ts>
  polymorphic(std::allocator_arg_t, const Allocator& alloc, std::in_place_type_t<U>, Ts&&... ts);

  polymorphic(const polymorphic& other);
  
  polymorphic(const polymorphic& other, const Allocator& alloc);

  polymorphic(polymorphic&& other) noexcept;
  
  polymorphic(polymorphic&& other, const Allocator& alloc) noexcept;

  ~polymorphic();

  polymorphic& operator=(const polymorphic& other);

  polymorphic& operator=(polymorphic&& other) noexcept;

  constexpr const T& operator*() const noexcept;

  constexpr T& operator*() noexcept;

  constexpr const T* operator->() const noexcept;

  constexpr T* operator->() noexcept;

  constexpr bool valueless_after_move() const noexcept;

  constexpr void swap(polymorphic& other) noexcept;

  friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept;
};

template <class T, class Alloc>
struct std::uses_allocator<polymorphic<T>, Alloc> : true_type {};
```

#### Constructors [polymorphic.ctor]

```c++
polymorphic()
```

* _Constraints_: `is_default_constructible_v<T>` is true,
  `is_copy_constructible_v<T>` is true.

* _Effects_: Constructs an polymorphic owning a default constructed `T` created
using the specified allocator.

* _Postconditions_: `*this` is not valueless.

```c++
template <class U, class... Ts>
polymorphic(std::in_place_type_t<U>, Ts&&... ts);
```

* _Constraints_: `is_same_v<T, U> || is_base_of_v<T, U>` is true,
  `is_constructible_v<U, Ts...>` is true, `is_copy_constructible_v<U>` is true.

* _Effects_: Constructs an polymorphic owning an instance of `U` created with
the arguments `Ts` using the specified allocator.

* _Postconditions_: `*this` is not valueless.

```c++
template <class U, class... Ts>
polymorphic(std::allocator_arg_t, const Allocator& alloc, std::in_place_type_t<U>, Ts&&... ts);
```

* _Constraints_: `is_same_v<T, U> || is_base_of_v<T, U>` is true,
  `is_constructible_v<U, Ts...>` is true, `is_copy_constructible_v<U>` is true.

* _Preconditions_: `Allocator` meets the _Cpp17Allocator_ requirements.

* _Effects_: Constructs an polymorphic owning an instance of `U` created with
the arguments `ts...` using the specified allocator.

* _Postconditions_: `*this` is not valueless.

```c++
polymorphic(const polymorphic& other);
```

* _Preconditions_: `other` is not valueless.

* _Effects_: Constructs an polymorphic owning an instance of `T` created with
the copy constructor of the object owned by `other` using the specified
allocator.

* _Postconditions_: `*this` is not valueless.

```c++
polymorphic(const polymorphic& other, const Allocator& alloc);
```

* _Preconditions_: `other` is not valueless and `Allocator` meets the
  _Cpp17Allocator_ requirements.

* _Effects_: Constructs an polymorphic owning an instance of `T` created with
the copy constructor of the object owned by `other` using the specified
allocator.

* _Postconditions_: `*this` is not valueless.

```c++
polymorphic(polymorphic&& other) noexcept;
```

* _Preconditions_: `other` is not valueless.

* _Effects_: Constructs a polymorphic that takes ownership of the object owned
  by `other`.

* _Postconditions_: `other` is valueless.

* _Remarks_: This constructor does not require that `is_move_constructible<T>_v`
  is true.

```c++
polymorphic(polymorphic&& other, const Allocator& alloc) noexcept;
```

* _Preconditions_: `other` is not valueless and `Allocator` meets the
  _Cpp17Allocator_ requirements.

* _Effects_: Constructs a polymorphic that takes ownership of the object owned
  by `other`.

* _Postconditions_: `other` is valueless.

* _Remarks_: This constructor does not require that `is_move_constructible<T>_v`
  is true.

#### Destructor [polymorphic.dtor]

```c++
~polymorphic();
```

* _Effects_: If `*this` is not valueless, destroys the owned object with the
specified allocator.

#### Assignment [polymorphic.assign]

```c++
polymorphic& operator=(const polymorphic& other) noexcept;
```

* _Preconditions_: `other` is not valueless.

* _Effects_: If `*this` is not valueless, destroys the owned object with the
specified allocator. Then, constructs an owned object using the (possibly
derived-type) copy constructor of the object owned by `other` using the
specified allocator.

* _Postconditions_: `*this` is not valueless.

```c++
polymorphic& operator=(polymorphic&& other) noexcept;
```

* _Preconditions_: `other` is not valueless.

* _Effects_: If `*this` is not valueless, destroys the owned object with the
  specified allocator. Then takes ownership of the object owned by `other`.

* _Postconditions_: `*this` is not valueless. `other` is valueless.

#### Observers [polymorphic.observers]

```c++
constexpr const T& operator*() const noexcept;
constexpr T& operator*() noexcept;
```

* _Preconditions_: `*this` is not valueless.

* _Effects_: Returns a reference to the owned object.

* _Remarks_: These functions are constexpr functions.

```c++
constexpr const T* operator->() const noexcept;
constexpr T* operator->() noexcept;
```

* _Preconditions_: `*this` is not valueless.

* _Effects_: Returns a pointer to the owned object.

* _Remarks_: These functions are constexpr functions.

```c++
constexpr bool valueless_after_move() const noexcept;
```

* _Returns_: `true` if `*this` is valueless, otherwise `false`.

#### Swap [polymorphic.swap]

```c++
constexpr void swap(polymorphic& other) noexcept;
```

* _Preconditions_: `*this` is not valueless, `other` is not valueless.

* _Effects_: Swaps the objects owned by `*this` and `other` by swapping
  pointers.

* _Remarks_: Does not call `swap` on the owned objects directly.

```c++
friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept;
```

* _Preconditions_: `lhs` is not valueless, `rhs` is not valueless.

* _Effects_: Swaps the objects owned by `lhs` and `rhs` by swapping pointers

* _Remarks_: Does not call `swap` on the owned objects directly.


#### Allocator related traits

TODO: Copied from https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2047r3.html but I'm unsure why this recusively inherits from its self?

```c++
template<class T, class Allocator>
struct uses_allocator<T, Allocator> : uses_allocator<T, Allocator> { };
```

## Reference implementation

A C++20 reference implementation of this proposal is available on GitHub at
[https://www.github.com/jbcoe/value_types]

## Acknowledgements

The authors would like to thank Andrew Bennieston, Bengt Gustafsson, Casey
Carter, Daniel Krügler, David Krauss, Ed Catmur, Geoff Romer, Germán Diago,
Jonathan Wakely, Kilian Henneberger, LanguageLawyer, Louis Dionne, Maciej Bogus,
Malcolm Parsons, Matthew Calabrese, Nathan Myers, Nevin Liber, Nina Ranns,
Patrice Roy, Roger Orr, Stephan T Lavavej, Stephen Kelly, Thomas Koeppe, Thomas
Russell, Tom Hudson, Tomasz Kamiński, Tony van Eerd and Ville Voutilainen for
suggestions and useful discussion.

## References

"_A Preliminary Proposal for a Deep-Copying Smart Pointer_", W.E.Brown, 2012
[http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3339.pdf]

_A polymorphic value-type for C++_, J.B.Coe, S.Parent 2019
[https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0201r5.html]

_A Free-Store-Allocated Value Type for C++_, J.B.Coe, A.Peacock 2022
[https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1950r2.html]

A C++20 reference implementation is available on GitHub
[https://github.com/jbcoe/value_types]

## Appendix A: Detailed design decisions

We discuss some of the decisions that were made in the design of `indirect` and
`polymorphic`. Where there are multiple options we discuss the advantages and
disadvantages of each.

### Two class templates, not one

It's conceivable that a single class template could be used as a vocabulary type
for an indirect value-type supporting polymorphsim. Implementing this would
impose efficiency costs on the copy constructor in the case where the owned
object is the same type as the template type. When the owned object is a derived
type, the copy constructor uses type erasure to perform dynamic dispatch and
call the derived type copy constructor. The overhead of indirection and a
virtual function call is not tolerable where the owned object type and template
type match.

One potential solution would be to use a `std::variant` to store the owned type
or the control block used to manage the owned type. This would allow the copy
constructor to be implemented efficiently in the case where the owned type and
template type match. This would increase the object size beyond that of a single
pointer as the discriminant would need to be stored.

In the name of minimal size and efficency we opted to use two class templates.

### No observable null state

As an implementation detail, a null state is powerful as it allows move and swap
to be cheaply implemented without requiring memory allocation or for the owned
object to be moveable or swappable.

In designing composite classes, `indirect` and `polymorphic` will be used in
place of pointers which do permit a null state.

We decided that `indirect` and `polymorphic` need a null state for
implementation but that this should not be observable to the user.

### Copiers, deleters, pointer constructors and allocator support

Both `indirect_value` and `polymorphic_value` have constructors that take a
pointer along with a copier and deleter. The copier and deleter can be used to
specify how the object should be copied and deleted. The existence of a pointer
constructor introduces significant sharp-edges into the design of
`polymorphic_value` allowing the possibiliy o object slicing on copy when the
dynamic and static types of a derived-type pointer do not match.

We decied to remove the copier, deleter and pointer constructor in favour of
adding allocator support. Composite class design with `indirect` and
`polymorphic` does not need a pointer constructor and adding excluding a
pointer-constructor now does not prevent us from adding one in a later revision
of the standard. Allocator support, we were advised, needs to be there from the
beginning and cannot be added retrospectively. As `indirect` and `polymorphic`
are intended to be used alongside other C++ standrd library types like
`std::map` and `std::vector` it is important that they have allocator support in
contexts where allocators are used.

### Pointer-like helper functions

Earlier revisions of `polymorphic_value` (when it was `cloned_ptr`) had helper
functions to get access to the underlying pointer. These were removed under the
advice of the Library Evolution Working Group as they were not core to the
design of the class template nor were they consistent with value-type semantics.

Pointer-like accessors like `dynamic_pointer_cast` and `static_pointer_cast`
which are provided for `std::shared_ptr` could be added in a later revision of
the standard if required.

### Comparisons and hashing

We supported comparisons and hashing for `indirect` but not `polymorphic`.

In the case where the owned object `T` is hashable or comparable, `indirect<T>`
is hashable or comparable too by forwarding the hash or comparison to the owned
object.

Comparing and hashing polymorphic types is not a uniquely solved problem, though
it could well be implemented by adding suitable member functions to the base
class. Rather than impose the signatures of these member functions upon users of
`polymorphic` we decided to leave hashing and comparsion unsupported but
implementable by users.

### Implicit conversions

We decided that there should be no implicit conversion of a value `T` to an
`indirect<T>` or `polymorphic<T>`. An implicit conversion would require use of
the free-store and of memory allocation which is best made explicit by the user.

```c++
Rectangle r(w, h);
polymorphic<Shape> s = r; // error
```

To hoist a value into an `indirect` or `polymorphic` the user must use the
appropriate constructor.

```c++
Rectangle r(w, h);
polymorphic<Shape> s(std::in_place_type<Rectangle>, r); // Uses in-place constructor.
assert(dynamic_cast<Rectangle*>(&*s) != nullptr);
```

### Explicit conversions

The older class template `polymorphic_value` had explicit conversions allowing
construction of a `polymorphic_value<T>` from a `polymorphic_value<U>` where `T`
was a base class of `U`.

```c++
polymorphic_value<Quadrilateral> q(std::in_place_type<Rectangle>, w, h);
polymorphic_value<Shape> s = q; // uses explicit converting constructor.
assert(dynamic_cast<Rectangle*>(&*s) != nullptr);
```

The following code cannot be written with `polymorphic` as it does not allow
conversions between derived types:

```c++
polymorphic<Quadrilateral> q(std::in_place_type<Rectangle>, w, h);
polymorphic<Shape> s = q; // error
```

This is a deliberate design decision. `polymorphic` is intended to be used for
ownership of member data in composite classes where compiler-generated special
member functions will be used.

There is no motivating use case for explicit conversion between derived types
outside of tests.

### Small object optimisation for `polymorphic`

`polymorphic` could be designed to make use of a small object optimisation. A
small object optimisation uses a small buffer to potentially store the owned
object. This wpould make move construction more complicated as the owned object
must be moved from one buffer to another potentially invoking allocations if the
owned object's move constructor allocates memory.

As designed, `polymorphic<T>` does not require that `T` (or constructed classes
of type `U` derived from `T`) are move constructible or move assignable for
`polymorphic<T>` to be move constructible or move assignable.

Memory indirection is the point of `indirect`. An `indirect` with a small buffer
optimisation would be better implmented as just a `T`. `polymorphic` is intended
to be an `indirect` that supports polymorphism.

Memory indirection is part of the design of `polymorphic`; a value type with a
small buffer optimisation that did not allocate a control block for the owned
object would need be a different type.
