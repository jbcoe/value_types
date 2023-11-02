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
#include <memory>
#include <utility>
#include <variant>

namespace xyz {

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
  using ptr_fn = T* (*)(struct buffer*);
  using cptr_fn = const T* (*)(const struct buffer*);
  using clone_fn = void (*)(const struct buffer*, struct buffer*);
  using relocate_fn = void (*)(struct buffer*, struct buffer*);
  using destroy_fn = void (*)(struct buffer*);

  ptr_fn* ptr_ = nullptr;
  cptr_fn* cptr_ = nullptr;
  clone_fn* clone_ = nullptr;
  relocate_fn* relocate_ = nullptr;
  destroy_fn* destroy_ = nullptr;
  std::array<std::byte, PolymorphicBufferCapacity> data_;

  constexpr buffer() = default;

  template <typename U, typename... Ts>
    requires std::derived_from<U, T> && std::constructible_from<U, Ts...> &&
             (is_sbo_compatible<U>())
  constexpr buffer(Ts&&... ts) {
    new (data_.data()) U(std::forward<Ts>(ts)...);
    ptr_ = [](struct buffer* self) -> T* {
      return static_cast<T*>(
          std::launder(reinterpret_cast<U*>(self->data_.data())));
    };
    cptr_ = [](const struct buffer* self) -> const T* {
      return static_cast<const T*>(
          std::launder(reinterpret_cast<const U*>(self->data_.data())));
    };
    clone_ = [](const struct buffer* self, struct buffer* destination) -> void {
      new (destination->data_.data())
          U(*std::launder(reinterpret_cast<const U*>(self->data_.data())));
      destination->ptr_ = self->ptr_;
      destination->clone_ = self->clone_;
      destination->relocate_ = self->relocate_;
      destination->destroy_ = self->destroy_;
    };
    relocate_ = [](struct buffer* self, struct buffer* destination) -> void {
      new (destination->data_.data())
          U(std::move(*std::launder(reinterpret_cast<U*>(self->data_.data()))));
      destination->ptr_ = self->ptr_;
      destination->clone_ = self->clone_;
      destination->relocate_ = self->relocate_;
      destination->destroy_ = self->destroy_;
    };
    destroy_ = [](struct buffer* self) -> void {
      std::launder(reinterpret_cast<U*>(self->data_.data()))->~U();
    };
  }

  constexpr void clone(struct buffer* destination) const {
    clone_(this, destination);
  }

  constexpr void destroy() { destroy_(this); }

  constexpr void relocate(struct buffer* destination) {
    relocate_(this, destination);
  }

  constexpr T* ptr() const { return ptr_(this); }
};

}  // namespace detail

template <class T, class A = std::allocator<T>>
class polymorphic {
  std::variant<std::monostate, detail::buffer<T>, detail::control_block<T, A>*>
      storage_;
  enum class idx { EMPTY, BUFFER, CONTROL_BLOCK };

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
    if constexpr (is_sbo_compatible<T>()) {
      storage_.emplace<idx::BUFFER>(detail::buffer<T>());
    } else {
      using cb_allocator = typename std::allocator_traits<
          A>::template rebind_alloc<detail::direct_control_block<T, T, A>>;
      using cb_traits = std::allocator_traits<cb_allocator>;
      cb_allocator cb_alloc(alloc_);
      auto* mem = cb_traits::allocate(cb_alloc, 1);
      try {
        cb_traits::construct(cb_alloc, mem);
        storage_.emplace<idx::CONTROL_BLOCK>(mem);
      } catch (...) {
        cb_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }
  }

