---
marp: true
theme: default
paginate: true
size: 16:9
footer: https://github.com/jbcoe/value_types
---

# Vocabulary Types for Composite Class Design

_Jonathan B. Coe, Antony Peacock & Sean Parent_

## BSI IST/5/-/21 (C++) panel

## [P3019r3](https://wg21.link/p3019r3)

### 2023-11-24

---

# Introduction

We propose adding two new class templates to the C++ Standard Library:

```c++
template <typename T, typename Allocator = std::allocator<T>>
class indirect;
```


```c++
template <typename T, typename Allocator = std::allocator<T>>
class polymorphic;
```

These fill a gap in the suite of existing standard library vocabulary types.

---

# Vocabulary Types

We refer to standard library types such as `std::array`, `std::map`,
`std::optional`,  `std::variant`, `std::tuple` and `std::vector` as
_vocabulary types_.

We postulate that an arbitrary piece of C++ library or application code would
make use of some of these types.

Standardizing vocabulary types is important as it allows different libraries to
easily interoperate and for users to build applications.

The standard library contains other, non-vocabulary types to do specific jobs
such as interacting with the filesystem, formatting text for output, or dealing
with concurrency.

We probably want to standardize both sorts of library type.

---

# Composite classes

We define composite classes as classes with other class instances as member
data.

Vocabulary types can be used to express common idioms.

---

| Idiom | Type |
|---|---|
| An instance of an object `T` | `T` |
| A nullable instance of an object `T` | `std::optional<T>` |
| An instance of one of a closed-set of types `Ts...`| `std::variant<Ts...>`|
| Once instance of each of a closed-set of types `Ts...`| `std::tuple<Ts...>`|
| `N` instances of a type `T`| `std::array<T, N>`|
| Variable-count, multiple instances of a type `T`| `std::vector<T>`|
| Unique, variable-count, instances of a type `T`| `std::set<T>`|
| Key-accessed, instances of a type `T`| `std::map<Key, T>`|

---

# Special member functions

The compiler can generate special member functions. Each special member function
can be compiler-generated if it is supported by all component objects.

| Special member(s) | Signature(s) |
|----|---|
| Default constructor | `T();` |
| Destructor | `~T();` |
| Copy constructor/assignment | `T(const T&);` `T& operator=(const T&);` |
| Move constructor/assignment|  `T(T&&);` `T& operator=(T&&);` |
---

# Const propagation

The compiler will only allow const-qualified member functions to be called on
components when they are accessed through a const-access path.

We call this _const-propagation_.

Standard library vocabulary types provide const-qualified and
non-const-qualified overloads of accessors to owned objects to enforce const
propagation:

```c++
const T& std::vector<T>::operator[](size_t index) const;
T& std::vector<T>::operator[](size_t index);
```

---

# Requirements for vocabulary types

Composite classes built with vocabulary types should be composable:

* const propagates.

* The compiler can generate all special member functions where they are defined
  for an owned type `T`.

These requirements make working with C++ easier - classes should do business
logic or resource management, not both.

---

# Combining vocabulary types

We can combine vocabulary types to express combined idioms:

| Idiom | Type |
|---|---|
| Variable-count, multiple, nullable instances of a type `T`| `std::vector<std::optional<T>>`|
| Key-accessed, instances of one of a closed-set of types `Ts...`| `std::map<Key, std::variant<Ts...>>`|

---

# Incomplete types

We may want a composite to contain an instance of an incomplete type, either
directly or through the use of existing vocabulary types.

Incomplete type support is needed for defining recursive data structures,
supporting open-set polymorphism, and hiding implementation detail.

Incomplete types are supported by storing the object of the incomplete type
outside of the composite object.

Storing an object outside of a composite type can also be used to optimize use
of cache (hot/cold splitting).

---

## Incomplete types using pointers

Since we want to store the object of incomplete type outside of our composite
type, pointers are the first thing we may reach for:

```c++
class Composite {
    T* ptr_;
    // ...
}
```

Pointers are a poor fit for owned data as we need to implement all special
member functions and manually check const-qualified methods (const does not
propagate to a pointee when the pointer is accessed through a const-access
path).

---

## Incomplete types using `std::unique_ptr<T>`

Unique pointers are a little better:

```c++
class Composite {
    std::unique_ptr<T> ptr_;
    // ...
}
```

We do not need to implement move construction, move assignment or destruction.
The compiler will implicitly delete the copy constructor and copy assignment
operator; either the composite is non-copyable or we implement the copy
constructor and copy assignment operator ourselves.

Const does not propagate through `std::unique_ptr`, we must manually check
const-qualified methods for correctness.

---

## Incomplete types using `std::shared_ptr<T>`

Shared pointers do not model the right thing:

```c++
class Composite {
    std::shared_ptr<T> ptr_;
    // ...
}
```

The compiler-generated copy constructor and assignment operator will give rise
to multiple composite objects that share the same components: that is not
ownership.

Const does not propagate through `std::shared_ptr<T>`, we must manually check
const-qualified methods for correctness.

---

## Incomplete types using `std::shared_ptr<const T>`

Shared pointers to const might work:

```c++
class Composite {
    std::shared_ptr<const T> ptr_;
    // ...
}
```

