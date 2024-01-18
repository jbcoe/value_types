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

#ifndef XYZ_INDIRECT_H
#define XYZ_INDIRECT_H

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

namespace xyz {

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
// This implementation is duplicated in compatibility/polymorphic_cxx14.h.
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

template <class T, class A>
class indirect;

template <class>
struct is_indirect : std::false_type {};

template <class T, class A>
struct is_indirect<indirect<T, A>> : std::true_type {};

template <class T, class A = std::allocator<T>>
class indirect : private detail::empty_base_optimization<A> {
  using allocator_traits = std::allocator_traits<A>;
  using alloc_base = detail::empty_base_optimization<A>;

 public:
  using value_type = T;
  using allocator_type = A;
  using pointer = typename allocator_traits::pointer;
  using const_pointer = typename allocator_traits::const_pointer;

  template <class AA = A,
            typename std::enable_if<std::is_default_constructible<AA>::value,
                                    int>::type = 0>
  indirect() {
    static_assert(std::is_default_constructible<T>::value, "");
    p_ = construct_from(alloc_base::get());
  }

  indirect(std::allocator_arg_t, const A& alloc) : alloc_base(alloc) {
    static_assert(std::is_default_constructible<T>::value, "");
    p_ = construct_from(alloc_base::get());
  }

  template <
      class U, class... Us,
      typename std::enable_if<std::is_constructible<T, U&&, Us&&...>::value,
                              int>::type = 0,
      class AA = A,
      typename std::enable_if<std::is_default_constructible<AA>::value,
                              int>::type = 0,
      typename std::enable_if<
          !std::is_same<typename std::remove_const<
                            typename std::remove_reference<U>::type>::type,
                        indirect>::value,
          int>::type = 0,
      typename std::enable_if<
          !std::is_same<typename std::remove_const<
                            typename std::remove_reference<U>::type>::type,
                        std::allocator_arg_t>::value,
          int>::type = 0>
  explicit indirect(U&& u, Us&&... us) {
    p_ = construct_from(alloc_base::get(), std::forward<U>(u),
                        std::forward<Us>(us)...);
  }

  template <
      class U, class... Us,
      typename std::enable_if<std::is_constructible<T, U&&, Us&&...>::value,
                              int>::type = 0,
      typename std::enable_if<
          !std::is_same<typename std::remove_const<
                            typename std::remove_reference<U>::type>::type,
                        indirect>::value,
          int>::type = 0>
  indirect(std::allocator_arg_t, const A& alloc, U&& u, Us&&... us)
      : alloc_base(alloc) {
    p_ = construct_from(alloc_base::get(), std::forward<U>(u),
                        std::forward<Us>(us)...);
  }

  indirect(const indirect& other)
      : alloc_base(allocator_traits::select_on_container_copy_construction(
            other.alloc_base::get())) {
    static_assert(std::is_copy_constructible<T>::value, "");
    if (other.valueless_after_move()) {
      p_ = nullptr;
      return;
    }
    p_ = construct_from(alloc_base::get(), *other);
  }

  indirect(std::allocator_arg_t, const A& alloc, const indirect& other)
      : alloc_base(alloc) {
    static_assert(std::is_copy_constructible<T>::value, "");
    if (other.valueless_after_move()) {
      p_ = nullptr;
      return;
    }
    p_ = construct_from(alloc_base::get(), *other);
  }

  indirect(indirect&& other) noexcept
      : alloc_base(other.alloc_base::get()), p_(nullptr) {
    std::swap(p_, other.p_);
  }

  indirect(std::allocator_arg_t, const A& alloc, indirect&& other) noexcept
      : alloc_base(alloc), p_(nullptr) {
    std::swap(p_, other.p_);
  }

  ~indirect() { reset(); }

