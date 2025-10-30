# Against implicit conversions for `indirect`

<!-- markdownlint-disable MD029 -->

ISO/IEC JTC1 SC22 WG21 Programming Language C++

D3902R0

Working Group: Library Evolution

Date: 2025-10-29

_Jonathan Coe \<<jonathanbcoe@gmail.com>\>_

_Antony Peacock \<<ant.peacock@gmail.com>\>_

_Sean Parent \<<sparent@adobe.com>\>_

## Abstract

The national body comment US 77-140 says:

_indirect should convert to T& to simplify the use cases (e.g., returning the object from a function with a return type T&) where indirect appears as a drop-in replacement for T when T may be an incomplete type conditionally. With the proposed change, indirect is closer to reference_wrapper, but carries storage._

The authors of indirect are opposed to this change without significant implementation experience.

## Discussion

### Background

The class template `indirect` confers value-like semantics on a
dynamically-allocated object. An `indirect` may hold an object of a class `T`.
Copying the `indirect` will copy the object `T`. When an `indirect<T>` is
accessed through a const access path, constness will propagate to the owned
object.

`indirect<T>` can be implemented, like `reference_wrapper` as a class with a 
pointer member. 

When an instance of `indirect<T>` is used in move construction or move assignment,
the moved-from instance becomes valueless: the member pointer is `nullptr`. 

Early drafts of `indirect<T>` [1] had preconditions on all member functions, apart
from destruction and assignment, that `this` was not in a valueless state.
Equality and comparison required that neither the left-hand-side or right-had-side operand
was valueless. While the standard requires only that moved-from objects are in a _valid but unspecified state_, there was strong feeling from implementers that adding preconditions
so liberally left the undefined behaviour surface of `indirect` too large. In particular,
people were concerned that generic code may copy, move from and compare objects in a 
potentially valueless state. 

In the current working draft of the C++ standard, `indirect<T>` must not be in a valueless state
for `operator->` and `operator*` (const and non-const overloads). This is consistent with other
types with a potential null state like `unique_ptr` and `optional`.

### Requested changes

National Body Comment US 77-140 would require the addition of new member functions to `indirect`:

```c++
constexpr operator const T&() const & noexcept;
constexpr operator T&() & noexcept;
constexpr operator const T&&() const && noexcept;
constexpr operator T&&() && noexcept;
```

### Author's stance

The authors are opposed to the addition of implicit conversions to reference (and rvalue-reference)
as these conversions would have the precondition that `this` is not in a valueless state. 

Having striven to modify the design of indirect to avoid non-valueless preconditions, the authors are reluctant to
see opportuntities for undefined behaviour introduced at such a late stage without motivating implementation experience or a paper.

## Future direction

With compelling implemention experience, it would be possible to introduce implicit reference conversions
for `indirect` in a later version of the C++ Standard. 

There is nothing about the current design of `indirect` that blocks later introduction of implicit conversions.

## Acknowledgements

Many thanks to Neelofer Banglawala for useful input, review and discussion.

## References

[1] p3019r1: _Vocabulary Types for Composite Class Design_, \
J. B. Coe, A. Peacock, and S. Parent, 2024 \
<https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p3019r1.pdf>

