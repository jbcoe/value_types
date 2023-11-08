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
#include <type_traits>
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
  using allocator_traits = std::allocator_traits<A>;

 public:
  using value_type = T;
  using allocator_type = A;
  using pointer = typename allocator_traits::pointer;
  using const_pointer = typename allocator_traits::const_pointer;

<<<<<<< HEAD
  constexpr indirect()
    requires std::default_initializable<T>
  {
    auto mem = allocator_traits::allocate(alloc_, 1);
=======
  constexpr indirect() {
    static_assert(std::is_default_constructible_v<T>);
    T* mem = allocator_traits::allocate(alloc_, 1);
>>>>>>> main
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
    auto mem = allocator_traits::allocate(alloc_, 1);
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
    auto mem = allocator_traits::allocate(alloc_, 1);
    try {
      allocator_traits::construct(alloc_, mem, std::forward<Ts>(ts)...);
      p_ = mem;
    } catch (...) {
      allocator_traits::deallocate(alloc_, mem, 1);
      throw;
    }
  }

  constexpr indirect(const indirect& other)
      : alloc_(allocator_traits::select_on_container_copy_construction(
            other.alloc_)) {
    static_assert(std::is_copy_constructible_v<T>);
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    auto mem = allocator_traits::allocate(alloc_, 1);
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
      : alloc_(alloc) {
    static_assert(std::is_copy_constructible_v<T>);
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    auto mem = allocator_traits::allocate(alloc_, 1);
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

  constexpr indirect& operator=(const indirect& other) {
    static_assert(std::is_copy_constructible_v<T>);
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

  constexpr const_pointer operator->() const noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return p_;
  }

  constexpr pointer operator->() noexcept {
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
  pointer p_;

#if defined(_MSC_VER)
  [[msvc::no_unique_address]] A alloc_;
#else
  [[no_unique_address]] A alloc_;
#endif

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
  requires xyz::is_hashable<T>
struct std::hash<xyz::indirect<T, Alloc>> {
  constexpr std::size_t operator()(const xyz::indirect<T, Alloc>& key) const {
    return std::hash<typename xyz::indirect<T, Alloc>::value_type>{}(*key);
  }
};

#if (__cpp_lib_format >= 201907L)

namespace xyz {

template <class T, class charT>
concept is_formattable = requires(T t) { std::formatter<T, charT>{}; };

}  // namespace xyz

template <class T, class Alloc, class charT>
  requires xyz::is_formattable<T, charT>
struct std::formatter<xyz::indirect<T, Alloc>, charT>
    : std::formatter<T, charT> {
  template <class ParseContext>
  constexpr auto parse(ParseContext& ctx) -> typename ParseContext::iterator {
    return std::formatter<T, charT>::parse(ctx);
  }

  template <class FormatContext>
  auto format(xyz::indirect<T, Alloc> const& value, FormatContext& ctx) const ->
      typename FormatContext::iterator {
    return std::formatter<T, charT>::format(*value, ctx);
  }
};

#endif  // __cpp_lib_format >= 201907L

#endif  // XYZ_INDIRECT_H
