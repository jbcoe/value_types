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
#include <variant>

namespace xyz {

namespace detail {

template <class T, class A>
struct control_block {
  using allocator_type = A;

  T* p_;
};

template <class T, class U, class A>
class direct_control_block : public control_block<T, A> {
  U u_;

 public:
  using allocator_type = A;

   template <typename... Ts>
   constexpr direct_control_block(Ts&&... ts) : u_(std::forward<Ts>(ts)...) {}
};

}  // namespace detail

constexpr const size_t XYZ_POLYMORPHIC_BUFFER_SIZE = 128;

template <class T, class A = std::allocator<T>>
class polymorphic {
  std::variant<std::monostate,
               std::array<std::byte, XYZ_POLYMORPHIC_BUFFER_SIZE>,
               detail::control_block<T, A>*>
      storage_;

#if defined(_MSC_VER)
  [[msvc::no_unique_address]] A alloc_;
#else
  [[no_unique_address]] A alloc_;
#endif

  using allocator_traits = std::allocator_traits<A>;

 public:
  using value_type = T;
  using allocator_type = A;

  constexpr polymorphic()
    requires std::default_initializable<T>
  {
    if constexpr (sizeof(T) <= XYZ_POLYMORPHIC_BUFFER_SIZE &&
                  std::is_trivially_copyable<T>::value) {
      new (&std::get<1>(storage_)) T();
    } else {
      using cb_allocator = typename allocator_traits::template rebind_alloc<
          detail::direct_control_block<T, T, A>>;
      cb_allocator cb_alloc(alloc_);
      using cb_alloc_traits = std::allocator_traits<cb_allocator>;
      auto* mem = cb_alloc_traits::allocate(cb_alloc, 1);
      try {
        cb_alloc_traits::construct(cb_alloc, mem);
        storage_.template emplace<2>(mem);
      } catch (...) {
        cb_alloc_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }
  }

  template <typename U, typename... Ts>
  constexpr polymorphic(std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
  {
    if constexpr (sizeof(U) <= XYZ_POLYMORPHIC_BUFFER_SIZE &&
                  std::is_trivially_copyable<U>::value) {
      new (&std::get<1>(storage_)) U(std::forward<Ts>(ts)...);
    } else {
      using cb_allocator = typename allocator_traits::template rebind_alloc<
          detail::direct_control_block<T, U, A>>;
      cb_allocator cb_alloc(alloc_);
      using cb_alloc_traits = std::allocator_traits<cb_allocator>;
      auto* mem = cb_alloc_traits::allocate(cb_alloc, 1);
      try {
        cb_alloc_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
        storage_.template emplace<2>(mem);
      } catch (...) {
        cb_alloc_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }
  }

  template <class U, class... Ts>
  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> &&
             (std::derived_from<U, T> || std::same_as<U, T>)
      : alloc_(alloc) {
    if constexpr (sizeof(U) <= XYZ_POLYMORPHIC_BUFFER_SIZE &&
                  std::is_trivially_copyable<U>::value) {
      new (&std::get<1>(storage_)) U(std::forward<Ts>(ts)...);
    } else {
      using cb_allocator = typename std::allocator_traits<
          A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
      using cb_alloc_traits = std::allocator_traits<cb_allocator>;
      cb_allocator cb_alloc(alloc_);
      auto* mem = cb_alloc_traits::allocate(cb_alloc, 1);
      try {
        cb_alloc_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
        storage_.template emplace<2>(mem);
      } catch (...) {
        cb_alloc_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }
  }

  constexpr polymorphic(const polymorphic& other) : alloc_(other.alloc_) {
    assert(!other.valueless_after_move());
    if (storage_.index() == 1) {
    } else if (storage_.index() == 2) {
    } else {
      std::unreachable();
    }
  }

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        const polymorphic& other)
      : alloc_(alloc) {
    assert(!other.valueless_after_move());
    if (storage_.index() == 1) {
    } else if (storage_.index() == 2) {
    } else {
      std::unreachable();
    }
  }

  constexpr polymorphic(polymorphic&& other) noexcept : alloc_(other.alloc_) {
    assert(!other.valueless_after_move());
    if (storage_.index() == 1) {
    } else if (storage_.index() == 2) {
    } else {
      std::unreachable();
    }
  }

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        polymorphic&& other) noexcept
      : alloc_(alloc) {
    assert(!other.valueless_after_move());
    if (storage_.index() == 1) {
    } else if (storage_.index() == 2) {
    } else {
      std::unreachable();
    }
  }

  constexpr polymorphic& operator=(const polymorphic& other) {
    assert(!other.valueless_after_move());
    if (storage_.index() == 1) {
    } else if (storage_.index() == 2) {
    } else {
      std::unreachable();
    }
    return *this;
  }

  constexpr polymorphic& operator=(polymorphic&& other) noexcept {
    assert(!other.valueless_after_move());
    if (storage_.index() == 1) {
    } else if (storage_.index() == 2) {
    } else {
      std::unreachable();
    }
    return *this;
  }

  constexpr ~polymorphic() noexcept {
    if (storage_.index() == 0) {
    } else if (storage_.index() == 1) {
    } else if (storage_.index() == 2) {
    } else {
      std::unreachable();
    }
  }

  constexpr T* operator->() noexcept {
    assert(!valueless_after_move());
    if constexpr (sizeof(T) <= XYZ_POLYMORPHIC_BUFFER_SIZE) {
      return std::launder(reinterpret_cast<T*>(&storage_));
    } else {
      return std::get<1>(storage_).p_;
    }
  }

  constexpr const T* operator->() const noexcept {
    assert(!valueless_after_move());
    if constexpr (sizeof(T) <= XYZ_POLYMORPHIC_BUFFER_SIZE) {
      return std::launder(reinterpret_cast<const T*>(&storage_));
    } else {
      return std::get<1>(storage_).p_;
    }
  }

  constexpr T& operator*() noexcept {
    assert(!valueless_after_move());
    if constexpr (sizeof(T) <= XYZ_POLYMORPHIC_BUFFER_SIZE) {
      return *std::launder(reinterpret_cast<T*>(&storage_));
    } else {
      return *std::get<1>(storage_).p_;
    }
  }

  constexpr const T& operator*() const noexcept {
    assert(!valueless_after_move());
    if constexpr (sizeof(T) <= XYZ_POLYMORPHIC_BUFFER_SIZE) {
      return *std::launder(reinterpret_cast<const T*>(&storage_));
    } else {
      return *std::get<1>(storage_).p_;
    }
  }

  constexpr bool valueless_after_move() const noexcept {
    return storage_.index() == 0;
  }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }


  
  constexpr void swap(polymorphic& /*other*/) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
//    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
//    std::swap(cb_, other.cb_);
//    if constexpr (allocator_traits::propagate_on_container_swap::value) {
//      std::swap(alloc_, other.alloc_);
//    }
  }

  friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    lhs.swap(rhs);
  }
};
}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_