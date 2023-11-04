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
#include <functional>
#include <memory>
#include <utility>
#include <variant>

namespace xyz {

[[noreturn]] inline void unreachable() { std::terminate(); }  // LCOV_EXCL_LINE

struct NoPolymorphicSBO {};

namespace detail {

static constexpr size_t PolymorphicBufferCapacity = 32;

template <typename T>
constexpr bool is_sbo_compatible() {
  return !std::is_base_of_v<NoPolymorphicSBO, T> &&
         std::is_nothrow_move_constructible_v<T> &&
         (sizeof(T) <= PolymorphicBufferCapacity);
}

template <class T, class A>
struct control_block {
  using allocator_type = A;

  virtual T* ptr() noexcept = 0;
  virtual ~control_block() = default;
  virtual void destroy(A& alloc) = 0;
  virtual control_block<T, A>* clone(A& alloc) = 0;
};

template <class T, class U, class A>
class direct_control_block : public control_block<T, A> {
  U u_;

 public:
  ~direct_control_block() override = default;

  template <class... Ts>
  direct_control_block(Ts&&... ts)
    requires std::constructible_from<U, Ts...>
      : u_(std::forward<Ts>(ts)...) {}

  T* ptr() noexcept override { return &u_; }

  control_block<T, A>* clone(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    auto* mem = cb_alloc_traits::allocate(cb_alloc, 1);
    try {
      cb_alloc_traits::construct(cb_alloc, mem, u_);
      return mem;
    } catch (...) {
      cb_alloc_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  void destroy(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    cb_alloc_traits::destroy(cb_alloc, this);
    cb_alloc_traits::deallocate(cb_alloc, this, 1);
  }
};

template <class T>
struct buffer {
  using ptr_fn = T* (*)(buffer*);
  using clone_fn = void (*)(const buffer*, buffer*);
  using relocate_fn = void (*)(buffer*, buffer*);
  using destroy_fn = void (*)(buffer*);

  struct vtable {
    ptr_fn ptr = nullptr;
    clone_fn clone = nullptr;
    relocate_fn relocate = nullptr;
    destroy_fn destroy = nullptr;
  } vtable_;
  std::array<std::byte, PolymorphicBufferCapacity> data_;

  buffer() = default;

  template <typename U, typename... Ts>
    requires std::derived_from<U, T> && std::constructible_from<U, Ts...> &&
             (is_sbo_compatible<U>())
  buffer(std::type_identity<U>, Ts&&... ts)
      : vtable_{.ptr = [](buffer* self) -> T* {
                  U* u = std::launder(reinterpret_cast<U*>(self->data_.data()));
                  return static_cast<T*>(u);
                },
                .clone = [](const buffer* self, buffer* destination) -> void {
                  const U* u = std::launder(
                      reinterpret_cast<const U*>(self->data_.data()));
                  new (destination->data_.data()) U(*u);
                  destination->vtable_ = self->vtable_;
                },
                .relocate = [](buffer* self, buffer* destination) -> void {
                  U* u = std::launder(reinterpret_cast<U*>(self->data_.data()));
                  new (destination->data_.data()) U(std::move(*u));
                  destination->vtable_ = self->vtable_;
                },
                .destroy = [](buffer* self) -> void {
                  std::launder(reinterpret_cast<U*>(self->data_.data()))->~U();
                }} {
    new (data_.data()) U(std::forward<Ts>(ts)...);
  }

  void clone(buffer& destination) const {
    (*vtable_.clone)(this, &destination);
  }

  void destroy() { vtable_.destroy(this); }

  void relocate(buffer& destination) {
    (*vtable_.relocate)(this, &destination);
  }

  T* ptr() { return (*vtable_.ptr)(this); }
};

}  // namespace detail

template <class T, class A = std::allocator<T>>
class polymorphic {
  T* p_;
  std::variant<std::monostate, detail::buffer<T>, detail::control_block<T, A>*>
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
  polymorphic(std::allocator_arg_t, const A& alloc, std::in_place_type_t<U>,
              Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
      : alloc_(alloc) {
    if constexpr (detail::is_sbo_compatible<U>()) {
      storage_.template emplace<idx::BUFFER>(std::type_identity<U>{},
                                             std::forward<Ts>(ts)...);
      update_ptr();
    } else {
      using cb_allocator = typename std::allocator_traits<
          A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
      using cb_traits = std::allocator_traits<cb_allocator>;
      cb_allocator cb_alloc(alloc_);
      auto* mem = cb_traits::allocate(cb_alloc, 1);
      try {
        cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
        storage_.template emplace<idx::CONTROL_BLOCK>(mem);
        update_ptr();
      } catch (...) {
        cb_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }
  }

  polymorphic()
    requires std::default_initializable<T> && std::default_initializable<A>
      : polymorphic(std::allocator_arg, A(), std::in_place_type<T>) {}

  template <class U, class... Ts>
  explicit polymorphic(std::in_place_type_t<U>, Ts&&... ts)
    requires std::default_initializable<A>
      : polymorphic(std::allocator_arg, A(), std::in_place_type<U>,
                    std::forward<Ts>(ts)...) {}

  polymorphic(std::allocator_arg_t, const A& alloc, const polymorphic& other)
      : alloc_(alloc) {
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(other.storage_.index())) {
      case idx::BUFFER:
        std::get<idx::BUFFER>(other.storage_)
            .clone(storage_.template emplace<idx::BUFFER>());
        update_ptr();
        break;
      case idx::CONTROL_BLOCK:
        storage_.template emplace<idx::CONTROL_BLOCK>(
            std::get<idx::CONTROL_BLOCK>(other.storage_)->clone(alloc_));
        update_ptr();
        break;
      case idx::EMPTY:  // LCOV_EXCL_LINE
        unreachable();  // LCOV_EXCL_LINE
    }
  }

  polymorphic(const polymorphic& other)
      : polymorphic(std::allocator_arg,
                    allocator_traits::select_on_container_copy_construction(
                        other.alloc_),
                    other) {}

  polymorphic(std::allocator_arg_t, const A& alloc,
              polymorphic&& other) noexcept
      : alloc_(alloc) {
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(other.storage_.index())) {
      case idx::BUFFER: {
        auto& buf = std::get<idx::BUFFER>(other.storage_);
        storage_.template emplace<idx::BUFFER>();
        buf.relocate(std::get<idx::BUFFER>(storage_));
        update_ptr();
        other.reset();
        break;
      }
      case idx::CONTROL_BLOCK: {
        auto* cb = std::get<idx::CONTROL_BLOCK>(other.storage_);
        storage_.template emplace<idx::CONTROL_BLOCK>(cb);
        update_ptr();
        other.storage_.template emplace<idx::EMPTY>();
        break;
      }
      case idx::EMPTY:  // LCOV_EXCL_LINE
        unreachable();  // LCOV_EXCL_LINE
    }
  }

