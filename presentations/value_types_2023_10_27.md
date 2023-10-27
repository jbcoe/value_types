---
marp: true
theme: default
paginate: true
size: 16:9
footer: https://github.com/jbcoe/value_types
---

# Value types for composite class design

GDM Compiler Club 2023-10-27

Jonathan B. Coe, Antony Peacock & Sean Parent

<https://github.com/jbcoe/value_types>

---

# Overview

* We recommend the use of two new class templates, `indirect` and `polymorphic` to make the design of composite classes simple and correct.

* We'll look into some of the challenges of composite class design and see which problems are unsolved by current vocabulary types.

* We'll look at implementations of the proposed new types (mostly polymorphic).

---

# Encapsulation with classes (and structs)

Classes (and structs) let us group together logically associated data and functions that operate on that data:

```cpp
class Circle {
    std::pair<double, double> position_;
    double radius_;
    std::string colour_;

public:
    std::string_view colour() const; 
    double area() const;
};
```

---

# Encapsulation with classes (and structs) II

User-defined types can be used for member data:

```cpp
struct Point {
    double x, y;
};

class Circle {
    Point position_;
    double radius_;
    std::string colour_;
};
```

---

# Special member functions

We can define special member functions to create, copy, move or destroy instances of our class:

```cpp
class Circle {
    Circle(std::string_view colour, double radius, Point position);
    
    Circle(const Circle&);
    Circle& operator=(const Circle&);
    
    Circle(Circle&&);
    Circle& operator=(Circle&&);

    ~Circle();
};
```

---

# Compiler generated functions

* We can specify special member functions for classes we define.

* The compiler will (sometimes) generate special member functions for us.

* Generated special member functions are member-by-member calls to the appropriate 
  special member function of each member object. 

---

# Compiler generated functions with pointer members

When a class contains pointer members, the compiler generated special
member functions will copy/move/delete the pointer but not the pointee:

```cpp
struct A {
    A(...);
    B* b;
};

A a(...);
A a2(a);
assert(a.b == a2.b);

```

This might require us to write our own versions of the special member functions, something we'd sorely like to avoid having to do.

---

# `const` in C++

Member functions in C++ can be const-qualified:

```cpp
struct A {
    void foo() const;
    void foo(); 
    void bar();
};
```
 When an object is accessed through a const-access-path then only const-qualified member functions can be called:

```~cpp
const A a;
a.bar();
```

```~shell
error: passing 'const A' as 'this' argument discards qualifiers
```

---

# `const` propagation

An object's member data becomes const-qualified when the object is accessed through a const-access-path:

```cpp
class A {
  public:
    void foo(); // non-const
};

class B {
    A a_;
  public:
    void bar() const { a_.foo(); }
};
```

```~shell
error: passing 'const A' as 'this' argument discards qualifiers
```

---

# `const` propagation and reference types

* Pointer (or reference) member data becomes const-qualified when accessed through a const-access-path, but the const-ness does not propagate through to the pointee.

* Pointers can't be made to point at different objects when accessed through a const-access-path but the object they point to can be accessed in a non-const manner.

* const-propagation must be borne in mind when designing composite classes for const-correctness.

---

# class-instance members

Class-instance members are often a good option for member data.

```cpp
class A {
  B b_;
  C c_;
};
```

Class-instance members ensure const-correctness.

Compiler generated special member functions will be correct.

---

# Repeated member data

We might have cause to store a variable number of objects as part of our class

```cpp
class Talk {
    Person speaker_;
    std::vector<Person> audience_;
}
```

Standard library containers like `vector` and `map` support a variable number
of objects and have special member functions that will handle the contents of the container.

---

# Indirect storage

We may have member data that is too big to be sensibly stored directly in the class.

If member data is accessed infrequently we might want it stored elsewhere to keep cache lines hot.

```cpp
class A {
    Data data_;
    BigData big_data_; // We want this stored elsewhere.
}
```

---

# Incomplete types

If the definition of a member is not available when a class is defined then we'll need to store the member as an incomplete type.

This can come about it node-like structures:

```cpp
class Node {
    int data_;
    Node next_; // won't compile as `Node` is not yet defined.
}
```
---

# The Pointer To Implementation Pattern

The PIMPL pattern can be used to reduce compile times and keep ABI stable.
We store an incomplete type which defines the implementation detail of our class.

