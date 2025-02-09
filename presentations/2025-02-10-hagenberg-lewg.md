# `indirect` and `polymorphic`: value types for composite class design

## ISO/IEC JTC1 SC22 WG21 Programming Language C++

## Changes between P3019r11 (Accepted in Plenary in Wroclaw 2024) and P3019r13

## Hagenberg Austria 2025-02-10

---

## Replacing _constraints_ with _mandates_ [LEWG]

Constraining a deduced template argument to avoid evaluating constraints at class
instatiation time works but composes badly with other types that may have `indirect`
and `polymorphic` members.

Another type may constrain its member functions using traits on its member variables.
If these constraints are not implemented in terms of deduced template arguments then
`indirect<T>` and `polymorphic<T>` will be unable to evaluate these constraints when
`T` is incomplete.

Replacing constraints (SFINAE/requirements) with mandates (`static_assert`) will leave
a program using unsupported functions ill-formed. Type traits will give potentially
misleading results but this is consistent with existing types in the standard library,
such as `vector`.

---

## Allowing indirect to be used with a non-copyable type [LEWG]

We required `indirect` to be unconditionally copyable so that it would work with
`variant` when constraints were on a deduced template type. There are other
implementations of `variant` for which this change is not sufficient for a
`variant` of `indirect` to work with an incomplete type.

We revert to our original design where `T` in `indirect` can be non-copyable.
This is consistent with `vector` where:
`std::is_copy_constructible_v<std::vector<NonCopyable>` is `True`.

---

## Removal of 'mandates `T` is a complete type' where this is implied by constraints

Constraints using type traits already require that the constrained type is complete.

We remove mandates that `T` is complete where it is constrained.

---

## Removal of `noexcept` on `indirect`'s three-way comparison

`synth-three-way` has no noexcept specification so defining
operator `<=>` to have `noexcept(noexcept(synth-three-way(A, B)))`
does not do anything meaningful.

---

## Consistently use `synth-three-way-result` for `indirect`

We misused `compare_three_way_result_t` in the synopsis of `indirect`
and have corrected this:

```diff
   template <class U, class AA>
   friend constexpr auto operator<=>(
-    const indirect& lhs, const indirect<U, AA>& rhs) noexcept(see below)
-    -> compare_three_way_result_t<T, U>;
+    const indirect& lhs, const indirect<U, AA>& rhs)
+    -> synth-three-way-result<T, U>;

   template <class U>
   friend constexpr auto operator<=>(
-    const indirect& lhs, const U& rhs) noexcept(see below)
-    -> compare_three_way_result_t<T, U>;
+    const indirect& lhs, const U& rhs)
+    -> synth-three-way-result<T, U>;
```

---

## Add missing `explicit` to `indirect` synopsis

We missed an `explicit` in the `indirect` synopsis and
have corrected this:

```diff
class indirect {
   using pointer = typename allocator_traits<Allocator>::pointer;
   using const_pointer = typename allocator_traits<Allocator>::const_pointer;

-  constexpr indirect();
+  explicit constexpr indirect();

   explicit constexpr indirect(allocator_arg_t, const Allocator& a);
```

---

## Sorting constraints consistently

Assorted diffs best viewed in <https://github.com/jbcoe/value_types/compare/r11...r13>

---

## Removing a redundant constraint that `U` is not `polymorphic` in `polymorphic`'s `in_place_type<U>` constructor

Diff is messy due to sorting and best viewed in <https://github.com/jbcoe/value_types/compare/r11...r13>

---

## Proposed poll

"Replace constraints on `T` in `polymorphic`'s default constructor, `indirect`'s default
constructor and comparison operators with mandates. Allow `indirect`, like `vector`, to
be used with non-copyable types."

Note: This means that type traits can report that `polymorphic` is default constructible,
`indirect` is default constructible, copy constructible or comparable when it is not.
This is the same behaviour as `vector`, which supports incomplete types and can give
misleading type-trait information.

---

## Conclusion

Thank you for the many hours of input, interest and engagement we have seen from folk
on `indirect` and `polymorphic`. These are, at heart, simple types but, if designed
well, have the power to change how people use C++.

I would also like to thank my assorted collaborators on GitHub. I have never met (most)
of you but you have made these types better and the open development of this proposal
is something I would strongly encourage and do again without hesitation.

---

## Appendix: List of git changes: `r11...r13`

- [LWG] Add missing `explicit` to `indirect` synoposis.

- [LEWG] Use _mandates_ not _constraints_ where the constraint is purely
  on the (potentially incomplete) type `T`.

- [LEWG] Remove _mandates_ that `T` is a complete type where it is implied by constraints.

- [LEWG] Add _mandates_ that `T` is copy-construcible in `indirect`'s copy constructors and assignments.

- [LEWG] Allow `indirect` to be used with non-copyable types.

- [LWG] Consistently use `synth-three-way-result` for `indirect`.

- [LEWG] Use _mandates_ for _constraints_ on comparison, in line with `optional`: https://eel.is/c++draft/optional#relops

- [None] Fix punctuation and style inconsistency issues.

- [LWG] Use 'may be only X' instead of 'may only be X'.

- [LEWG] Remove conditional `noexcept` on `indirect`'s three-way comparison.

- [None] Add additional example to clarify tagged constructor usage.

- [LWG] Fix typo where `alloc` was used when it should have been `a`.

- [LWG] Remove redundant constraint on `U` in `polymorphic`'s in-place constructors.

- [LWG] Consistently sort constraints.

- [None] Add detailed discussion of constraints and incomplete types.
