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

[[noreturn]] inline void unreachable() { std::terminate(); }

namespace detail {

template <class T, class A>
struct control_block {
  using allocator_type = A;

  T* p_;
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
  constexpr direct_control_block(Ts&&... ts) : u_(std::forward<Ts>(ts)...) {
    control_block<T, A>::p_ = &u_;
  }

  constexpr control_block<T, A>* clone(A& alloc) override {
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

  constexpr void destroy(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    cb_alloc_traits::destroy(cb_alloc, this);
    cb_alloc_traits::deallocate(cb_alloc, this, 1);
  }
};

static constexpr size_t PolymorphicBufferCapacity = 32;

template <typename T>
constexpr bool is_sbo_compatible() {
  return std::is_nothrow_move_constructible_v<T> &&
         (sizeof(T) <= PolymorphicBufferCapacity);
}

template <class T>
struct buffer {
  using ptr_fn = T* (*)(buffer*);
  using cptr_fn = const T* (*)(const buffer*);
  using clone_fn = void (*)(const buffer*, buffer*);
  using relocate_fn = void (*)(buffer*, buffer*);
  using destroy_fn = void (*)(buffer*);

  struct vtable {
    ptr_fn ptr = nullptr;
    cptr_fn cptr = nullptr;
    clone_fn clone = nullptr;
    relocate_fn relocate = nullptr;
    destroy_fn destroy = nullptr;
  } vtable_;
  std::array<std::byte, PolymorphicBufferCapacity> data_;

  constexpr buffer() = default;

  template <typename U, typename... Ts>
    requires std::derived_from<U, T> && std::constructible_from<U, Ts...> &&
             (is_sbo_compatible<U>())
  constexpr buffer(std::type_identity<U>, Ts&&... ts)
      : vtable_{
            .ptr = [](buffer* self) -> T* {
              U* u = std::launder(reinterpret_cast<U*>(self->data_.data()));
              return static_cast<T*>(u);
            },
            .cptr = [](const buffer* self) -> const T* {
              const U* u =
                  std::launder(reinterpret_cast<const U*>(self->data_.data()));
              return static_cast<const T*>(u);
            },
            .clone = [](const buffer* self, buffer* destination) -> void {
              const U* u =
                  std::launder(reinterpret_cast<const U*>(self->data_.data()));
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

  constexpr void clone(buffer& destination) const {
    (*vtable_.clone)(this, &destination);
  }

  constexpr void destroy() { vtable_.destroy(this); }

  constexpr void relocate(buffer& destination) {
    (*vtable_.relocate)(this, &destination);
  }

  constexpr const T* cptr() const { return (*vtable_.cptr)(this); }

  constexpr T* ptr() { return (*vtable_.ptr)(this); }
};

}  // namespace detail

template <class T, class A = std::allocator<T>>
class polymorphic {
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
  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> &&
             (std::derived_from<U, T> || std::same_as<U, T>)
      : alloc_(alloc) {
    if constexpr (detail::is_sbo_compatible<U>()) {
      storage_.template emplace<idx::BUFFER>(
          detail::buffer<T>(std::type_identity<U>{}, std::forward<Ts>(ts)...));
    } else {
      using cb_allocator = typename std::allocator_traits<
          A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
      using cb_traits = std::allocator_traits<cb_allocator>;
      cb_allocator cb_alloc(alloc_);
      auto* mem = cb_traits::allocate(cb_alloc, 1);
      try {
        cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
        storage_.template emplace<idx::CONTROL_BLOCK>(mem);
      } catch (...) {
        cb_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }
  }

  constexpr polymorphic()
    requires std::default_initializable<T>
      : polymorphic(std::allocator_arg, A(), std::in_place_type<T>) {}

  template <class U, class... Ts>
  explicit constexpr polymorphic(std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
      : polymorphic(std::allocator_arg, A(), std::in_place_type<U>,
                    std::forward<Ts>(ts)...) {}

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        const polymorphic& other)
      : alloc_(alloc) {
    assert(!other.valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(other.storage_.index())) {
      case idx::BUFFER:
        storage_.template emplace<idx::BUFFER>();
        std::get<idx::BUFFER>(other.storage_)
            .clone(std::get<idx::BUFFER>(storage_));
        break;
      case idx::CONTROL_BLOCK:
        storage_.template emplace<idx::CONTROL_BLOCK>(
            std::get<idx::CONTROL_BLOCK>(other.storage_)->clone(alloc_));
        break;
      case idx::EMPTY:
        unreachable();
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
    switch (static_cast<idx>(other.storage_.index())) {
      case idx::BUFFER: {
        auto& buf = std::get<idx::BUFFER>(other.storage_);
        storage_.template emplace<idx::BUFFER>();
        buf.relocate(std::get<idx::BUFFER>(storage_));
        other.reset();
        break;
      }
      case idx::CONTROL_BLOCK: {
        auto* cb = std::get<idx::CONTROL_BLOCK>(other.storage_);
        storage_.template emplace<idx::CONTROL_BLOCK>(cb);
        other.storage_.template emplace<idx::EMPTY>();
        break;
      }
    }
  }

  constexpr polymorphic(polymorphic&& other) noexcept
      : polymorphic(std::allocator_arg, other.alloc_, other) {}

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
          return *this;
        }
        case idx::CONTROL_BLOCK: {
          auto* cb = std::get<idx::CONTROL_BLOCK>(other.storage_);
          storage_.template emplace<idx::CONTROL_BLOCK>(cb);
          other.storage_.template emplace<idx::EMPTY>();
          return *this;
        }
        case idx::EMPTY:
          unreachable();
      }
    } else {
      if (alloc_ == other.alloc_) {
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER: {
            auto& buf = std::get<idx::BUFFER>(other.storage_);
            storage_.template emplace<idx::BUFFER>();
            buf.relocate(std::get<idx::BUFFER>(storage_));
            other.reset();
            return *this;
          }
          case idx::CONTROL_BLOCK: {
            auto* cb = std::get<idx::CONTROL_BLOCK>(other.storage_);
            storage_.template emplace<idx::CONTROL_BLOCK>(cb);
            other.storage_.template emplace<idx::EMPTY>();
            return *this;
          }
          case idx::EMPTY:
            unreachable();
        }
      } else {
        switch (static_cast<idx>(other.storage_.index())) {
          case idx::BUFFER:
            storage_.template emplace<idx::BUFFER>();
            std::get<idx::BUFFER>(other.storage_)
                .clone(std::get<idx::BUFFER>(storage_));
            break;
          case idx::CONTROL_BLOCK:
            storage_.template emplace<idx::CONTROL_BLOCK>(
                std::get<idx::CONTROL_BLOCK>(other.storage_)->clone(alloc_));
            break;
          case idx::EMPTY:
            unreachable();
        }
        other.reset();
      }
    }
    return *this;
  }