This can come about it node-like structures:

```cpp
class A {
  public:
    int foo();
    double bar();
  private:
    Impl implementation_; // won't compile as `Impl` is not yet defined.
}
```

Where `A` is defined in a header file we want to define `Impl` in the associated `cc` source file.

---

# Polymorphism

We might require a member of our composite class to be one of a number of types.

* A Zoo could contain a list of Animals of different types.

* A code_checker could contain different checking_tools.

* A Game could contain different GameEntities.

* A Portfolio could contain different kinds of FinancialInstrument.

Our class will need to reserve storage for our polymorphic data member.

--- 

# Closed-set polymorphism

Closed-set polymorphism gives users of a class a choices for member data from a known set of types. We can use sum-types like `optional` and `variant` to
represent this.

```cpp
class Taco {
    std::optional<Avocado> avocado_;
    std::variant<Chicken, Pork, Beef> meat_;
    std::variant<Chipotle, GhostPepper, Hot> sauce_;
};
```

Storing a closed-set polymorphic member directly is possible as `variant` and `optional` reserve enough memory for the largest possible type.

--- 

# Open-set polymorphism

Open-set polymorphism allows users of a class represent member data with types
that were not known when the class was defined.

```
struct SimulationObject {
    virtual ~SimulationObject();
    virtual void update() = 0;
};

class Simulation {
    ???? simulation_objects_;
};
```

Storing open-set polymorphic objects is challenging. We've no idea how much memory
any of the objects might take so direct storage of the data is not possible.

---

# pointer (and reference) members

We can support polymorphism and incomplete types by storing a pointer as a member.

The pointer can be an incomplete type:

```cpp
class A {
    class B* b_;
    class A* next_;
}
```

or the base type in a class heirarchy:

```
struct Shape { 
    virtual void Draw() const = 0;
};

class Picture { Shape* shape_; }
```

---

# Collections of pointers as members

We can store multiple pointers to objects in our class in standard library collections:

```cpp
struct Animal {
    const char* MakeNoise() const = 0;
};

class Zoo {
    std::vector<Animal*> animals_;
};

class SafeZoo {
    std::map<std::string, std::vector<Animal*>> animals_;
};
```

---

# Issues with pointer members

* Compiler generated special members functions handle only the pointer, not the pointee.

* `const` will not propagate to pointees

* If we want to model ownership in our composite then we'll have to do work:

    * Manually maintain special member functions.
    * Check const-qualified member functions for const correctness.

---

# Improving on pointer members

C++'s handling of pointers is not wrong, but in the examples above, we've failed to communicate what we mean to the compiler.

There are instances where pointer members perfectly model what we want to express but such instances are not composites:

```cpp
class Worker {
    std::string name_;
    Manager* manager_;
};
```

Let's see if we can do better.

---

# Smart pointers

Smart pointers have different semantics to raw pointers and can be used to express intent to the compiler.

* Ownership can be transferred.

* Resources can be freed on destruction.

C++98 had std::auto_ptr. With the introduction of move semantics, we have improved smart pointers in C++11.

[C++11 deprecated std::auto_ptr. C++17 removed it.]

---

# `std::unique_ptr<T>` members

```cpp
class A {
    std::unique_ptr<B> b_;
};
```

This is an improvement over raw-pointer members.

We now have a correct compiler-generated destructor, move-constructor and move-assignment operator.

`std::unique_ptr` is non-copyable, so the compiler won't generate a copy constructor or assignement operator for us.

`std::unique_ptr` does not propagate `const` so we'll have to check our implementations of const-qualified member functions ourselves.

---

# `std::shared_ptr<T>` members

```cpp
class A {
    std::shared_ptr<B> b_;
};
```

With `shared_ptr` members, the compiler can generate all special member functions for us. 

Sadly this is not much of an improvement as the copy constructor and assignment operator will not copy the `B` object that we point to, only add references to it.

We now have mutable shared state and still no const-propagation.

---

# `std::shared_ptr<const T>` members

```cpp
class A {
    std::shared_ptr<const B> b_;
};
```

Again, the compiler can generate all special member functions for us. 

Copy and assignement will add references to the same `B` object but seeing as it's immutable that could be ok (so long as it's not mutable by another route).