  template <class U, class... Ts>
  explicit constexpr polymorphic(std::in_place_type_t<U>, Ts&&... ts)
    requires std::constructible_from<U, Ts&&...> &&
             std::copy_constructible<U> && std::derived_from<U, T>
  {
    if constexpr (is_sbo_compatible<U>()) {
      storage_.emplace<idx::BUFFER>(detail::buffer<U>(std::forward<Ts>(ts)...));
    } else {
      using cb_allocator = typename std::allocator_traits<
          A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
      using cb_traits = std::allocator_traits<cb_allocator>;
      cb_allocator cb_alloc(alloc_);
      auto* mem = cb_traits::allocate(cb_alloc, 1);
      try {
        cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
        storage_.emplace<idx::CONTROL_BLOCK>(mem);
      } catch (...) {
        cb_traits::deallocate(cb_alloc, mem, 1);
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
    if constexpr (is_sbo_compatible<U>()) {
      storage_.emplace<idx::BUFFER>(detail::buffer<U>(std::forward<Ts>(ts)...));
    } else {
      using cb_allocator = typename std::allocator_traits<
          A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
      using cb_traits = std::allocator_traits<cb_allocator>;
      cb_allocator cb_alloc(alloc_);
      auto* mem = cb_traits::allocate(cb_alloc, 1);
      try {
        cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
        storage_.emplace<idx::CONTROL_BLOCK>(mem);
      } catch (...) {
        cb_traits::deallocate(cb_alloc, mem, 1);
        throw;
      }
    }
  }

  constexpr polymorphic(const polymorphic& other)
      : alloc_(allocator_traits::select_on_container_copy_construction(
            other.alloc_)) {
    // assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    // cb_ = other.cb_->clone(alloc_);
    other.buffer_.clone(&buffer_);
  }

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        const polymorphic& other)
      : alloc_(alloc) {
    // assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_->clone(alloc_);
  }

  constexpr polymorphic(polymorphic&& other) noexcept : alloc_(other.alloc_) {
    // assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    // cb_ = std::exchange(other.cb_, nullptr);
    other.buffer_.relocate(&buffer_);
  }

  constexpr polymorphic(std::allocator_arg_t, const A& alloc,
                        polymorphic&& other) noexcept
      : alloc_(alloc) {
    // assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = std::exchange(other.cb_, nullptr);
  }

  constexpr ~polymorphic() { reset(); }

  constexpr polymorphic& operator=(const polymorphic& other) {
    // assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
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
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    reset();
    if constexpr (allocator_traits::propagate_on_container_move_assignment::
                      value) {
      alloc_ = other.alloc_;
      // cb_ = std::exchange(other.cb_, nullptr);
      other.buffer_.relocate(&buffer_);
    } else {
      if (alloc_ == other.alloc_) {
        // cb_ = std::exchange(other.cb_, nullptr);
        other.buffer_.relocate(&buffer_);
    } else {
        // cb_ = other.cb_->clone(alloc_);
        other.buffer_.clone(&buffer_);
        other.reset();
      }
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
    switch (storage_.index()) {
      case idx::EMPTY:
        assert("Valueless after move");  // LCOV_EXCL_LINE
        std::unreachable();
      case idx::BUFFER:
        return storage_.template get<idx::BUFFER>().ptr();
      case idx::CONTROL_BLOCK:
        return storage_.template get<idx::CONTROL_BLOCK>()->p_;
    }
  }

  constexpr const T* operator->() const noexcept {
    switch (storage_.index()) {
      case idx::EMPTY:
        assert("Valueless after move");  // LCOV_EXCL_LINE
        std::unreachable();
      case idx::BUFFER:
        return storage_.template get<idx::BUFFER>().cptr();
      case idx::CONTROL_BLOCK:
        return storage_.template get<idx::CONTROL_BLOCK>()->p_;
    }
  }

  constexpr T& operator*() noexcept {
    return *operator->();
  }

  constexpr const T& operator*() const noexcept {
    return *operator->();
  }

  constexpr bool valueless_after_move() const noexcept {
    return storage_.index() == idx::EMPTY;
  }

  constexpr allocator_type get_allocator() const noexcept { return alloc_; }

  constexpr void swap(polymorphic& other) noexcept(
      allocator_traits::propagate_on_container_swap::value ||
      allocator_traits::is_always_equal::value) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    // std::swap(cb_, other.cb_);
    detail::buffer<T> tmp;
    buffer_.relocate(&tmp);
    other.buffer_.relocate(&buffer_);
    tmp.relocate(&other.buffer_);
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
    // if (cb_ != nullptr) {
    //   cb_->destroy(alloc_);
    //   cb_ = nullptr;
    // }
    buffer_.destroy();
  }
};  // namespace xyz

}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_