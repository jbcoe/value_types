---
marp: true
theme: default
paginate: true
size: 16:9
footer: https://github.com/jbcoe/value_types
---

# Against Implicit Conversion to Reference for Indirect

## A response to US national body comment: 77-140

### 2025-11-02

---

## NB comment 77-140

US national body comment 77-140 proposes:

`indirect` should convert to `T&` to simplify the use cases (e.g., returning the object from a function with a return type `T&`) where indirect appears as a drop-in replacement for `T` when `T` may be an incomplete type conditionally. With the proposed change, indirect is closer to reference_wrapper, but carries storage.

---

## Proposed change

Add (constexpr and noexcept)

```c++
operator const T&() const &
operator T&() &
operator const T&&() const &&
operator T&&() &&
```

The authors of `indirect` are opposed to this change without significant usage experience and a paper.

---

## A valueless state and undefined behaviour

`indirect<T>` can get into a valueless state after being moved from, in such a state the implicit conversion to `T&` would invoke undefined behaviour.

`indirect<T>` has preconditions on `operator*` and `operator->` but these must be explicitly invoked. Preconditions on an implicit conversion are not something that we have experience with.

Allowing undefined behaviour on an implicit conversion is at odds with the design direction we've taken from the Library Evolution Working Group which requested that the valueless state was explicitly handled by `operator==`, `operator<=>` and `hash`. There is no possible handling of a valueless state in implicit conversion to a reference.

---

## Consistency with existing standard library types

`reference_wrapper<T>` does not own the associated instance of `T`, has an implicit conversion to `T&` but no null state.

`optional<T>` and `unique_ptr<T>` own the associated instances of `T`, have a null state and no implicit conversion to `T&`.

`indirect<T>` owns the associated instance of `T`, has a valueless state and, currently, no implicit conversion to `T&`.

---

## Static analysis and code review

Where an instance of `indirect<T>` `i` is used in a context where a reference is needed, it can be explicitly converted to a reference in the current draft by using `operator *`. Uses of `operator *` can be checked where required by coding standards whereas implicit conversions in generic code may elude code review or static analysis.

---

## Future extensions

If usage experience, compelling argument and motivation for implicit conversion arise, the C++ standards committee can add implicit conversions to `indirect<T>` in a later version of the C++ standard.

A national body comment, on its own, is not ideal format for supplementary design work. We invite a paper and further work but are opposed to the proposed design direction.
