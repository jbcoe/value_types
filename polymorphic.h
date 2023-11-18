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
#include <concepts>
#include <memory>
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

namespace detail {
template <class T, class A>
struct control_block {
  using allocator_traits = std::allocator_traits<A>;

  typename allocator_traits::pointer p_;

  virtual constexpr ~control_block() = default;
  virtual constexpr void destroy(A& alloc) = 0;
  virtual constexpr control_block<T, A>* clone(A& alloc) = 0;
};

template <class T, class U, class A>
class direct_control_block final : public control_block<T, A> {
  union storage_t {
    char c_;
    U u_;
    constexpr storage_t() {}
    constexpr ~storage_t() {}
  } storage_;

 public:
  constexpr ~direct_control_block() override = default;

  template <class... Ts>
  constexpr direct_control_block(std::allocator_arg_t, const A& alloc,
                                 Ts&&... ts) {
    using u_allocator =
        typename std::allocator_traits<A>::template rebind_alloc<U>;
    u_allocator u_alloc(alloc);
    using u_alloc_traits = std::allocator_traits<u_allocator>;
    u_alloc_traits::construct(u_alloc, &storage_.u_, std::forward<Ts>(ts)...);
    control_block<T, A>::p_ = &storage_.u_;
  }

  constexpr control_block<T, A>* clone(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    auto mem = cb_alloc_traits::allocate(cb_alloc, 1);
    try {
      cb_alloc_traits::construct(cb_alloc, mem, std::allocator_arg, alloc,
                                 storage_.u_);
      return mem;
    } catch (...) {
      cb_alloc_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  constexpr void destroy(A& alloc) override {
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
class polymorphic {
  using cblock_t = detail::control_block<T, A>;
  cblock_t* cb_;

#if defined(_MSC_VER)
  [[msvc::no_unique_address]] A alloc_;
#else
  [[no_unique_address]] A alloc_;
#endif

  using allocator_traits = std::allocator_traits<A>;

 public:
  using value_type = T;
  using allocator_type = A;
  using pointer = typename allocator_traits::pointer;
  using const_pointer = typename allocator_traits::const_pointer;

  constexpr polymorphic() {
    static_assert(std::is_default_constructible_v<T>);
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, T, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::allocator_arg, alloc_);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  constexpr polymorphic(std::allocator_arg_t, const A& alloc) : alloc_(alloc) {
    static_assert(std::is_default_constructible_v<T>);
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, T, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::allocator_arg, alloc_);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  template <class U, class... Ts>
  explicit constexpr polymorphic(std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
  {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::allocator_arg, alloc_,
                           std::forward<Ts>(ts)...);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  template <class U, class... Ts>
  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
      : alloc_(alloc) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::allocator_arg, alloc_,
                           std::forward<Ts>(ts)...);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  constexpr polymorphic(const polymorphic& other)
      : alloc_(allocator_traits::select_on_container_copy_construction(
            other.alloc_)) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_->clone(alloc_);
  }

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        const polymorphic& other)
      : alloc_(alloc) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_->clone(alloc_);
  }

  constexpr polymorphic(polymorphic&& other) noexcept : alloc_(other.alloc_) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = std::exchange(other.cb_, nullptr);
  }

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        polymorphic&& other) noexcept
      : alloc_(alloc) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = std::exchange(other.cb_, nullptr);
  }

  constexpr ~polymorphic() { reset(); }

  constexpr polymorphic& operator=(const polymorphic& other) {
    if (this == &other) return *this;
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    if constexpr (allocator_traits::propagate_on_container_copy_assignment::
                      value) {
      if (alloc_ != other.alloc_) {
        reset();  // using current allocator.
        alloc_ = other.alloc_;
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    cb_ = other.cb_->clone(alloc_);
    return *this;
  }

  constexpr polymorphic& operator=(polymorphic&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value ||
      allocator_traits::is_always_equal::value) {
    if (this == &other) return *this;
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    if constexpr (allocator_traits::propagate_on_container_move_assignment::
                      value) {
      if (alloc_ != other.alloc_) {
        reset();  // using current allocator.
        alloc_ = other.alloc_;
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    if (alloc_ == other.alloc_) {
      std::swap(cb_, other.cb_);
    } else {
      cb_ = other.cb_->clone(alloc_);
    }
    return *this;
  }

  constexpr pointer operator->() noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  constexpr const_pointer operator->() const noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  constexpr T& operator*() noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  constexpr const T& operator*() const noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  constexpr bool valueless_after_move() const noexcept {
    return cb_ == nullptr;
  }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  constexpr void swap(polymorphic& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    assert(cb_ != nullptr);        // LCOV_EXCL_LINE
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    if constexpr (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects, we can swap both.
      std::swap(alloc_, other.alloc_);
      std::swap(cb_, other.cb_);
      return;
    } else /* constexpr */ {
      if (alloc_ == other.alloc_) {
        std::swap(cb_, other.cb_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept(
      noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

 private:
  constexpr void reset() noexcept {
    if (cb_ != nullptr) {
      cb_->destroy(alloc_);
      cb_ = nullptr;
    }
  }
};  // namespace xyz

}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_