Again, the compiler-generated copy constructor and assignment operator will give
rise to multiple composite objects that share the same components, _but_ the
shared components are immutable.

We cannot call non-const qualified member functions accessed though `ptr_`, part
of our composite is immutable.

---

# New vocabulary types (post P3019)

| Idiom | Type |
|---|---|
| An instance of an object of an incomplete type `T` | `indirect<T>` |
| An instance of an object from an open set of types derived from a base type `T` | `polymorphic<T>` |

---

# Required properties of our new vocabulary types

Any composable vocabulary type needs:

* Resource ownership (destruction and move).

* Deep copies.

* const-propagation.

Both of the proposed types, `indirect` and `polymorphic`, need:

* Indirectly allocated object storage.

---

# Composition with `std::unique_ptr`

```c++
struct Foo
{
    unique_ptr<Bar> a;
    unique_ptr<Bar> b;
};
```

behaves like

```c++
struct Foo
{
    Bar* a;
    Bar* b;
};
```

---

# Where as composition with `std::indirect`

```c++
struct Foo
{
    indirect<Bar> a;
    indirect<Bar> b;
};
```

behaves like

```c++
struct Foo
{
    Bar a;
    Bar b;
};
```

---

# Benefits of value types over pointer types

- Deep copy
- Deep comparison
- Const propagation
- Copy semantics rather than move-only semantics
- Allocator support

---

```c++
#include <boost/interprocess/allocators/adaptive_pool.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <scoped_allocator>
#include <vector>
#include <memory>

namespace bi = boost::interprocess;

template<class T> using alloc = bi::adaptive_pool<T, bi::managed_shared_memory::segment_manager>;
using icp_value = std::indirect<int, alloc<int>>;
using ipc_vector = std::vector<icp_value, std::scoped_allocator_adaptor<alloc<icp_value>>>;

int main ()
{
    bi::managed_shared_memory s(bi::create_only, "Example", 65536);

    // create vector of values in shared memory
    ipc_vector v(s.get_segment_manager());

    // The inner type propagates the allocator type from the outer type's scope
    v.reserve(10);
    for(int i = 0; i < 10; i++)
      v.emplace_back(42+i);

    bi::shared_memory_object::remove("Example");
}
```

---

# How has the design evolved?

- P1950R2: indirect_value -> indirect
- P0201R6: polymorphic_value -> polymorphic

---

# Previous version support a copier and deleter model

```c++
template <
  typename T,
  typename Copier = default_copy<T>,
  typename Deleter = std::default_delete<T>
>
class indirect_value;
```

```c++
template <typename T>
class polymorphic_value {
public:
    template <class U, class C = default_copy<U>, class D = std::default_delete<T>>
    constexpr explicit polymorphic_value(U* u, C c = C(), D d = D())
}
```

---

# Current design is now based on allocators

```c++
template <typename T, typename Allocator = std::allocator<T>>
class indirect;
```

```c++
template <typename T, typename Allocator = std::allocator<T>>
class polymorphic;
```

---

# Other significant design changes

- Removal of the null-state in favour of null state via optional
  * `std::optional<std::indirect<T>>`
  * `std::optional<std::polymorphic<T>>`
- All constructors are now explicit
- Additional constructors to support allocators
- Deduction guides for constructors
- Comparison operators return `auto`

---

# Current Status

- Design approved by LEWG (remaining issues to be resolved by telecon)
- Remaining issues:
  * `std::indirect` comparison operators
  * Pre-conditons as a result of moved from state

---

# Behaviour of `indirect` comparison operators

`std::indirect` is a wapper type so comparison operators forward to
underlying type and return `auto`.

It was suggested standard types should always be regular so should
return `bool`

A survey of non-bool equality overload identified the follow categories:
- Valarray-like containers: element-wise operator== overload
- SIMD types: SIMD libraries operator == return masks
- `int`-returning comparisons
- Expression templates
- Monotype API: All operations return the same 'handle' type.

---

# Explicity handling the valueless state has a cost

```
Run on (12 X 24 MHz CPU s)
CPU Caches:
  L1 Data 64 KiB
  L1 Instruction 128 KiB
  L2 Unified 4096 KiB (x12)
Load Average: 0.98, 1.38, 1.66
----------------------------------------------------------------------------------
Benchmark                                        Time             CPU   Iterations
----------------------------------------------------------------------------------
Int_SortingBenchmark                      28352850 ns     27718636 ns           22
Optional_Int_SortingBenchmark             56351883 ns     56240727 ns           11
VariantInt_SortingBenchmark              105704208 ns    105381667 ns            6
Indirect_SortingBenchmark                213564833 ns    211927333 ns            3
Indirect_ValuelessCheckSortingBenchmark  267500583 ns    265372500 ns            2
```

---

# Further details

* Our proposal, with design discussion and proposed formal wording:
  <https://wg21.link/p3019r1>

* Our (header-only, C++20) reference implementation with tests and benchmarks:
  <https://github.com/jbcoe/value_types>

* Original `indirect_value` design and proposal:
  https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1950r2.html>

* Original `polymorphic_value` design and proposal:
  <https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p0201r6.html>

---

# Acknowledgements

Thanks to @nbx8 and @Ukilele for short-notice review of this material.
