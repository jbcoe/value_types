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

#define IMPL_OPTION 2

namespace xyz {

namespace detail {

template <class T, class A>
struct control_block {
  control_block() {}
  template<typename U> control_block(U& example) {
    offset = reinterpret_cast<char*>(static_cast<T*>(&example)) - reinterpret_cast<char*>(&example);

    destroy = [](A& alloc, void* src) {
      U* u = static_cast<U*>(src);
      using u_allocator = typename std::allocator_traits<A>::template rebind_alloc<U>;
      using u_alloc_traits = std::allocator_traits<u_allocator>;

      u_allocator u_alloc(alloc);
      u_alloc_traits::destroy(u_alloc, u);
      u_alloc_traits::deallocate(u_alloc, u, 1);
    };

    clone = [](A& alloc, const void* src)->T* {
      const U* u = static_cast<const U*>(src);
      using u_allocator = typename std::allocator_traits<A>::template rebind_alloc<U>;
      using u_alloc_traits = std::allocator_traits<u_allocator>;

      u_allocator u_alloc(alloc);
      auto* mem = u_alloc_traits::allocate(u_alloc, 1);
      try {
        u_alloc_traits::construct(u_alloc, mem, *u);
        return mem;
      } catch (...) {
        u_alloc_traits::deallocate(u_alloc, mem, 1);
        throw;
      }
    };
  }

  size_t offset;            // Where inside U does T start. This works for virtual bases as long as we know that the U we get as example is _exactly_ an U, not a subclass.
  void (*destroy)(A& alloc, void* src);
  T* (*clone)(A& alloc, const void* src);
};

#if IMPL_OPTION == 1

#error Option 1 does not work as the ctor does not have a real example object to provide.

// Just initiate each (used) template variable instance. This is susceptible to static initialization order issues
template<class T, class U, class A> inline control_block<T, A> s_control_block = control_block<T, A>(std::in_place_type<U>);

template<class T, class U, class A> control_block<T, A>* init_control_block()
{
  return &s_control_block<T, U, A>;
}

#elif IMPL_OPTION == 2

// Put the control block inside the function to make sure it is initialized as early as needed. Properly thread-safe but the
// overhead of checking the "initialized" flag for each call is a drawback.
template<class T, class A, class U> control_block<T, A>* init_control_block(U& example)
{
  static control_block<T, A> instance(example);
  return &instance;
}

#elif IMPL_OPTION == 3

template<class T, class A, class U> inline control_block<T, A> s_control_block;

// Set up the control block each time to make sure it is set early enough without the function-static variable overhead.
// Thread safety is so-so as multiple threads may write the same values to the function pointers at the same time, which could be
// bad on some architectures, possibly.
template<class T, class A, class U> control_block<T, A>* init_control_block(U& example)
{
  s_control_block<T, A, U> = control_block<T, A>(example);
  return &s_control_block<T, A, U>;
}

#else
#error Unknown IMPL_OPTION value
#endif

}  // namespace detail


template <class T, class A = std::allocator<T>>
class polymorphic {
  T* p_ = nullptr;
  detail::control_block<T, A>* cb_ = nullptr;

#if defined(_MSC_VER)
  [[msvc::no_unique_address]] A alloc_;
#else
  [[no_unique_address]] A alloc_;
#endif

public:
  using value_type = T;
  using allocator_type = A;

  polymorphic()
    requires std::default_initializable<T>
  {
    auto* mem = std::allocator_traits<A>::allocate(alloc_, 1);
    try {
      std::allocator_traits<A>::construct(alloc_, mem);
      p_ = mem;
      cb_ = detail::init_control_block<T, A>(*mem);
    } catch (...) {
      std::allocator_traits<A>::deallocate(alloc_, mem, 1);
      throw;
    }
  }

