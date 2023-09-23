# Value types for composite class design

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

TODO(jbcoe): Composite class design with and without polymorphism.

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

```cpp
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

### Unobservable null state and interaction with `std::optional`

## Prior work

## Impact on the standard

This proposal is a pure library extension. It requires additions to be made to
the standard library header `<memory>`.

## Technical specifications

### X.Y Class template indirect [indirect]

#### X.Y.1 Class template indirect general [indirect.general]

An _indirect value_ is an object that manages the lifetime of an owned object
using an allocator.  The owned object (if any) is copied or destroyed using the
specified allocator when the indirect value is copied or destroyed. An
indirect value object is _valueless_ if it has no owned object. A indirect value
may only become valueless after it has been moved from.

The template parameter `T` of `indirect<T>` must be a non-union class type.

The template parameter T of `indirect<T>` may be an incomplete type.

#### X.Z.1 Class template indirect synopsis [indirect.syn]

```cpp
template <class T, class A = std::allocator<T>>
class indirect {
 public:
  using value_type = T;
  using allocator_type = A;

  indirect();

  template <class... Ts>
  indirect(std::in_place_t, Ts&&... ts);

  template <class... Ts>
  indirect(std::allocator_arg_t, const A& alloc, std::in_place_t, Ts&&... ts);

  indirect(const indirect& other);

  indirect(indirect&& other) noexcept;

  ~indirect();

  indirect& operator=(const indirect& other);

  indirect& operator=(indirect&& other) noexcept;

  constexpr const T& operator*() const noexcept;

  constexpr T& operator*() noexcept;

  constexpr const T* operator->() const noexcept;

  constexpr T* operator->() noexcept;

  constexpr void swap(indirect& other) noexcept;

  friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept;

  constexpr bool valueless_after_move() const noexcept { return p_ == nullptr; }
};
```

#### Constructors [indirect.ctor]

#### Destructor [indirect.dtor]

#### Assignment [indirect.assign]

#### Observers [indirect.observers]

#### Swap [indirect.swap]

### X.Z Class template polymorphic [polymorphic]

#### X.Z.1 Class template polymorphic general [polymorphic.general]

A _polymorphic value_ is an object that manages the lifetime of an owned object
using an allocator. A polymorphic value object may own objects of different
types at different points in its lifetime. The owned object (if any) is copied or
destroyed using the specified allocator when the polymorphic value is copied or
destroyed. A polymorphic value object is _valueless_ if it has no owned object.
A polymorphic value may only become valueless after it has been moved from.

The template parameter `T` of `polymorphic<T>` must be a non-union class type.

The template parameter `T` of `polymorphic<T>` may be an incomplete type.

#### X.Z.1 Class template polymorphic synopsis [polymorphic.syn]

```cpp
template <class T, class A = std::allocator<T>>
class polymorphic {
 public:
  using value_type = T;
  using allocator_type = A;

  polymorphic();

  template <class U, class... Ts>
  polymorphic(std::in_place_type_t<U>, Ts&&... ts);

  template <class U, class... Ts>
  polymorphic(std::allocator_arg_t, const A& alloc, std::in_place_type_t<U>, Ts&&... ts);

  polymorphic(const polymorphic& other);

  polymorphic(polymorphic&& other) noexcept;

  ~polymorphic();

  polymorphic& operator=(const polymorphic& other);

  polymorphic& operator=(polymorphic&& other) noexcept;

  constexpr const T& operator*() const noexcept;

  constexpr T& operator*() noexcept;

  constexpr const T* operator->() const noexcept;

  constexpr T* operator->() noexcept;

  constexpr void swap(polymorphic& other) noexcept;

  friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept;

  constexpr bool valueless_after_move() const noexcept { return p_ == nullptr; }
};
```

#### Constructors [polymorphic.ctor]

#### Destructor [polymorphic.dtor]

#### Assignment [polymorphic.assign]

#### Observers [polymorphic.observers]

#### Swap [polymorphic.swap]

## Reference implementation

A reference implementation of this proposal is available on GitHub at
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

"_A Preliminary Proposal for a Deep-Copying Smart Pointer_", W.E.Brown,
2012 [http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3339.pdf]

_A polymorphic value-type for C++_, J.B.Coe, S.Parent 2019
[https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0201r5.html]

_A Free-Store-Allocated Value Type for C++_, J.B.Coe, A.Peacock
2022 [https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1950r2.html]

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

```cpp
Rectangle r(w, h);
polymorphic<Shape> s = r; // error
```

To hoist a value into an `indirect` or `polymorphic` the user must use the
appropriate constructor.

```cpp
Rectangle r(w, h);
polymorphic<Shape> s(std::in_place_type<Rectangle>, r); // Uses in-place constructor.
assert(dynamic_cast<Rectangle*>(&*s) != nullptr);
```

### Explicit conversions

The older class template `polymorphic_value` had explicit conversions allowing
construction of a `polymorphic_value<T>` from a `polymorphic_value<U>` where `T`
was a base class of `U`.

```cpp
polymorphic_value<Quadrilateral> q(std::in_place_type<Rectangle>, w, h);
polymorphic_value<Shape> s = q; // uses explicit converting constructor.
assert(dynamic_cast<Rectangle*>(&*s) != nullptr);
```

The following code cannot be written with `polymorphic` as it does not allow
conversions between derived types:

```cpp
polymorphic<Quadrilateral> q(std::in_place_type<Rectangle>, w, h);
polymorphic<Shape> s = q; // error
```

This is a deliberate design decision. `polymorphic` is intended to be used for
ownership of member data in composite classes where compiler-generated special
member functions will be used.

There is no motivating use case for explicit conversion between derived types
outside of tests.

# Small object optimisation for `polymorphic`