We've lost the ability to call any mutation method of the B object though as any access to it is through a const-access-path.

---

# New proposed classes

We propose the addition of two new class templates

* `polymorphic`

* `indirect`

---

# `polymorphic<T>` members

```cpp
class A {
    polymorphic<B> b_; // proposed addition to the standard
};
```

A `polymorphic` member allows the compiler to generate special member functions correctly and propagates `const` so that const-qualified member functions can be verified by the compiler.

`B` is allowed to be a base class and `b_` can store an instance of a derived type.
Copying and deleting derived types works correctly as polymorphic is implemented using type-erasure.

---

# Design of `polymorphic`

`polymorphic` is a class template taking a single template argument - the base class of the types that we want to store.

```cpp
template <class T, class Allocator=std::allocator<T>>
class polymorphic;
```
---

## Constructors

```cpp
polymorphic() noexcept;

template <class U, class... Ts> // restrictions apply
explicit constexpr polymorphic(std::in_place_type_t<U>, Ts&&... ts);
```

---

## Move and copy

```cpp
constexpr polymorphic(const polymorphic& p);

constexpr polymorphic(polymorphic&& p) noexcept;
```

---

## Assignment

```cpp
constexpr polymorphic& operator=(const polymorphic& p);

constexpr polymorphic& operator=(polymorphic &&p) noexcept;

```

---

## Modifiers and observers
```cpp
constexpr void swap(polymorphic& p) noexcept;

constexpr T& operator*() noexpect;
constexpr T* operator->() noexcept;

constexpr const T& operator*() const noexcept;
constexpr const T* operator->() const noexcept;

constexpr allocator_type get_allocator() const noexcept;

constexpr bool valueless_after_move() const noexcept;

```

---

# Implementing `polymorphic`

```cpp
template <class T, class A = std::allocator<T>>
class polymorphic {
  detail::control_block<T, A>* cb_;
  [[no_unique_address]] A alloc_;

 public:
  // Constructors and assignment. 

  constexpr T* operator->() noexcept { return *cb_->p_; }
  constexpr const T* operator->() const noexcept { return *cb_->p_; }

  constexpr T& operator*() noexcept { return **cb_->p_; }
  constexpr const T& operator*() const noexcept { return **cb_->p_; } 
  
  constexpr bool valueless_after_move() const noexcept { return bool(cb_); }
};
```

---


## Implementing the control block

Control blocks will inherit from a base-class:

```cpp
template <class T, class A>
struct control_block {
  T* p_;
  virtual constexpr ~control_block() = default;
  virtual constexpr void destroy(A& alloc) = 0;
  virtual constexpr control_block<T, A>* clone(A& alloc) = 0;
};
```

---

## Construction from a known type
We can support constructors of the form:

```cpp
template<class U, class ...Ts> // restrictions apply
polymorphic(std::in_place_type<U>, Ts...ts);
```

with a suitable control block:

---

## `polymorphic` control block

```cpp
template <class T, class U, class A>
class direct_control_block : public control_block<T, A> {
  U u_;

 public:
  constexpr ~direct_control_block() override = default;

  template <class... Ts>
  constexpr direct_control_block(Ts&&... ts) : u_(std::forward<Ts>(ts)...) {
    control_block<T, A>::p_ = &u_;
  }

  // ...
```

---

## `polymorphic` control block (continued - copies)
```cpp
  // ...

  constexpr control_block<T, A>* clone(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    auto* mem = cb_alloc_traits::allocate(cb_alloc, 1);
    try {
      cb_alloc_traits::construct(cb_alloc, mem, u_);
      return mem;
    } catch (...) {
      cb_alloc_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }
```

---

## `polymorphic` control block (continued - destruction)

```cpp
  // ...

  constexpr void destroy(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    cb_alloc_traits::destroy(cb_alloc, this);
    cb_alloc_traits::deallocate(cb_alloc, this, 1);
  }
};
```

---

## `polymorphic` default constructor

```cpp
  constexpr polymorphic()
    requires std::default_initializable<T>
  {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, T, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto* mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }
```

---

## `polymorphic` in-place constructor

```cpp
  template <class U, class... Ts>
  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> &&
             (std::derived_from<U, T> || std::same_as<U, T>)
      : alloc_(alloc) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto* mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }
```

---

## `polymorphic` copy-constructor

