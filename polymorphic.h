#ifndef XYZ_POLYMORPHIC_WITH_ALLOCATORS_H
#define XYZ_POLYMORPHIC_WITH_ALLOCATORS_H

#include <memory>
#include <utility>

namespace xyz {

namespace detail {
template <class T, class A>
struct control_block {
  T* p_;
  virtual ~control_block() = default;
  virtual void destroy(A& alloc) = 0;
  virtual control_block<T, A>* clone(A& alloc) = 0;
};

template <class T, class U, class A>
class direct_control_block : public control_block<T, A> {
  U u_;

 public:
  template <class... Ts>
  direct_control_block(Ts&&... ts) : u_(std::forward<Ts>(ts)...) {
    control_block<T, A>::p_ = &u_;
  }

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

}  // namespace detail

template <class T, class A = std::allocator<T>>
class polymorphic {
  detail::control_block<T, A>* cb_;
  [[no_unique_address]] A alloc_;

 public:
  using value_type = T;
  using allocator_type = A;

  polymorphic() {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, T, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto* mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  template <class U, class... Ts>
  polymorphic(std::in_place_type_t<U>, Ts&&... ts) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto* mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  template <class U, class... Ts>
  polymorphic(std::allocator_arg_t, const A& alloc, std::in_place_type_t<U>,
              Ts&&... ts)
      : alloc_(alloc) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto* mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  polymorphic(const polymorphic& other) : alloc_(other.alloc_) {
    assert(other.cb_ != nullptr);
    cb_ = other.cb_->clone(alloc_);
  }

  polymorphic(polymorphic&& other) noexcept : alloc_(std::move(other.alloc_)) {
    assert(other.cb_ != nullptr);
    cb_ = std::exchange(other.cb_, nullptr);
  }

  ~polymorphic() { reset(); }

  polymorphic& operator=(const polymorphic& other) {
    assert(other.cb_ != nullptr);
    polymorphic tmp(other);
    swap(tmp);
    return *this;
  }

  polymorphic& operator=(polymorphic&& other) noexcept {
    assert(other.cb_ != nullptr);
    reset();
    alloc_ = std::move(other.alloc_);
    cb_ = std::exchange(other.cb_, nullptr);
    return *this;
  }

  constexpr T* operator->() noexcept {
    assert(cb_ != nullptr);
    return cb_->p_;
  }

  constexpr const T* operator->() const noexcept {
    assert(cb_ != nullptr);
    return cb_->p_;
  }

  constexpr T& operator*() noexcept {
    assert(cb_ != nullptr);
    return *cb_->p_;
  }

  constexpr const T& operator*() const noexcept {
    assert(cb_ != nullptr);
    return *cb_->p_;
  }

  constexpr void swap(polymorphic& other) noexcept {
    using std::swap;
    swap(cb_, other.cb_);
  }

  friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept {
    using std::swap;
    swap(lhs.cb_, rhs.cb_);
  }

  constexpr bool valueless_after_move() const noexcept {
    return cb_ == nullptr;
  }

 private:
  void reset() noexcept {
    if (cb_ != nullptr) {
      cb_->destroy(alloc_);
      cb_ = nullptr;
    }
  }
};

}  // namespace xyz
#endif  // XYZ_POLYMORPHIC_WITH_ALLOCATORS_H