  constexpr T* operator->() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(storage_.index())) {
      case idx::BUFFER:
        return std::get<idx::BUFFER>(storage_).ptr();
      case idx::CONTROL_BLOCK:
        return std::get<idx::CONTROL_BLOCK>(storage_)->p_;
      case idx::EMPTY:
        unreachable();
    }
    unreachable();
  }

  constexpr const T* operator->() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(storage_.index())) {
      case idx::BUFFER:
        return std::get<idx::BUFFER>(storage_).cptr();
      case idx::CONTROL_BLOCK:
        return std::get<idx::CONTROL_BLOCK>(storage_)->p_;
      case idx::EMPTY:
        unreachable();
    }
    unreachable();
  }

  constexpr T& operator*() noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(storage_.index())) {
      case idx::BUFFER:
        return *(std::get<idx::BUFFER>(storage_).ptr());
      case idx::CONTROL_BLOCK:
        return *(std::get<idx::CONTROL_BLOCK>(storage_)->p_);
      case idx::EMPTY:
        unreachable();
    }
    unreachable();
  }

  constexpr const T& operator*() const noexcept {
    assert(!valueless_after_move());  // LCOV_EXCL_LINE
    switch (static_cast<idx>(storage_.index())) {
      case idx::BUFFER:
        return *(std::get<idx::BUFFER>(storage_).cptr());
      case idx::CONTROL_BLOCK:
        return *(std::get<idx::CONTROL_BLOCK>(storage_)->p_);
      case idx::EMPTY:
        unreachable();
    }
    unreachable();
  }

  constexpr bool valueless_after_move() const noexcept {
    return storage_.index() == idx::EMPTY;
  }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  constexpr void swap(polymorphic& other) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    // TODO(jbcoe): implement swap.
    // assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    //  std::swap(cb_, other.cb_);
    // detail::buffer<T> tmp;
    // buffer().relocate(&tmp);
    // other.buffer().relocate(&buffer());
    // tmp.relocate(&other.buffer());
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
  constexpr auto& buffer() {
    assert(storage_.index() == idx::BUFFER);  // LCOV_EXCL_LINE
    return std::get<idx::BUFFER>(storage_);
  }

  constexpr auto const& buffer() const {
    assert(storage_.index() == idx::BUFFER);  // LCOV_EXCL_LINE
    return std::get<idx::BUFFER>(storage_);
  }
  constexpr void reset() noexcept {
    switch (storage_.index()) {
      case idx::BUFFER:
        return std::get<idx::BUFFER>(storage_).destroy();
      case idx::CONTROL_BLOCK:
        return std::get<idx::CONTROL_BLOCK>(storage_)->destroy(alloc_);
    }
    storage_.template emplace<idx::EMPTY>();
  }
};  // namespace xyz

}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_
