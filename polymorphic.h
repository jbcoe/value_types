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

#ifdef XYZ_POLYMORPHIC_USES_EXPERIMENTAL_INLINE_VTABLE
#include "experimental/polymorphic_inline_vtable.h"
#endif  // XYZ_POLYMORPHIC_USES_EXPERIMENTAL_INLINE_VTABLE

#ifdef XYZ_POLYMORPHIC_USES_EXPERIMENTAL_SMALL_BUFFER_OPTIMIZATION
#include "experimental/polymorphic_sbo.h"
#endif  // XYZ_POLYMORPHIC_USES_EXPERIMENTAL_SMALL_BUFFER_OPTIMIZATION

#ifndef XYZ_POLYMORPHIC_H_
#define XYZ_POLYMORPHIC_H_

#include <cassert>
#include <concepts>
#include <memory>
#include <optional>
#include <utility>

namespace xyz {

#ifndef XYZ_GUARD_DEFINED
#define XYZ_GUARD_DEFINED
template <typename F>
class guard {
  std::optional<F> f_;

 public:
  constexpr guard(F f) : f_(f) {}
  constexpr ~guard() {
    if (f_) {
      (*f_)();
    }
  }
  constexpr void reset() { f_.reset(); }
};

template <typename F>
guard(F) -> guard<F>;
#endif  // XYZ_GUARD_DEFINED

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

struct NoPolymorphicSBO {};

namespace detail {
template <class T, class A>
class control_block {
 public:
  using allocator_traits = std::allocator_traits<A>;

  constexpr typename allocator_traits::pointer ptr() { return p_; }
  constexpr typename allocator_traits::const_pointer ptr() const { return p_; }

  virtual constexpr ~control_block() = default;
  virtual constexpr void destroy(A& alloc) = 0;
  virtual constexpr control_block<T, A>* clone(A& alloc) = 0;

 protected:
  typename allocator_traits::pointer p_;
};

template <class T, class U, class A>
class direct_control_block final : public control_block<T, A> {
  union storage_t {
    char c_;
    U u_;
    constexpr storage_t() {}
    constexpr ~storage_t() {}
  } storage_;
  [[no_unique_address]] A alloc_;

  using u_allocator = typename std::allocator_traits<A>::template rebind_alloc<
      direct_control_block<T, U, A>>;
  using u_alloc_traits = std::allocator_traits<u_allocator>;

 public:
  using allocator_type = A;

  allocator_type get_allocator() const { return alloc_; }

  constexpr ~direct_control_block() override {
    u_allocator u_alloc(alloc_);
    u_alloc_traits::destroy(u_alloc, &storage_.u_);
  }

  template <class... Ts>
  constexpr direct_control_block(std::allocator_arg_t, const A& alloc,
                                 Ts&&... ts)
      : alloc_(alloc) {
    u_allocator u_alloc(alloc_);
    u_alloc_traits::construct(u_alloc, &storage_.u_, std::forward<Ts>(ts)...);
    control_block<T, A>::p_ = &storage_.u_;
  }

  template <class... Ts>
  constexpr direct_control_block(Ts&&... ts)
    requires(std::is_constructible_v<U, Ts...> &&
             std::is_default_constructible_v<A>)
  {
    u_allocator u_alloc(alloc_);
    u_alloc_traits::construct(u_alloc, &storage_.u_, std::forward<Ts>(ts)...);
    control_block<T, A>::p_ = &storage_.u_;
  }

  constexpr control_block<T, A>* clone(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    auto mem = cb_alloc_traits::allocate(cb_alloc, 1);
    guard mem_guard([&]() { cb_alloc_traits::deallocate(cb_alloc, mem, 1); });
    cb_alloc_traits::construct(cb_alloc, mem, storage_.u_);
    mem_guard.reset();
    return mem;
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
    using cb_allocator_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_allocator_traits::allocate(cb_alloc, 1);
    guard mem_guard(
        [&]() { cb_allocator_traits::deallocate(cb_alloc, mem, 1); });
    cb_allocator_traits::construct(cb_alloc, mem);
    cb_ = mem;
    mem_guard.reset();
  }

  constexpr polymorphic(std::allocator_arg_t, const A& alloc) : alloc_(alloc) {
    static_assert(std::is_default_constructible_v<T>, "");
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, T, A>>;
    using cb_allocator_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_allocator_traits::allocate(cb_alloc, 1);
    guard mem_guard(
        [&]() { cb_allocator_traits::deallocate(cb_alloc, mem, 1); });
    cb_allocator_traits::construct(cb_alloc, mem);
    cb_ = mem;
    mem_guard.reset();
  }

  template <class U, class... Ts>
  explicit constexpr polymorphic(std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
  {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_allocator_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_allocator_traits::allocate(cb_alloc, 1);
    guard mem_guard(
        [&]() { cb_allocator_traits::deallocate(cb_alloc, mem, 1); });
    cb_allocator_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
    cb_ = mem;
    mem_guard.reset();
  }

  template <class U, class... Ts>
  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> &&
             (std::derived_from<U, T> || std::same_as<U, T>)
      : alloc_(alloc) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_allocator_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_allocator_traits::allocate(cb_alloc, 1);
    guard mem_guard(
        [&]() { cb_allocator_traits::deallocate(cb_alloc, mem, 1); });
    cb_allocator_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
    cb_ = mem;
    mem_guard.reset();
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
    return cb_->ptr();
  }

  constexpr const_pointer operator->() const noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return cb_->ptr();
  }

  constexpr T& operator*() noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return *cb_->ptr();
  }

  constexpr const T& operator*() const noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return *cb_->ptr();
  }

  constexpr bool valueless_after_move() const noexcept {
    return cb_ == nullptr;
  }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  constexpr void swap(polymorphic& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE

    if constexpr (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects we can swap both.
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
