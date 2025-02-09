---
marp: true
theme: default
paginate: true
size: 16:9
footer: https://github.com/jbcoe/value_types
---

# `indirect` and `polymorphic`: value types for composite class design

## ISO/IEC JTC1 SC22 WG21 Programming Language C++

## Changes from P3019r11
(Accepted in Plenary in Wroclaw 2024)

## Hagenberg Austria 2025-02-10

## https://github.com/jbcoe/value_types/compare/r11...main


---

# List of changes from 3019r11

- [LWG] Add missing `explicit` to `indirect` synoposis.

- [LEWG] Use _mandates_ not _constraints_ where the constraint is purely
  on the (potentially incomplete) type `T`.

- [LEWG] Remove _mandates_ that `T` is a complete type where it is implied by constraints.

- [LEWG] Add _mandates_ that `T` is copy-construcible in `indirect`'s copy constructors and assignments.

- [LEWG] Allow `indirect` to be used with non-copyable types.

- [LWG] Consistently use `synth-three-way-result` for `indirect`.

- [LEWG] Use mandates for constraints on comparison, in line with `optional`: https://eel.is/c++draft/optional#relops

- [None] Punctuation and style consistency fixes.

- [LWG] Use "may be only X" instead of "may only be X".

- [LEWG] Remove conditional `noexcept` on `indirect`'s three-way comparison.

- [None] Add additional example to clarify tagged constructor usage.

- [LWG] Fix typo where `alloc` was used when it should have been `a`.

- [LWG] Remove redundant constraint on `U` in `polymorphic`'s in-place constructors.

- [LWG] Consistently sort constraints.

- [None] Add detailed discussion of constraints and incomplete types.

---

https://github.com/jbcoe/value_types/compare/r11...main

---