  polymorphic(polymorphic&& other) noexcept
      : polymorphic(std::allocator_arg, other.alloc_, std::move(other)) {}

  ~polymorphic() { reset(); }

  polymorphic& operator=(const polymorphic& other) {
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

  polymorphic& operator=(polymorphic&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value) {
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE
    reset();
    if constexpr (allocator_traits::propagate_on_container_move_assignment::
                      value) {
      alloc_ = other.alloc_;
      switch (static_cast<idx>(other.storage_.index())) {
        case idx::BUFFER: {
          auto& buf = std::get<idx::BUFFER>(other.storage_);
          storage_.template emplace<idx::BUFFER>();
          buf.relocate(std::get<idx::BUFFER>(storage_));
          other.reset();
          update_ptr();
          return *this;
        }
        case idx::CONTROL_BLOCK: {
          auto* cb = std::get<idx::CONTROL_BLOCK>(other.storage_);
          storage_.template emplace<idx::CONTROL_BLOCK>(cb);
          other.storage_.template emplace<idx::EMPTY>();
          update_ptr();
          return *this;
        }
        case idx::EMPTY:  // LCOV_EXCL_LINE
          unreachable();  // LCOV_EXCL_LINE
      }
    } else {
      if (alloc_ == other.alloc_) {
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER: {
            auto& buf = std::get<idx::BUFFER>(other.storage_);
            storage_.template emplace<idx::BUFFER>();
            buf.relocate(std::get<idx::BUFFER>(storage_));
            other.reset();
            update_ptr();
            return *this;
          }
          case idx::CONTROL_BLOCK: {
            auto* cb = std::get<idx::CONTROL_BLOCK>(other.storage_);
            storage_.template emplace<idx::CONTROL_BLOCK>(cb);
            other.storage_.template emplace<idx::EMPTY>();
            update_ptr();
            return *this;
          }
          case idx::EMPTY:  // LCOV_EXCL_LINE
            unreachable();  // LCOV_EXCL_LINE
        }
      } else {
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER:
            storage_.template emplace<idx::BUFFER>();
            std::get<idx::BUFFER>(other.storage_)
                .clone(std::get<idx::BUFFER>(storage_));
            update_ptr();
            break;
          case idx::CONTROL_BLOCK:
            storage_.template emplace<idx::CONTROL_BLOCK>(
                std::get<idx::CONTROL_BLOCK>(other.storage_)->clone(alloc_));
            update_ptr();
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

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  void swap(polymorphic& other) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    assert(!valueless_after_move());        // LCOV_EXCL_LINE
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE

    switch (static_cast<idx>(storage_.index())) {
      case idx::BUFFER:
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER: {
            auto& buf = std::get<idx::BUFFER>(storage_);
            auto& other_buf = std::get<idx::BUFFER>(other.storage_);
            detail::buffer<T> tmp;
            buf.relocate(tmp);
            other_buf.relocate(buf);
            tmp.relocate(other_buf);
            update_ptr();
            other.update_ptr();
            break;
          }
          case idx::CONTROL_BLOCK: {
            auto& buf = std::get<idx::BUFFER>(storage_);
            auto* other_cb = std::get<idx::CONTROL_BLOCK>(other.storage_);
            buf.relocate(other.storage_.template emplace<idx::BUFFER>());
            storage_.template emplace<idx::CONTROL_BLOCK>(other_cb);
            update_ptr();
            other.update_ptr();
            break;
          }
          case idx::EMPTY:  // LCOV_EXCL_LINE
            unreachable();  // LCOV_EXCL_LINE
        }
        break;
      case idx::CONTROL_BLOCK:
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER: {
            auto* cb = std::get<idx::CONTROL_BLOCK>(storage_);
            auto& other_buf = std::get<idx::BUFFER>(other.storage_);
            other_buf.relocate(storage_.template emplace<idx::BUFFER>());
            other.storage_.template emplace<idx::CONTROL_BLOCK>(cb);
            update_ptr();
            other.update_ptr();
            break;
          }
          case idx::CONTROL_BLOCK: {
            std::swap(std::get<idx::CONTROL_BLOCK>(storage_),
                      std::get<idx::CONTROL_BLOCK>(other.storage_));
            update_ptr();
            other.update_ptr();
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

  friend void swap(polymorphic& lhs, polymorphic& rhs) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    lhs.swap(rhs);
  }

 private:
  void reset() noexcept {
    switch (storage_.index()) {
      case idx::BUFFER:
        std::get<idx::BUFFER>(storage_).destroy();
        break;
      case idx::CONTROL_BLOCK:
        std::get<idx::CONTROL_BLOCK>(storage_)->destroy(alloc_);
        break;
    }
    storage_.template emplace<idx::EMPTY>();
    p_ = nullptr;
  }

  void update_ptr() {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(storage_.index())) {
      case idx::BUFFER:
        p_ = std::get<idx::BUFFER>(storage_).ptr();
        break;
      case idx::CONTROL_BLOCK:
        p_ = std::get<idx::CONTROL_BLOCK>(storage_)->ptr();
        break;
      case idx::EMPTY:  // LCOV_EXCL_LINE
        unreachable();  // LCOV_EXCL_LINE
    }
  }
};

}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_