  template <class U, class... Ts>
  explicit polymorphic(std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> &&
             (std::derived_from<U, T> || std::same_as<U, T>)
  {
    using u_allocator = typename std::allocator_traits<A>::template rebind_alloc<U>;
    using u_traits = std::allocator_traits<u_allocator>;
    u_allocator u_alloc(alloc_);
    auto* mem = u_traits::allocate(u_alloc, 1);
    try {
      u_traits::construct(u_alloc, mem, std::forward<Ts>(ts)...);
      p_ = mem;
      cb_ = detail::init_control_block<T, A>(*mem);
    } catch (...) {
      u_traits::deallocate(u_alloc, mem, 1);
      throw;
    }
  }

  template <class U, class... Ts>
  polymorphic(std::allocator_arg_t, const A& alloc, std::in_place_type_t<U>,
              Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> &&
             (std::derived_from<U, T> || std::same_as<U, T>)
      : alloc_(alloc) {
    using u_allocator = typename std::allocator_traits<A>::template rebind_alloc<U>;
    using u_traits = std::allocator_traits<u_allocator>;
    u_allocator u_alloc(alloc_);
    auto* mem = u_traits::allocate(u_alloc, 1);
    try {
      u_traits::construct(u_alloc, mem, std::forward<Ts>(ts)...);
      p_ = mem;
      cb_ = detail::init_control_block<T, A>(*mem);
    } catch (...) {
      u_traits::deallocate(u_alloc, mem, 1);
      throw;
    }
  }

  polymorphic(const polymorphic& other) : alloc_(other.alloc_) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_;
    p_ = cb_->clone(alloc_, reinterpret_cast<const char*>(other.p_) - other.cb_->offset);
  }

  polymorphic(std::allocator_arg_t, const A& alloc, const polymorphic& other) : alloc_(alloc) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_;
    p_ = cb_->clone(alloc_, reinterpret_cast<const char*>(other.p_) - other.cb_->offset);
  }

  polymorphic(polymorphic&& other) noexcept : alloc_(std::move(other.alloc_)) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_;
    p_ = std::exchange(other.p_, nullptr);
  }

  polymorphic(std::allocator_arg_t, const A& alloc, polymorphic&& other) noexcept : alloc_(alloc){
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_;
    p_ = std::exchange(other.p_, nullptr);
  }

  ~polymorphic() { reset(); }

  polymorphic& operator=(const polymorphic& other) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    polymorphic tmp(other);
    swap(tmp);
    return *this;
  }

  polymorphic& operator=(polymorphic&& other) noexcept {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    reset();
    alloc_ = std::move(other.alloc_);
    cb_ = other.cb_;
    p_ = std::exchange(other.p_, nullptr);
    return *this;
  }

  constexpr T* operator->() noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return p_;
  }

  constexpr const T* operator->() const noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return p_;
  }

  constexpr T& operator*() noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return *p_;
  }

  constexpr const T& operator*() const noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return *p_;
  }

  constexpr bool valueless_after_move() const noexcept {
    return p_ == nullptr;
  }

  constexpr void swap(polymorphic& other) noexcept {
    // NOTE: I don't think these asserts should be here. Even if one is valueless it should be swappable
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    using std::swap;
    swap(cb_, other.cb_);
    swap(p_, other.p_);
  }

  friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept {
    // NOTE: I don't think these asserts should be here. Even if one is valueless it should be swappable
    assert(lhs.p_ != nullptr);  // LCOV_EXCL_LINE
    assert(rhs.p_ != nullptr);  // LCOV_EXCL_LINE
    using std::swap;
    swap(lhs.cb_, rhs.cb_);
    swap(lhs.p_, rhs.p_);
  }

 private:
  void reset() noexcept {
    if (p_ != nullptr) {
      cb_->destroy(alloc_, reinterpret_cast<char*>(p_) - cb_->offset);
      p_ = nullptr;
    }
  }
};

}  // namespace xyz

template <class T, class Alloc>
struct std::uses_allocator<xyz::polymorphic<T, Alloc>, Alloc> : true_type {};

#endif  // XYZ_POLYMORPHIC_H_
