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

namespace xyz {
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

  indirect()
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
  indirect(std::in_place_t, Ts&&... ts)
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
  indirect(std::allocator_arg_t, const A& alloc, std::in_place_t, Ts&&... ts)
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

  indirect(const indirect& other)
    requires std::copy_constructible<T>
      : alloc_(other.alloc_) {
    assert(other.p_ != nullptr);
    T* mem = allocator_traits::allocate(alloc_, 1);
    try {
      allocator_traits::construct(alloc_, mem, *other);
      p_ = mem;
    } catch (...) {
      allocator_traits::deallocate(alloc_, mem, 1);
      throw;
    }
  }

  indirect(indirect&& other) noexcept
      : p_(nullptr), alloc_(std::move(other.alloc_)) {
    assert(other.p_ != nullptr);
    using std::swap;
    swap(p_, other.p_);
  }

  indirect(indirect&& other, const A& alloc) noexcept
      : p_(nullptr), alloc_(alloc) {
    assert(other.p_ != nullptr);
    using std::swap;
    swap(p_, other.p_);
  }

  ~indirect() { reset(); }

  indirect& operator=(const indirect& other)
    requires std::copy_constructible<T>
  {
    assert(other.p_ != nullptr);
    indirect tmp(other);
    swap(tmp);
    return *this;
  }

  indirect& operator=(indirect&& other) noexcept {
    assert(other.p_ != nullptr);
    reset();
    alloc_ = std::move(other.alloc_);
    p_ = std::exchange(other.p_, nullptr);
    return *this;
  }

  constexpr const T& operator*() const noexcept {
    assert(p_ != nullptr);
    return *p_;
  }

  constexpr T& operator*() noexcept {
    assert(p_ != nullptr);
    return *p_;
  }

  constexpr const T* operator->() const noexcept {
    assert(p_ != nullptr);
    return p_;
  }

  constexpr T* operator->() noexcept {
    assert(p_ != nullptr);
    return p_;
  }

  constexpr bool valueless_after_move() const noexcept { return p_ == nullptr; }
  
  constexpr void swap(indirect& other) noexcept {
    assert(p_ != nullptr);
    assert(other.p_ != nullptr);
    using std::swap;
    swap(p_, other.p_);
  }

  friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept {
    assert(lhs.p_ != nullptr);
    assert(rhs.p_ != nullptr);
    using std::swap;
    swap(lhs.p_, rhs.p_);
  }

  friend bool operator==(const indirect& lhs, const indirect& rhs) noexcept
    requires std::equality_comparable<T>
  {
    assert(lhs.p_ != nullptr);
    assert(rhs.p_ != nullptr);
    return *lhs == *rhs;
  }

  friend bool operator!=(const indirect& lhs, const indirect& rhs) noexcept
    requires std::equality_comparable<T>
  {
    assert(lhs.p_ != nullptr);
    assert(rhs.p_ != nullptr);
    return *lhs != *rhs;
  }

  friend auto operator<=>(const indirect& lhs, const indirect& rhs)
    requires std::three_way_comparable<T>
  {
    assert(lhs.p_ != nullptr);
    assert(rhs.p_ != nullptr);
    return *lhs <=> *rhs;
  }

 private:
  void reset() noexcept {
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
struct std::uses_allocator<xyz::indirect<T>, Alloc> : true_type {};

template <class T>
  requires xyz::is_hashable<T>
struct std::hash<xyz::indirect<T>> {
  constexpr std::size_t operator()(const xyz::indirect<T>& key) const {
    return std::hash<typename xyz::indirect<T>::value_type>{}(*key);
  }
};

#endif  // XYZ_INDIRECT_H
