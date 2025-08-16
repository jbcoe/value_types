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

Modify `indirect` constructors as indicated:

Modify `polymorphic` constructors as indicated:
