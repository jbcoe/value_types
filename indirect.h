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
#include <compare>
#include <concepts>
#include <memory>
#include <utility>

#if __has_include(<format>)
#include <format>
#endif

namespace xyz {

template <class T, class A>
class indirect;

template <class>
inline constexpr bool is_indirect_v = false;

template <class T, class A>
inline constexpr bool is_indirect_v<indirect<T, A>> = true;

template <class T, class A = std::allocator<T>>
class indirect {
  T* p_;

#if defined(_MSC_VER)
  [[msvc::no_unique_address]] A alloc_;
#else
  [[no_unique_address]] A alloc_;
#endif

  using allocator_traits = std::allocator_traits<A>;

 public:
  using value_type = T;
  using allocator_type = A;

  constexpr indirect()
    requires std::default_initializable<T>
  {
    T* mem = allocator_traits::allocate(alloc_, 1);
    try {
      allocator_traits::construct(alloc_, mem);
      p_ = mem;
    } catch (...) {
      allocator_traits::deallocate(alloc_, mem, 1);
      throw;
    }
  }

  template <class... Ts>
  explicit constexpr indirect(std::in_place_t, Ts&&... ts)
    requires std::constructible_from<T, Ts&&...>
  {
    T* mem = allocator_traits::allocate(alloc_, 1);
    try {
      allocator_traits::construct(alloc_, mem, std::forward<Ts>(ts)...);
      p_ = mem;
    } catch (...) {
      allocator_traits::deallocate(alloc_, mem, 1);
      throw;
    }
  }

  template <class... Ts>
  constexpr indirect(std::allocator_arg_t, const A& alloc, std::in_place_t,
                     Ts&&... ts)
    requires std::constructible_from<T, Ts&&...>
      : alloc_(alloc) {
    T* mem = allocator_traits::allocate(alloc_, 1);
    try {
      allocator_traits::construct(alloc_, mem, std::forward<Ts>(ts)...);
      p_ = mem;
    } catch (...) {
      allocator_traits::deallocate(alloc_, mem, 1);
      throw;
    }
  }

  constexpr indirect(const indirect& other)
    requires std::copy_constructible<T>
      : alloc_(allocator_traits::select_on_container_copy_construction(
            other.alloc_)) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    T* mem = allocator_traits::allocate(alloc_, 1);
    try {
      allocator_traits::construct(alloc_, mem, *other);
      p_ = mem;
    } catch (...) {
      allocator_traits::deallocate(alloc_, mem, 1);
      throw;
    }
  }

