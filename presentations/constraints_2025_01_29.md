---
marp: true
theme: default
paginate: true
size: 16:9
footer: https://github.com/jbcoe/value_types
---

# Constraints on Indirect and Polymorphic

_Jonathan B. Coe, Neelofer Banglawala, Antony Peacock

## BSI IST/5/-/21 (C++) panel

## [P3019r3](https://wg21.link/p3019r13)

### 2025-02-04

---

## Introduction

We explore the design space for constraints on wrapper-types with incomplete type support.

We avoid the allocator related complexity of `indirect` and `polymorphic` by considering a
simple wrapper type.

---

Our simple wrapper type wraps another type:

```c++
template <typename T>
class basic_wrapper {
  T t;
 public:
  ...
};
```

---

If the wrapper needs to support incomplete types, we can't store an instance of `T`
directly, instead we store a pointer to `T`:

```c++
template <typename T>
class wrapper {
  T* t;
 public:
  wrapper() { t_ = new T(); }

  wrapper(const wrapper& other) { t_ = new T(*other.t_); }

  friend bool operator==(const wrapper& lhs, const wrapper& rhs) {
    return *lhs.t_ == *rhs.t_;
  }
};
```

---

To constrain member functions with incomplete types we defer the constraint
check by adding a deduced template argument

```c++
template <typename T>
class constrained_wrapper {
  T* t;
 public:
    template <typename TT = T>
  constrained_wrapper()
    requires std::default_initializable<TT>
      : t_(new T()) {}

  constrained_wrapper(const constrained_wrapper& other)
    requires false;

  template <typename TT = T>
  constrained_wrapper(const constrained_wrapper& other)
    requires std::copy_constructible<TT>
      : t_(new T(*other.t_)) {}

  template <typename U>
  friend bool operator==(const constrained_wrapper<T>& lhs,
                         const constrained_wrapper<U>& rhs)
    requires std::equality_comparable_with<T, U>
  {
    return *lhs.t_ == *rhs.t_;
  }
};
```

---

A composite type may contain an instance of a wrapper on an incomplete type.

For the compiler to determine if it should generate special member functions,
the type will need to be complete at class instantiation time if the wrapper is constrained.

The need for complete types can be circumvented by forward declaring (or deleting) the constrained functions.

```cpp
class Incomplete;
class composite {
  constrained_wrapper<Incomplete> x_;

  composite();
  composite(const composite&);
  friend bool operator==(const composite&, const composite&);
};
```

---

When the wrapper is unconstrained, the compiler can generate special member functions.

If the wrapped type would not have satisfied the relevant constraint, then the program
is ill-formed at the point where the function is used.

```cpp
class Incomplete;
class composite2 {
  wrapper<Incomplete> x_;

  // composite(); // Implicitly defaulted.
  // composite(const composite&); // Implicitly defaulted.
  friend bool operator==(const composite&, const composite&) = default;
};
```

---

If the wrapped type is a template argument, for a constrained wrapper to accurately represent the
supported member functions, the deferred constraint check trick needs to be (virally) repeated:

```cpp
template<typename Incomplete_T>
class composite3 {
  constrained_wrapper<Incomplete_T> x_;

  template <typename TT=T>
  composite3() requires std::default_constructible<T> = default;

  composite3(const composite3&) requires false;

  template <typename TT=T>
  composite3(const composite3&) requires std::copy_constructible<T>;
  // requires manual implementation.

  template <typename U>
  friend bool operator==(const composite3<T>& lhs,
                         const composite3<U>& rhs)
    requires std::equality_comparable_with<T, U> = default;
};
```

---

Existing types in the standard library with incomplete type support and value semantics
give misleading information in type_traits:

```cpp
class Restricted {
  Restricted(const Restricted&) = delete;
}

class composite4 {
  std::vector<Restricted> xs_;
};

static_assert(std::copy_constructible<composite4>); // True.

composite4 c;
composite4 cc(c); // Ill-formed.
```

---

Vector is old and some of the deferred type constraint tricks were unknown or
not possible to implement when vector was originally designed.

The authors believe that `indirect` should be consistent with `vector` and will
report itself to be default constructible, copy constructible and equality comparable
for a potentially incomplete type. Use of any unsupported function would render a program
ill-formed.

Both indirect and polymorphic are intended to be user-friendly vocabulary types without
significant teaching burden.

---

Changing indirect, polymorphic and vector to correctly report their constraints via type
traits or concepts by adding deferred constraints using a deduced template type would be a
breaking change.

Existing well-formed code would be rendered ill-formed and special member
functions would need to be explicitly forward declared or deleted.

```diff
class Incomplete;
class Composite {
  std::vector<Incomplete> xs_;
+ Composite(const Composite&);
};
```

This could be mitigated with tooling.
