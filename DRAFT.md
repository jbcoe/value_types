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

### Deep copies

### `const` propagation

### Unobservable null state and interaction with `std::optional`

## Impact on the standard

This proposal is a pure library extension. It requires additions to be made to
the standard library header `<memory>`.

## Technical specifications

## Reference implementation

## Acknowledgements

The authors would like to thank Andrew Bennieston, Bengt Gustafsson, Casey
Carter, Daniel Krügler, David Krauss, Ed Catmur, Geoff Romer, Germán Diago,
Jonathan Wakely, Kilian Henneberger, LanguageLawyer, Louis Dionne, Maciej Bogus,
Malcolm Parsons, Matthew Calabrese, Nathan Myers, Nevin Liber, Nina Ranns,
Patrice Roy, Roger Orr, Stephan T Lavavej, Stephen Kelly, Thomas Koeppe, Thomas
Russell, Tom Hudson, Tomasz Kamiński, Tony van Eerd and Ville Voutilainen for
suggestions and useful discussion.

## References


[N3339], "A Preliminary Proposal for a Deep-Copying Smart Pointer", W.E.Brown, 2012 [http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2012/n3339.pdf]

Reference implementation on GitHub [https://github.com/jbcoe/value_types]


## Appendix A: Detailed design decisions

### Two class templates, not one

### Type erasure

### No observable null state

### Allocator support

### Copiers and deleters

### Pointer constructors

### Pointer-like helper functions

### Comparisons

### Hashing

### Implicit conversions
