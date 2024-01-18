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

  explicit constexpr indirect()
    requires std::is_default_constructible_v<A>
  {
    static_assert(std::is_default_constructible_v<T>);
    p_ = construct_from(alloc_);
  }

  explicit constexpr indirect(std::allocator_arg_t, const A& alloc)
      : alloc_(alloc) {
    static_assert(std::is_default_constructible_v<T>);
    p_ = construct_from(alloc_);
  }

  template <class U, class... Us>
  explicit constexpr indirect(U&& u, Us&&... us)
    requires(std::constructible_from<T, U &&, Us && ...> &&
             std::is_default_constructible_v<A> &&
             !std::is_same_v<std::remove_cvref_t<U>, indirect> &&
             !std::is_same_v<std::remove_cvref_t<U>, std::allocator_arg_t>)
  {
    p_ = construct_from(alloc_, std::forward<U>(u), std::forward<Us>(us)...);
  }

  template <class U, class... Us>
  explicit constexpr indirect(std::allocator_arg_t, const A& alloc, U&& u,
                              Us&&... us)
    requires(std::constructible_from<T, U &&, Us && ...> &&
             !std::is_same_v<std::remove_cvref_t<U>, indirect>)
      : alloc_(alloc) {
    p_ = construct_from(alloc_, std::forward<U>(u), std::forward<Us>(us)...);
  }

  constexpr indirect(const indirect& other)
      : alloc_(allocator_traits::select_on_container_copy_construction(
            other.alloc_)) {
    static_assert(std::is_copy_constructible_v<T>);
    if (other.valueless_after_move()) {
      p_ = nullptr;
      return;
    }
    p_ = construct_from(alloc_, *other);
  }

  constexpr indirect(std::allocator_arg_t, const A& alloc,
                     const indirect& other)
      : alloc_(alloc) {
    static_assert(std::is_copy_constructible_v<T>);
    if (other.valueless_after_move()) {
      p_ = nullptr;
      return;
    }
    p_ = construct_from(alloc_, *other);
  }

  constexpr indirect(indirect&& other) noexcept
      : p_(nullptr), alloc_(other.alloc_) {
    std::swap(p_, other.p_);
  }

  constexpr indirect(
      std::allocator_arg_t, const A& alloc,
      indirect&& other) noexcept(allocator_traits::is_always_equal::value)
      : p_(nullptr), alloc_(alloc) {
    std::swap(p_, other.p_);
  }

  constexpr ~indirect() { reset(); }

  constexpr indirect& operator=(const indirect& other) {
    if (this == &other) return *this;
    static_assert(std::is_copy_constructible_v<T>);
    if constexpr (allocator_traits::propagate_on_container_copy_assignment::
                      value) {
      if (alloc_ != other.alloc_) {
        reset();  // using current allocator.
        alloc_ = other.alloc_;
      }
    }
    if (other.valueless_after_move()) {
      reset();
      return *this;
    }
    if (alloc_ == other.alloc_ && std::is_copy_assignable_v<T> &&
        p_ != nullptr) {
      *p_ = *other.p_;
      return *this;
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    p_ = construct_from(alloc_, *other);
    return *this;
  }

  constexpr indirect& operator=(indirect&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value ||
      allocator_traits::is_always_equal::value) {
    if (this == &other) return *this;
    static_assert(std::is_copy_constructible_v<T>);

    if constexpr (allocator_traits::propagate_on_container_move_assignment::
                      value) {
      if (alloc_ != other.alloc_) {
        reset();  // using current allocator.
        alloc_ = other.alloc_;
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    if (other.valueless_after_move()) {
      return *this;
    }
    if (alloc_ == other.alloc_) {
      std::swap(p_, other.p_);
    } else {
      p_ = construct_from(alloc_, std::move(*other));
    }
    return *this;
  }

  constexpr const T& operator*() const& noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  constexpr T& operator*() & noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  constexpr T&& operator*() && noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  constexpr const T&& operator*() const&& noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  constexpr const_pointer operator->() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  constexpr pointer operator->() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  constexpr bool valueless_after_move() const noexcept { return p_ == nullptr; }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  constexpr void swap(indirect& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    if constexpr (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects, we can swap both.
      std::swap(alloc_, other.alloc_);
      std::swap(p_, other.p_);
      return;
    } else /* constexpr */ {
      if (alloc_ == other.alloc_) {
        std::swap(p_, other.p_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend constexpr void swap(indirect& lhs,
                             indirect& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

  template <class U, class AA>
  friend constexpr bool operator==(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs == *rhs)) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs == *rhs;
  }

  template <class U, class AA>
  friend constexpr auto operator<=>(
      const indirect<T, A>& lhs,
      const indirect<U, AA>& rhs) noexcept(noexcept(*lhs <=> *rhs)) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs <=> *rhs;
  }

  template <class U>
  friend constexpr bool operator==(const indirect<T, A>& lhs,
                                   const U& rhs) noexcept(noexcept(*lhs == rhs))
    requires(!is_indirect_v<U>)
  {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs == rhs;
  }

  template <class U>
  friend constexpr bool operator==(
      const U& lhs, const indirect<T, A>& rhs) noexcept(noexcept(lhs == *rhs))
    requires(!is_indirect_v<U>)
  {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs == *rhs;
  }

  template <class U>
  friend constexpr auto operator<=>(const indirect<T, A>& lhs,
                                    const U& rhs) noexcept(noexcept(*lhs <=>
                                                                    rhs))
    requires(!is_indirect_v<U>)
  {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs <=> rhs;
  }

  template <class U>
  friend constexpr auto operator<=>(
      const U& lhs, const indirect<T, A>& rhs) noexcept(noexcept(lhs <=> *rhs))
    requires(!is_indirect_v<U>)
  {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs <=> *rhs;
  }

 private:
  pointer p_;

#if defined(_MSC_VER)
  // https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#msvc-extensions-and-abi
  [[msvc::no_unique_address]] A alloc_;
#else
  [[no_unique_address]] A alloc_;
#endif

  constexpr void reset() noexcept {
    if (p_ == nullptr) return;
    destroy_with(alloc_, p_);
    p_ = nullptr;
  }

  template <typename... Ts>
  constexpr static pointer construct_from(A alloc, Ts&&... ts) {
    pointer mem = allocator_traits::allocate(alloc, 1);
    try {
      allocator_traits::construct(alloc, std::to_address(mem),
                                  std::forward<Ts>(ts)...);
      return mem;
    } catch (...) {
      allocator_traits::deallocate(alloc, mem, 1);
      throw;
    }
  }

  constexpr static void destroy_with(A alloc, pointer p) {
    allocator_traits::destroy(alloc, std::to_address(p));
    allocator_traits::deallocate(alloc, p, 1);
  }
};

template <class T>
concept is_hashable = requires(T t) { std::hash<T>{}(t); };

template <typename Value>
indirect(Value) -> indirect<Value>;

template <typename Alloc, typename Value>
indirect(std::allocator_arg_t, Alloc, Value) -> indirect<
    Value, typename std::allocator_traits<Alloc>::template rebind_alloc<Value>>;

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
