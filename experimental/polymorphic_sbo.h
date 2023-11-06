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

static constexpr size_t POLYMORPHIC_SBO_CAPACITY = 32;
static constexpr size_t POLYMORPHIC_SBO_ALIGNMENT = alignof(std::max_align_t);

[[noreturn]] inline void unreachable() { std::terminate(); }  // LCOV_EXCL_LINE

struct NoPolymorphicSBO {};

namespace detail {

struct tag_t {};
constexpr tag_t tag;

template <class T, class A>
struct control_block {
  using allocator_type = A;

  virtual constexpr T* ptr() noexcept = 0;
  virtual constexpr ~control_block() = default;
  virtual constexpr void destroy(A& alloc) = 0;
  virtual constexpr control_block<T, A>* clone(A& alloc) = 0;
};

template <class T, class U, class A>
class direct_control_block : public control_block<T, A> {
  U u_;

 public:
  constexpr ~direct_control_block() override = default;

  template <class... Ts>
  constexpr direct_control_block(Ts&&... ts)
    requires std::constructible_from<U, Ts...>
      : u_(std::forward<Ts>(ts)...) {}

  constexpr T* ptr() noexcept override { return &u_; }

  constexpr control_block<T, A>* clone(A& alloc) override {
    using cb_allocator_t = std::allocator_traits<A>::template rebind_alloc<
        direct_control_block<T, U, A>>;
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
    using cb_allocator_t = std::allocator_traits<A>::template rebind_alloc<
        direct_control_block<T, U, A>>;
    cb_allocator_t cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator_t>;
    cb_alloc_traits::destroy(cb_alloc, this);
    cb_alloc_traits::deallocate(cb_alloc, this, 1);
  }
};

template <class T, class A, size_t N, size_t Align>
class buffer {
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

  std::byte padding_[3];

  using data_t = std::array<std::byte, N>;
  alignas(Align) data_t data_;

  template <typename U>
  U* aligned_storage_for() {
    void* raw_buffer = data_.data();
    // `can_hold_type<U>` ensures that alignment is ok.
    auto aligned_buffer = raw_buffer;
    // size_t size = data_.size();
    // void* aligned_buffer = std::align(alignof(U), size, raw_buffer, size);
    // assert(aligned_buffer && "Cannot align sufficient storage for type U");
    return std::launder(reinterpret_cast<U*>(aligned_buffer));
  }

  template <typename U>
  const U* aligned_storage_for() const {
    const void* raw_buffer = data_.data();
    // `can_hold_type<U>` ensures that alignment is ok.
    auto aligned_buffer = raw_buffer;
    // size_t size = data_.size();
    // void* aligned_buffer = std::align(alignof(U), size, raw_buffer, size);
    // assert(aligned_buffer && "Cannot align sufficient storage for type U");
    return std::launder(reinterpret_cast<const U*>(aligned_buffer));
  }

 public:
  template <typename U>
  static constexpr bool can_hold_type() {
    // clang-format off
    return !std::is_base_of_v<NoPolymorphicSBO, T> && // Used to disable SBO for tests.
           std::is_nothrow_move_constructible_v<T> && // Needed for noexcept move of the buffer.
           sizeof(U) <= N &&                          // We need enough space.
           alignof(U) <= Align;                       // TODO: Probably stricter than we need.
    // clang-format on
  }

  constexpr buffer() = default;