  constexpr indirect(std::allocator_arg_t, const A& alloc,
                     const indirect& other)
    requires std::copy_constructible<T>
      : alloc_(alloc) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    T* mem = allocator_traits::allocate(alloc_, 1);
    try {
      allocator_traits::construct(alloc_, mem, *other);
      p_ = mem;
    } catch (...) {
      allocator_traits::deallocate(alloc_, mem, 1);
      throw;
    }
  }

  constexpr indirect(indirect&& other) noexcept
      : p_(nullptr), alloc_(other.alloc_) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    std::swap(p_, other.p_);
  }

  constexpr indirect(std::allocator_arg_t, const A& alloc,
                     indirect&& other) noexcept
      : p_(nullptr), alloc_(alloc) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    std::swap(p_, other.p_);
  }

  constexpr ~indirect() { reset(); }

  constexpr indirect& operator=(const indirect& other)
    requires std::copy_constructible<T>
  {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    if (this != &other) {
      if constexpr (std::is_copy_assignable_v<T>) {
        if (p_ == nullptr) {
          T* mem = allocator_traits::allocate(alloc_, 1);
          try {
            allocator_traits::construct(alloc_, mem, *other);
            p_ = mem;
          } catch (...) {
            allocator_traits::deallocate(alloc_, mem, 1);
            throw;
          }
        } else {
          *p_ = *other.p_;
        }
      } else {
        indirect tmp(other);
        this->swap(tmp);
      }
      if constexpr (allocator_traits::propagate_on_container_copy_assignment::
                        value) {
        alloc_ = other.alloc_;
      }
    }
    return *this;
  }

  constexpr indirect& operator=(indirect&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value ||
      allocator_traits::is_always_equal::value) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    if (this != &other) {
      reset();
      if constexpr (allocator_traits::propagate_on_container_move_assignment::
                        value) {
        alloc_ = other.alloc_;
        p_ = std::exchange(other.p_, nullptr);
      } else {
        if (alloc_ == other.alloc_) {
          p_ = std::exchange(other.p_, nullptr);
        } else {
          T* mem = allocator_traits::allocate(alloc_, 1);
          try {
            allocator_traits::construct(alloc_, mem, *other);
            p_ = mem;
            other.reset();
          } catch (...) {
            allocator_traits::deallocate(alloc_, mem, 1);
            throw;
          }
        }
      }
    }
    return *this;
  }

  constexpr const T& operator*() const noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return *p_;
  }

  constexpr T& operator*() noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return *p_;
  }

  constexpr const T* operator->() const noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return p_;
  }

  constexpr T* operator->() noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return p_;
  }

  constexpr bool valueless_after_move() const noexcept { return p_ == nullptr; }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  constexpr void swap(indirect& other) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    assert(p_ != nullptr);        // LCOV_EXCL_LINE
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    std::swap(p_, other.p_);
    if constexpr (allocator_traits::propagate_on_container_swap::value) {
      std::swap(alloc_, other.alloc_);
    }
  }

  friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    lhs.swap(rhs);
  }

  template <class U, class AA>
  friend constexpr bool operator==(const indirect<T, A>& lhs,
                                   const indirect<U, AA>& rhs)
    requires std::equality_comparable_with<T, U>
  {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs == *rhs;
  }

  template <class U, class AA>
  friend constexpr bool operator!=(const indirect<T, A>& lhs,
                                   const indirect<U, AA>& rhs)
    requires std::equality_comparable_with<T, U>
  {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs != *rhs;
  }

  template <class U, class AA>
  friend constexpr auto operator<=>(const indirect<T, A>& lhs,
                                    const indirect<U, AA>& rhs)
    requires std::three_way_comparable_with<T, U>
  {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs <=> *rhs;
  }

  template <class U>
  friend constexpr bool operator==(const indirect<T, A>& lhs, const U& rhs)
    requires(!is_indirect_v<U>)
  {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs == rhs;
  }

  template <class U>
  friend constexpr bool operator==(const U& lhs, const indirect<T, A>& rhs)
    requires(!is_indirect_v<U>)
  {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs == *rhs;
  }

  template <class U>
  friend constexpr bool operator!=(const indirect<T, A>& lhs, const U& rhs)
    requires(!is_indirect_v<U>)
  {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs != rhs;
  }

  template <class U>
  friend constexpr bool operator!=(const U& lhs, const indirect<T, A>& rhs)
    requires(!is_indirect_v<U>)
  {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs != *rhs;
  }

  template <class U>
  friend constexpr auto operator<=>(const indirect<T, A>& lhs, const U& rhs)
    requires(!is_indirect_v<U>)
  {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs <=> rhs;
  }

  template <class U>
  friend constexpr auto operator<=>(const U& lhs, const indirect<T, A>& rhs)
    requires(!is_indirect_v<U>)
  {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs <=> *rhs;
  }

 private:
  constexpr void reset() noexcept {
    if (p_ == nullptr) return;
    allocator_traits::destroy(alloc_, p_);
    allocator_traits::deallocate(alloc_, p_, 1);
    p_ = nullptr;
  }
};

template <class T>
concept is_hashable = requires(T t) { std::hash<T>{}(t); };

}  // namespace xyz

template <class T, class Alloc>
struct std::uses_allocator<xyz::indirect<T, Alloc>, Alloc> : true_type {};

template <class T, class Alloc>
  requires xyz::is_hashable<T>
struct std::hash<xyz::indirect<T, Alloc>> {
  constexpr std::size_t operator()(const xyz::indirect<T, Alloc>& key) const {
    return std::hash<typename xyz::indirect<T, Alloc>::value_type>{}(*key);
  }
};

#if (__cpp_lib_format >= 201907L)

namespace xyz {

template <class T>
concept is_formattable = requires(T t) { std::formatter<T, char>{}; };
    
}

template <class T, class Alloc, class charT>
  requires xyz::is_formattable<T>
struct std::formatter<xyz::indirect<T, Alloc>, charT> : std::formatter<T> {
  constexpr auto parse(format_parse_context& ctx)
      -> format_parse_context::iterator {
    return std::formatter<T>::parse(ctx);
  }

  template<class FormatContext>
  auto format(xyz::indirect<T, Alloc> const& value, FormatContext& ctx) const
      -> typename FormatContext::iterator {
//    using namespace std::literals;
//    return std::format_to(ctx.out(), "{}", *value);
    return std::formatter<T>::format(*value, ctx);
  }
};

#endif // __cpp_lib_format >= 201907L

#endif  // XYZ_INDIRECT_H
