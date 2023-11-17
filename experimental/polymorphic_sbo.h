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

#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <utility>
#include <variant>

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

static constexpr size_t PolymorphicBufferCapacity = 32;

template <typename T>
constexpr bool is_sbo_compatible() {
  return std::is_nothrow_move_constructible_v<T> &&
         (sizeof(T) <= PolymorphicBufferCapacity);
}

template <class T, class A>
struct control_block {
  virtual constexpr T* ptr() noexcept = 0;
  virtual constexpr ~control_block() = default;
  virtual constexpr void destroy(A& alloc) = 0;
  virtual constexpr control_block<T, A>* clone(A& alloc) = 0;
};

template <class T, class U, class A>
class direct_control_block final : public control_block<T, A> {
  U u_;

 public:
  constexpr ~direct_control_block() override = default;

  template <class... Ts>
  constexpr direct_control_block(Ts&&... ts)
    requires std::constructible_from<U, Ts...>
      : u_(std::forward<Ts>(ts)...) {}

  constexpr T* ptr() noexcept override { return &u_; }

  constexpr control_block<T, A>* clone(A& alloc) override {
    using cb_allocator_t = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator_t cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator_t>;
    auto* mem = cb_alloc_traits::allocate(cb_alloc, 1);
    try {
      cb_alloc_traits::construct(cb_alloc, mem, u_);
      return mem;
    } catch (...) {
      cb_alloc_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  constexpr void destroy(A& alloc) override {
    using cb_allocator_t = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator_t cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator_t>;
    cb_alloc_traits::destroy(cb_alloc, this);
    cb_alloc_traits::deallocate(cb_alloc, this, 1);
  }
};

template <class T, class A>
class buffer {
  alignas(
      std::max_align_t) std::array<std::byte, PolymorphicBufferCapacity> data_;

  struct vtable {
    using ptr_fn = T* (*)(buffer*);
    using clone_fn = void (*)(A, const buffer*, buffer*);
    using relocate_fn = void (*)(A, buffer*, buffer*);
    using destroy_fn = void (*)(A, buffer*);

    ptr_fn ptr = nullptr;
    clone_fn clone = nullptr;
    relocate_fn relocate = nullptr;
    destroy_fn destroy = nullptr;
  } vtable_;

  template <typename U>
  U* aligned_storage_for() {
    return std::launder(reinterpret_cast<U*>(data_.data()));
  }

  template <typename U>
  const U* aligned_storage_for() const {
    return std::launder(reinterpret_cast<const U*>(data_.data()));
  }

 public:
  constexpr buffer() = default;

  template <typename U, typename... Ts>
    requires std::derived_from<U, T> && std::constructible_from<U, Ts...> &&
             (is_sbo_compatible<U>())
  constexpr buffer(std::type_identity<U>, A allocator, Ts&&... ts) {
    using u_allocator_t =
        typename std::allocator_traits<A>::template rebind_alloc<U>;
    using u_allocator_traits = std::allocator_traits<u_allocator_t>;

    vtable_.ptr = [](buffer* self) -> T* {
      return static_cast<T*>(self->aligned_storage_for<U>());
    };

    vtable_.clone = [](A allocator, const buffer* self,
                       buffer* destination) -> void {
      const U* u = self->aligned_storage_for<U>();
      u_allocator_t u_allocator(allocator);
      u_allocator_traits::construct(u_allocator,
                                    destination->aligned_storage_for<U>(), *u);
      destination->vtable_ = self->vtable_;
    };

    vtable_.relocate = [](A allocator, buffer* self,
                          buffer* destination) -> void {
      if constexpr (std::is_trivially_copy_constructible_v<U>) {
        std::memcpy(destination->data_.data(), self->data_.data(),
                    PolymorphicBufferCapacity);
      } else {
        const U* u = self->aligned_storage_for<U>();
        u_allocator_t u_allocator(allocator);
        u_allocator_traits::construct(
            u_allocator, destination->aligned_storage_for<U>(), std::move(*u));
      }
      destination->vtable_ = self->vtable_;
    };

    vtable_.destroy = [](A allocator, buffer* self) -> void {
      u_allocator_t u_allocator(allocator);
      u_allocator_traits::destroy(u_allocator,
                                  std::launder(self->aligned_storage_for<U>()));
    };

    u_allocator_t u_allocator(allocator);
    u_allocator_traits::construct(u_allocator, aligned_storage_for<U>(),
                                  std::forward<Ts>(ts)...);
  }

  constexpr void clone(A allocator, buffer& destination) const {
    (*vtable_.clone)(allocator, this, &destination);
  }

  constexpr void destroy(A allocator) { vtable_.destroy(allocator, this); }

  constexpr void relocate(A allocator, buffer& destination) {
    (*vtable_.relocate)(allocator, this, &destination);
  }

  constexpr T* ptr() { return (*vtable_.ptr)(this); }
};

}  // namespace detail

template <class T, class A = std::allocator<T>>
class polymorphic {
  T* p_ = nullptr;
  std::variant<std::monostate, detail::buffer<T, A>,
               detail::control_block<T, A>*>
      storage_;
  enum idx { EMPTY, BUFFER, CONTROL_BLOCK };

#if defined(_MSC_VER)
  [[msvc::no_unique_address]] A alloc_;
#else
  [[no_unique_address]] A alloc_;
#endif

