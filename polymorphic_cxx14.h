/* Copyright (c) 2016 The Value Types Authors. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
==============================================================================*/

#ifndef XYZ_POLYMORPHIC_H_
#define XYZ_POLYMORPHIC_H_

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

#ifndef XYZ_IN_PLACE_TYPE_DEFINED
#define XYZ_IN_PLACE_TYPE_DEFINED
namespace xyz {

struct NoPolymorphicSBO {};

template <class T>
struct in_place_type_t {};
#endif  // XYZ_IN_PLACE_TYPE_DEFINED

#ifndef XYZ_UNREACHABLE_DEFINED
#define XYZ_UNREACHABLE_DEFINED
[[noreturn]] inline void unreachable() {  // LCOV_EXCL_LINE
#if (__cpp_lib_unreachable >= 202202L)
  std::unreachable();  // LCOV_EXCL_LINE
#elif defined(_MSC_VER)
  __assume(false);  // LCOV_EXCL_LINE
#else
  __builtin_unreachable();  // LCOV_EXCL_LINE
#endif
}
#endif  // XYZ_UNREACHABLE_DEFINED

#ifndef XYZ_EMPTY_BASE_DEFINED
#define XYZ_EMPTY_BASE_DEFINED
// This is a helper class to allow empty base class optimization.
// This implementation is duplicated in compatibility/in_place_type_cxx14.h.
// These implementations must be kept in sync.
// We duplicate implementations to allow this header to work as a single
// include. https://godbolt.org needs single-file includes.
// TODO: Add tests to keep implementations in sync.
namespace detail {
template <class T, bool CanBeEmptyBaseClass =
                       std::is_empty<T>::value && !std::is_final<T>::value>
class empty_base_optimization {
 protected:
  empty_base_optimization() = default;
  empty_base_optimization(const T& t) : t_(t) {}
  empty_base_optimization(T&& t) : t_(std::move(t)) {}
  T& get() noexcept { return t_; }
  const T& get() const noexcept { return t_; }
  T t_;
};

template <class T>
class empty_base_optimization<T, true> : private T {
 protected:
  empty_base_optimization() = default;
  empty_base_optimization(const T& t) : T(t) {}
  empty_base_optimization(T&& t) : T(std::move(t)) {}
  T& get() noexcept { return *this; }
  const T& get() const noexcept { return *this; }
};
}  // namespace detail
#endif  // XYZ_EMPTY_BASE_DEFINED

namespace detail {
template <class T, class A>
struct control_block {
  using allocator_traits = std::allocator_traits<A>;

  typename allocator_traits::pointer p_;

  virtual ~control_block() = default;
  virtual void destroy(A& alloc) = 0;
  virtual control_block<T, A>* clone(A& alloc) = 0;
};

template <class T, class U, class A>
class direct_control_block final : public control_block<T, A> {
  U u_;

 public:
  ~direct_control_block() override = default;

  template <class... Ts>
  direct_control_block(Ts&&... ts) : u_(std::forward<Ts>(ts)...) {
    control_block<T, A>::p_ = &u_;
  }

  control_block<T, A>* clone(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    auto mem = cb_alloc_traits::allocate(cb_alloc, 1);
    try {
      cb_alloc_traits::construct(cb_alloc, mem, u_);
      return mem;
    } catch (...) {
      cb_alloc_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  void destroy(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    cb_alloc_traits::destroy(cb_alloc, this);
    cb_alloc_traits::deallocate(cb_alloc, this, 1);
  }
};

}  // namespace detail

template <class T, class A = std::allocator<T>>
class polymorphic : private detail::empty_base_optimization<A> {
  using cblock_t = detail::control_block<T, A>;
  cblock_t* cb_;

  using allocator_traits = std::allocator_traits<A>;
  using alloc_base = detail::empty_base_optimization<A>;

 public:
  using value_type = T;
  using allocator_type = A;
  using pointer = typename allocator_traits::pointer;
  using const_pointer = typename allocator_traits::const_pointer;

  polymorphic() {
    static_assert(std::is_default_constructible<T>::value, "");
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, T, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_base::get());
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  template <
      class U, class... Ts,
      typename std::enable_if<std::is_constructible<U, Ts&&...>::value,
                              int>::type = 0,
      typename std::enable_if<std::is_copy_constructible<U>::value, int>::type =
          0,
      typename std::enable_if<std::is_base_of<T, U>::value, int>::type = 0>
  explicit polymorphic(in_place_type_t<U>, Ts&&... ts) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_base::get());
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  template <
      class U, class... Ts,
      typename std::enable_if<std::is_constructible<U, Ts&&...>::value,
                              int>::type = 0,
      typename std::enable_if<std::is_copy_constructible<U>::value, int>::type =
          0,
      typename std::enable_if<std::is_base_of<T, U>::value, int>::type = 0>
  polymorphic(std::allocator_arg_t, const A& alloc, in_place_type_t<U>,
              Ts&&... ts)
      : alloc_base(alloc) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_base::get());
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  polymorphic(const polymorphic& other)
      : alloc_base(allocator_traits::select_on_container_copy_construction(
            other.alloc_base::get())) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_->clone(alloc_base::get());
  }

  polymorphic(std::allocator_arg_t, const A& alloc, const polymorphic& other)
      : alloc_base(alloc) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_->clone(alloc_base::get());
  }

  polymorphic(polymorphic&& other) noexcept
      : alloc_base(other.alloc_base::get()) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_;
    other.cb_ = nullptr;
  }

  polymorphic(std::allocator_arg_t, const A& alloc,
              polymorphic&& other) noexcept
      : alloc_base(alloc) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_;
    other.cb_ = nullptr;
  }

  ~polymorphic() { reset(); }

  polymorphic& operator=(const polymorphic& other) {
    if (this == &other) return *this;
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    if (allocator_traits::propagate_on_container_copy_assignment::value) {
      if (alloc_base::get() != other.alloc_base::get()) {
        reset();  // using current allocator.
        alloc_base::get() = other.alloc_base::get();
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    cb_ = other.cb_->clone(alloc_base::get());
    return *this;
  }

  polymorphic& operator=(polymorphic&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value ||
      allocator_traits::is_always_equal::value) {
    if (this == &other) return *this;
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    if (allocator_traits::propagate_on_container_move_assignment::value) {
      if (alloc_base::get() != other.alloc_base::get()) {
        reset();  // using current allocator.
        alloc_base::get() = other.alloc_base::get();
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    if (alloc_base::get() == other.alloc_base::get()) {
      std::swap(cb_, other.cb_);
    } else {
      cb_ = other.cb_->clone(alloc_base::get());
    }
    return *this;
  }

  pointer operator->() noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  const_pointer operator->() const noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  T& operator*() noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  const T& operator*() const noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  bool valueless_after_move() const noexcept { return cb_ == nullptr; }

  allocator_type get_allocator() const noexcept { return alloc_base::get(); }

  void swap(polymorphic& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE

    if (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects we can swap both.
      std::swap(alloc_base::get(), other.alloc_base::get());
      std::swap(cb_, other.cb_);
      return;
    } else /*  */ {
      if (alloc_base::get() == other.alloc_base::get()) {
        std::swap(cb_, other.cb_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend void swap(polymorphic& lhs,
                   polymorphic& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

 private:
  void reset() noexcept {
    if (cb_ != nullptr) {
      cb_->destroy(alloc_base::get());
      cb_ = nullptr;
    }
  }
};

}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_