  template <typename U, typename... Ts>
    requires std::derived_from<U, T> && std::constructible_from<U, Ts...> &&
             (can_hold_type<U>())
  constexpr buffer(std::type_identity<U>, A allocator, Ts&&... ts) {
    using u_allocator_t = std::allocator_traits<A>::template rebind_alloc<U>;
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
        std::memcpy(destination->data_.data(), self->data_.data(), N);
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
      u_allocator_traits::destroy(u_allocator, self->aligned_storage_for<U>());
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

template <class T, class A = std::allocator<T>,
          size_t N = POLYMORPHIC_SBO_CAPACITY,
          size_t Align = POLYMORPHIC_SBO_ALIGNMENT>
class polymorphic {
  T* p_;

  using buffer_t = detail::buffer<T, A, N, Align>;
  using control_block_t = detail::control_block<T, A>;

  enum idx { EMPTY, BUFFER, CONTROL_BLOCK };
  std::variant<std::monostate, buffer_t, control_block_t*> storage_;

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
    if constexpr (buffer_t::template can_hold_type<U>()) {
      emplace<idx::BUFFER>(std::type_identity<U>{}, alloc_,
                           std::forward<Ts>(ts)...);
      p_ = get<idx::BUFFER>().ptr();
    } else {
      using cb_allocator_t = std::allocator_traits<A>::template rebind_alloc<
          detail::direct_control_block<T, U, A>>;
      using cb_traits = std::allocator_traits<cb_allocator_t>;
      cb_allocator_t cb_alloc(alloc_);
      auto* mem = cb_traits::allocate(cb_alloc, 1);
      try {
        cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
        emplace<idx::CONTROL_BLOCK>(mem);
        p_ = get<idx::CONTROL_BLOCK>()->ptr();
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
      case idx::BUFFER:
        other.get<idx::BUFFER>().clone(alloc_, emplace<idx::BUFFER>());
        p_ = get<idx::BUFFER>().ptr();
        break;
      case idx::CONTROL_BLOCK:
        emplace<idx::CONTROL_BLOCK>(
            other.get<idx::CONTROL_BLOCK>()->clone(alloc_));
        p_ = get<idx::CONTROL_BLOCK>()->ptr();
        break;
      case idx::EMPTY:  // LCOV_EXCL_LINE
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
      case idx::BUFFER: {
        other.get<idx::BUFFER>().relocate(alloc_, emplace<idx::BUFFER>());
        p_ = get<idx::BUFFER>().ptr();
        other.reset();
        break;
      }
      case idx::CONTROL_BLOCK: {
        auto* other_cb = other.get<idx::CONTROL_BLOCK>();
        emplace<idx::CONTROL_BLOCK>(other_cb);
        p_ = get<idx::CONTROL_BLOCK>()->ptr();
        other.emplace<idx::EMPTY>();
        break;
      }
      case idx::EMPTY:  // LCOV_EXCL_LINE
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
        case idx::BUFFER: {
          emplace<idx::BUFFER>();
          other.get<idx::BUFFER>().relocate(other.alloc_, get<idx::BUFFER>());
          p_ = get<idx::BUFFER>().ptr();
          other.reset();
          return *this;
        }
        case idx::CONTROL_BLOCK: {
          auto* other_cb = other.get<idx::CONTROL_BLOCK>();
          emplace<idx::CONTROL_BLOCK>(other_cb);
          p_ = get<idx::CONTROL_BLOCK>()->ptr();
          other.emplace<idx::EMPTY>();
          return *this;
        }
        case idx::EMPTY:  // LCOV_EXCL_LINE
          unreachable();  // LCOV_EXCL_LINE
      }
    } else {
      if (alloc_ == other.alloc_) {
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER: {
            other.get<idx::BUFFER>().relocate(other.alloc_,
                                              emplace<idx::BUFFER>());
            p_ = get<idx::BUFFER>().ptr();
            other.reset();
            return *this;
          }
          case idx::CONTROL_BLOCK: {
            auto* other_cb = other.get<idx::CONTROL_BLOCK>();
            emplace<idx::CONTROL_BLOCK>(other_cb);
            p_ = get<idx::CONTROL_BLOCK>()->ptr();
            other.emplace<idx::EMPTY>();
            return *this;
          }
          case idx::EMPTY:  // LCOV_EXCL_LINE
            unreachable();  // LCOV_EXCL_LINE
        }
      } else {
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER:
            other.get<idx::BUFFER>().clone(alloc_, emplace<idx::BUFFER>());
            p_ = get<idx::BUFFER>().ptr();
            break;
          case idx::CONTROL_BLOCK:
            emplace<idx::CONTROL_BLOCK>(
                other.get<idx::CONTROL_BLOCK>()->clone(alloc_));
            p_ = get<idx::CONTROL_BLOCK>()->ptr();
            break;
          case idx::EMPTY:  // LCOV_EXCL_LINE
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
    return storage_.index() == idx::EMPTY;
  }

  constexpr bool is_buffered(detail::tag_t) const noexcept {
    return storage_.index() == idx::BUFFER;
  }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  constexpr void swap(polymorphic& other) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    assert(!valueless_after_move());        // LCOV_EXCL_LINE
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE

    switch (static_cast<idx>(storage_.index())) {
      case idx::BUFFER:
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER: {
            auto& buf = get<idx::BUFFER>();
            auto& other_buf = other.get<idx::BUFFER>();
            buffer_t tmp;

            // Swap the buffers using relocate.
            buf.relocate(alloc_, tmp);
            other_buf.relocate(other.alloc_, buf);
            // `tmp` relocate uses the allocator
            // from `this` as `tmp`'s buffer content
            // came from `this`.
            tmp.relocate(alloc_, other_buf);

            // Update pointers.
            p_ = get<idx::BUFFER>().ptr();
            other.p_ = other.get<idx::BUFFER>().ptr();
            break;
          }
          case idx::CONTROL_BLOCK: {
            auto* other_cb = other.get<idx::CONTROL_BLOCK>();
            get<idx::BUFFER>().relocate(alloc_, other.emplace<idx::BUFFER>());
            emplace<idx::CONTROL_BLOCK>(other_cb);
            p_ = get<idx::CONTROL_BLOCK>()->ptr();
            other.p_ = other.get<idx::BUFFER>().ptr();
            break;
          }
          case idx::EMPTY:  // LCOV_EXCL_LINE
            unreachable();  // LCOV_EXCL_LINE
        }
        break;
      case idx::CONTROL_BLOCK:
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER: {
            auto* cb = get<idx::CONTROL_BLOCK>();
            other.get<idx::BUFFER>().relocate(other.alloc_,
                                              emplace<idx::BUFFER>());
            p_ = get<idx::BUFFER>().ptr();
            other.emplace<idx::CONTROL_BLOCK>(cb);
            other.p_ = other.get<idx::CONTROL_BLOCK>()->ptr();
            break;
          }
          case idx::CONTROL_BLOCK: {
            std::swap(get<idx::CONTROL_BLOCK>(),
                      other.get<idx::CONTROL_BLOCK>());
            p_ = get<idx::CONTROL_BLOCK>()->ptr();
            other.p_ = other.get<idx::CONTROL_BLOCK>()->ptr();
            break;
          }
          case idx::EMPTY:  // LCOV_EXCL_LINE
            unreachable();  // LCOV_EXCL_LINE
        }
        break;
      case idx::EMPTY:  // LCOV_EXCL_LINE
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
      case idx::BUFFER:
        get<idx::BUFFER>().destroy(alloc_);
        break;
      case idx::CONTROL_BLOCK:
        get<idx::CONTROL_BLOCK>()->destroy(alloc_);
        break;
      case idx::EMPTY:
        break;
    }
    emplace<idx::EMPTY>();
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