  using allocator_traits = std::allocator_traits<A>;

 public:
  using value_type = T;
  using allocator_type = A;

  template <class U, class... Ts>
  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
      : alloc_(alloc) {
    if constexpr (detail::is_sbo_compatible<U>()) {
      emplace<BUFFER>(std::type_identity<U>{}, alloc_, std::forward<Ts>(ts)...);
      p_ = get<BUFFER>().ptr();
    } else {
      using cb_allocator_t = typename std::allocator_traits<
          A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
      using cb_traits = std::allocator_traits<cb_allocator_t>;
      cb_allocator_t cb_alloc(alloc_);
      auto* mem = cb_traits::allocate(cb_alloc, 1);
      try {
        cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
        emplace<CONTROL_BLOCK>(mem);
        p_ = get<CONTROL_BLOCK>()->ptr();
      } catch (...) {
        cb_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }
  }

  constexpr polymorphic()
    requires std::default_initializable<T> && std::default_initializable<A>
      : polymorphic(std::allocator_arg, A(), std::in_place_type<T>) {}

  template <class U, class... Ts>
  explicit constexpr polymorphic(std::in_place_type_t<U>, Ts&&... ts)
    requires std::default_initializable<A>
      : polymorphic(std::allocator_arg, A(), std::in_place_type<U>,
                    std::forward<Ts>(ts)...) {}

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        const polymorphic& other)
      : alloc_(alloc) {
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(other.storage_.index())) {
      case BUFFER:
        other.get<BUFFER>().clone(alloc_, emplace<BUFFER>());
        p_ = get<BUFFER>().ptr();
        break;
      case CONTROL_BLOCK:
        emplace<CONTROL_BLOCK>(other.get<CONTROL_BLOCK>()->clone(alloc_));
        p_ = get<CONTROL_BLOCK>()->ptr();
        break;
      case EMPTY:       // LCOV_EXCL_LINE
        unreachable();  // LCOV_EXCL_LINE
    }
  }