```cpp
  constexpr polymorphic(const polymorphic& other)
      : alloc_(allocator_traits::select_on_container_copy_construction(
            other.alloc_)) {
    assert(other.cb_ != nullptr);
    cb_ = other.cb_->clone(alloc_);
  }
```

---

## `polymorphic` move-constructor

```cpp
  constexpr polymorphic(polymorphic&& other) noexcept : alloc_(other.alloc_) {
    assert(other.cb_ != nullptr);
    cb_ = std::exchange(other.cb_, nullptr);
  }
```
---

# Using `polymorphic` in your code

`polymorphic` is a single-file-, header-only-library and can be included in your C++ project by using the header file from our reference implementation

https://github.com/jbcoe/value_types

```
jbcoe/value_types is licensed under the

MIT License

A short and simple permissive license with conditions only 
requiring preservation of copyright and license notices.
Licensed works, modifications, and larger works may be 
distributed under different terms and without source code.
```

---

# `indirect<T>` members

```cpp
class A {
    indirect<B> b_; // proposed addition to the standard
};
```

An `indirect` member allows the compiler to generate special member functions correctly and propagates `const` so that const-qualified member functions can be verified by the compiler.

`B` can be an incomplete type. `b_` can only store an instance of `B`. Copying
and deleting the owned object works without virtual dispatch.

---

# Design of `indirect`

`indirect` is a class template

```cpp
template <class T, class Allocator=std::allocator<T>>
class indirect;
```

---

## Constructors

```cpp
constexpr indirect() noexcept;

template <class U, class... Ts>
constexpr explicit indirect(std::in_place_t, Ts&&... ts);
```

---

## Move and Copy

```cpp
constexpr indirect(const indirect& i);
constexpr indirect(indirect&& i) noexcept;
```

## Assignment

```cpp
constexpr indirect& operator=(const indirect& i);
constexpr indirect& operator=(indirect&& i) noexcept;
```

---

## Modifiers and observers

```cpp
constexpr void swap(indirect<T>& p) noexcept;

constexpr explicit operator bool() const noexcept;

constexpr T& operator*() noexcept;
constexpr T* operator->() noexcept;

constexpr const T& operator*() const noexcept;
constexpr const T* operator->() const noexcept; 
```

---

## Three way comparison and hash

`indirect` specializes `std::hash` when the stored type specializes `std::hash`.

`indirect` supports binary comparsion operations in cases where the stored type supports  binary comparsion operations.

`indirect` supports three-way-comparison (operator `<=>`) in cases where the stored type supports three-way-comparison.

---

# Implementing `indirect<T>`

`indirect` could be implemented with a raw pointer:

```cpp
template <class T, class Allocator = std::allocator<T>>
class indirect {
    T* ptr_;
    [[no_unique_address]] Allocator a_;
  public:
    // Constructors elided 

    constexpr T* operator->() noexcept { return ptr_; }
    constexpr const T* operator->() const noexcept { return ptr_; }
    constexpr T& operator*() & noexcept { return *ptr_; }
    constexpr const T& operator*() const& noexcept { return *ptr_; }
};
```

---

# Using `indirect<T>` in your code

`indirect` is a single-file-, header-only-library and can be included in your C++ project by using the header file from our reference implementation

https://github.com/jbcoe/value_types

```
jbcoe/value_types is licensed under the

MIT License

A short and simple permissive license with conditions only 
requiring preservation of copyright and license notices.
Licensed works, modifications, and larger works may be 
distributed under different terms and without source code.
```
---

# Values not references

The two class templates we've proposed are value types and treat the objects they own as values:

* They delete their owned objects upon destruction.

* They copy their owned objects (correctly) when copied.

* When a value type is copied it gives rise to two independent objects that can be modified separately.

Value types are the right choice for the design of composite classes (unless we want to do a bunch of extra work).

---

# Standardisation efforts

There is a proposal to add `indirect` and `polymorphic` to C++26.

* Polymorphic Value https://wg21.link/p3019r0

I have travel booked to present this work to the standards committee in November

---

# Acknowledgements

Many thanks to Sean Parent and to the Library Evolution Working group from the C++ Standards committee for interesting early discussion in the design
of `polymorphic`.

Thanks to our GitHub contributors who've made worked with us to improve our reference implementations.