  indirect& operator=(const indirect& other) {
    if (this == &other) return *this;
    static_assert(std::is_copy_constructible<T>::value, "");
    if (allocator_traits::propagate_on_container_copy_assignment::value) {
      if (alloc_base::get() != other.alloc_base::get()) {
        reset();  // using current allocator.
        alloc_base::get() = other.alloc_base::get();
      }
    }
    if (other.valueless_after_move()) {
      reset();
      return *this;
    }
    if (alloc_base::get() == other.alloc_base::get() &&
        std::is_copy_assignable<T>::value && p_ != nullptr) {
      *p_ = *other.p_;
      return *this;
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    p_ = construct_from(alloc_base::get(), *other);
    return *this;
  }

  indirect& operator=(indirect&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value ||
      allocator_traits::is_always_equal::value) {
    if (this == &other) return *this;
    static_assert(std::is_copy_constructible<T>::value, "");

    if (allocator_traits::propagate_on_container_move_assignment::value) {
      if (alloc_base::get() != other.alloc_base::get()) {
        reset();  // using current allocator.
        alloc_base::get() = other.alloc_base::get();
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    if (other.valueless_after_move()) {
      return *this;
    }
    if (alloc_base::get() == other.alloc_base::get()) {
      std::swap(p_, other.p_);
    } else {
      p_ = construct_from(alloc_base::get(), std::move(*other));
    }
    return *this;
  }

  const T& operator*() const& noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  T& operator*() & noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  T&& operator*() && noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  const T&& operator*() const&& noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  const_pointer operator->() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  pointer operator->() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  bool valueless_after_move() const noexcept { return p_ == nullptr; }

  allocator_type get_allocator() const noexcept { return alloc_base::get(); }

  void swap(indirect& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    if (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects we can swap both.
      std::swap(alloc_base::get(), other.alloc_base::get());
      std::swap(p_, other.p_);
      return;
    } else /*  */ {
      if (alloc_base::get() == other.alloc_base::get()) {
        std::swap(p_, other.p_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend void swap(indirect& lhs,
                   indirect& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

  template <class U, class AA>
  friend bool operator==(const indirect<T, A>& lhs,
                         const indirect<U, AA>& rhs) {
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) {
      return lhs.valueless_after_move() && rhs.valueless_after_move();
    }
    return *lhs == *rhs;
  }

  template <class U, class AA>
  friend bool operator!=(const indirect<T, A>& lhs,
                         const indirect<U, AA>& rhs) {
    if (lhs.valueless_after_move() || rhs.valueless_after_move()) {
      return !(lhs.valueless_after_move() && rhs.valueless_after_move());
    }
    return *lhs != *rhs;
  }

  template <class U, class AA>
  friend bool operator<(const indirect<T, A>& lhs, const indirect<U, AA>& rhs) {
    if (lhs.valueless_after_move()) {
      return !rhs.valueless_after_move();
    }
    return !rhs.valueless_after_move() && *lhs < *rhs;
  }

  template <class U, class AA>
  friend bool operator>(const indirect<T, A>& lhs, const indirect<U, AA>& rhs) {
    if (rhs.valueless_after_move()) {
      return !lhs.valueless_after_move();
    }
    return !lhs.valueless_after_move() && *lhs > *rhs;
  }

  template <class U, class AA>
  friend bool operator<=(const indirect<T, A>& lhs,
                         const indirect<U, AA>& rhs) {
    if (lhs.valueless_after_move()) {
      return true;
    }
    return !rhs.valueless_after_move() && *lhs <= *rhs;
  }

  template <class U, class AA>
  friend bool operator>=(const indirect<T, A>& lhs,
                         const indirect<U, AA>& rhs) {
    if (rhs.valueless_after_move()) {
      return true;
    }

    return !lhs.valueless_after_move() && *lhs >= *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator==(const indirect<T, A>& lhs, const U& rhs) {
    if (lhs.valueless_after_move()) {
      return false;
    }
    return *lhs == rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator==(const U& lhs, const indirect<T, A>& rhs) {
    if (rhs.valueless_after_move()) {
      return false;
    }
    return lhs == *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator!=(const indirect<T, A>& lhs, const U& rhs) {
    if (lhs.valueless_after_move()) {
      return true;
    }
    return *lhs != rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator!=(const U& lhs, const indirect<T, A>& rhs) {
    if (rhs.valueless_after_move()) {
      return true;
    }
    return lhs != *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator<(const indirect<T, A>& lhs, const U& rhs) {
    if (lhs.valueless_after_move()) {
      return true;
    }
    return *lhs < rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator<(const U& lhs, const indirect<T, A>& rhs) {
    if (rhs.valueless_after_move()) {
      return false;
    }
    return lhs < *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator>(const indirect<T, A>& lhs, const U& rhs) {
    if (lhs.valueless_after_move()) {
      return false;
    }
    return *lhs > rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator>(const U& lhs, const indirect<T, A>& rhs) {
    if (rhs.valueless_after_move()) {
      return true;
    }
    return lhs > *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator<=(const indirect<T, A>& lhs, const U& rhs) {
    if (lhs.valueless_after_move()) {
      return true;
    }
    return *lhs <= rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator<=(const U& lhs, const indirect<T, A>& rhs) {
    if (rhs.valueless_after_move()) {
      return false;
    }
    return lhs <= *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator>=(const indirect<T, A>& lhs, const U& rhs) {
    if (lhs.valueless_after_move()) {
      return false;
    }
    return *lhs >= rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator>=(const U& lhs, const indirect<T, A>& rhs) {
    if (rhs.valueless_after_move()) {
      return true;
    }
    return lhs >= *rhs;
  }

 private:
  pointer p_;

  void reset() noexcept {
    if (p_ == nullptr) return;
    destroy_with(alloc_base::get(), p_);
    p_ = nullptr;
  }

  template <typename... Ts>
  static T* construct_from(A alloc, Ts&&... ts) {
    T* mem = allocator_traits::allocate(alloc, 1);
    try {
      allocator_traits::construct(alloc, mem, std::forward<Ts>(ts)...);
      return mem;
    } catch (...) {
      allocator_traits::deallocate(alloc, mem, 1);
      throw;
    }
  }

  static void destroy_with(A alloc, T* p) {
    allocator_traits::destroy(alloc, p);
    allocator_traits::deallocate(alloc, p, 1);
  }
};

}  // namespace xyz

template <class T, class Alloc>
struct std::hash<xyz::indirect<T, Alloc>> {
  std::size_t operator()(const xyz::indirect<T, Alloc>& key) const {
    if (key.valueless_after_move()) {
      return -1;  // Implementation defined value.
    }
    return std::hash<typename xyz::indirect<T, Alloc>::value_type>{}(*key);
  }
};

#endif  // XYZ_INDIRECT_H