  constexpr polymorphic(const polymorphic& other)
      : polymorphic(std::allocator_arg,
                    allocator_traits::select_on_container_copy_construction(
                        other.alloc_),
                    other) {}

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        polymorphic&& other) noexcept
      : alloc_(alloc) {
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(other.storage_.index())) {
      case BUFFER: {
        other.get<BUFFER>().relocate(alloc_, emplace<BUFFER>());
        p_ = get<BUFFER>().ptr();
        other.reset();
        break;
      }
      case CONTROL_BLOCK: {
        auto* other_cb = other.get<CONTROL_BLOCK>();
        emplace<CONTROL_BLOCK>(other_cb);
        p_ = get<CONTROL_BLOCK>()->ptr();
        other.emplace<EMPTY>();
        break;
      }
      case EMPTY:       // LCOV_EXCL_LINE
        unreachable();  // LCOV_EXCL_LINE
    }
  }

  constexpr polymorphic(polymorphic&& other) noexcept
      : polymorphic(std::allocator_arg, other.alloc_, std::move(other)) {}

  constexpr ~polymorphic() { reset(); }

  constexpr polymorphic& operator=(const polymorphic& other) {
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE
    if (this != &other) {
      if constexpr (allocator_traits::propagate_on_container_copy_assignment::
                        value) {
        alloc_ = other.alloc_;
        polymorphic tmp(std::allocator_arg, alloc_, other);
        swap(tmp);
      } else {
        polymorphic tmp(other);
        this->swap(tmp);
      }
    }
    return *this;
  }

  constexpr polymorphic& operator=(polymorphic&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value) {
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE
    if (&other == this) return *this;

    reset();
    if constexpr (allocator_traits::propagate_on_container_move_assignment::
                      value) {
      alloc_ = other.alloc_;
      switch (static_cast<idx>(other.storage_.index())) {
        case BUFFER: {
          emplace<BUFFER>();
          other.get<BUFFER>().relocate(other.alloc_, get<BUFFER>());
          p_ = get<BUFFER>().ptr();
          other.reset();
          return *this;
        }
        case CONTROL_BLOCK: {
          auto* other_cb = other.get<CONTROL_BLOCK>();
          emplace<CONTROL_BLOCK>(other_cb);
          p_ = get<CONTROL_BLOCK>()->ptr();
          other.emplace<EMPTY>();
          return *this;
        }
        case EMPTY:       // LCOV_EXCL_LINE
          unreachable();  // LCOV_EXCL_LINE
      }
    } else {
      if (alloc_ == other.alloc_) {
        switch (static_cast<idx>(other.storage_.index())) {
          case BUFFER: {
            other.get<BUFFER>().relocate(other.alloc_, emplace<BUFFER>());
            p_ = get<BUFFER>().ptr();
            other.reset();
            return *this;
          }
          case CONTROL_BLOCK: {
            auto* other_cb = other.get<CONTROL_BLOCK>();
            emplace<CONTROL_BLOCK>(other_cb);
            p_ = get<CONTROL_BLOCK>()->ptr();
            other.emplace<EMPTY>();
            return *this;
          }
          case EMPTY:       // LCOV_EXCL_LINE
            unreachable();  // LCOV_EXCL_LINE
        }
      } else {
        switch (static_cast<idx>(other.storage_.index())) {
          case BUFFER:
            other.get<BUFFER>().clone(alloc_, emplace<BUFFER>());
            p_ = get<BUFFER>().ptr();
            break;
          case CONTROL_BLOCK:
            emplace<CONTROL_BLOCK>(other.get<CONTROL_BLOCK>()->clone(alloc_));
            p_ = get<CONTROL_BLOCK>()->ptr();
            break;
          case EMPTY:       // LCOV_EXCL_LINE
            unreachable();  // LCOV_EXCL_LINE
        }
        other.reset();
      }
    }
    return *this;
  }

  constexpr T* operator->() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  constexpr const T* operator->() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return p_;
  }

  constexpr T& operator*() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  constexpr const T& operator*() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    return *p_;
  }

  constexpr bool valueless_after_move() const noexcept {
    return storage_.index() == EMPTY;
  }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  constexpr void swap(polymorphic& other) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    assert(!valueless_after_move());        // LCOV_EXCL_LINE
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE

    switch (static_cast<idx>(storage_.index())) {
      case BUFFER:
        switch (static_cast<idx>(other.storage_.index())) {
          case BUFFER: {
            auto& buf = get<BUFFER>();
            auto& other_buf = other.get<BUFFER>();
            detail::buffer<T, A> tmp;

            // Swap the buffers using relocate.
            buf.relocate(alloc_, tmp);
            other_buf.relocate(other.alloc_, buf);
            // `tmp` relocate uses the allocator from `this` as `tmp`'s buffer
            // content came from `this`.
            tmp.relocate(alloc_, other_buf);

            // Update pointers.
            p_ = get<BUFFER>().ptr();
            other.p_ = other.get<BUFFER>().ptr();
            break;
          }
          case CONTROL_BLOCK: {
            auto* other_cb = other.get<CONTROL_BLOCK>();
            get<BUFFER>().relocate(alloc_, other.emplace<BUFFER>());
            emplace<CONTROL_BLOCK>(other_cb);
            p_ = get<CONTROL_BLOCK>()->ptr();
            other.p_ = other.get<BUFFER>().ptr();
            break;
          }
          case EMPTY:       // LCOV_EXCL_LINE
            unreachable();  // LCOV_EXCL_LINE
        }
        break;
      case CONTROL_BLOCK:
        switch (static_cast<idx>(other.storage_.index())) {
          case BUFFER: {
            auto* cb = get<CONTROL_BLOCK>();
            other.get<BUFFER>().relocate(other.alloc_, emplace<BUFFER>());
            p_ = get<BUFFER>().ptr();
            other.emplace<CONTROL_BLOCK>(cb);
            other.p_ = other.get<CONTROL_BLOCK>()->ptr();
            break;
          }
          case CONTROL_BLOCK: {
            std::swap(get<CONTROL_BLOCK>(), other.get<CONTROL_BLOCK>());
            p_ = get<CONTROL_BLOCK>()->ptr();
            other.p_ = other.get<CONTROL_BLOCK>()->ptr();
            break;
          }
          case EMPTY:       // LCOV_EXCL_LINE
            unreachable();  // LCOV_EXCL_LINE
        }
        break;
      case EMPTY:       // LCOV_EXCL_LINE
        unreachable();  // LCOV_EXCL_LINE
    }
    if constexpr (allocator_traits::propagate_on_container_swap::value) {
      std::swap(alloc_, other.alloc_);
    }
  }

  friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    lhs.swap(rhs);
  }

 private:
  constexpr void reset() noexcept {
    switch (static_cast<idx>(storage_.index())) {
      case BUFFER:
        get<BUFFER>().destroy(alloc_);
        break;
      case CONTROL_BLOCK:
        get<CONTROL_BLOCK>()->destroy(alloc_);
        break;
      case EMPTY:
        break;
    }
    emplace<EMPTY>();
    p_ = nullptr;
  }

  template <idx type_index>
  constexpr auto& get() {
    return std::get<type_index>(storage_);
  }

  template <idx type_index>
  constexpr const auto& get() const {
    return std::get<type_index>(storage_);
  }

  template <idx type_index, typename... Ts>
  constexpr decltype(auto) emplace(Ts&&... ts) {
    return storage_.template emplace<type_index>(std::forward<Ts>(ts)...);
  }
};
}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_
