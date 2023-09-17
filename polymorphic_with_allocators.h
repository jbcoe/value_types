#ifndef XYZ_POLYMORPHIC_WITH_ALLOCATORS_H
#define XYZ_POLYMORPHIC_WITH_ALLOCATORS_H

#include <memory>
#include <utility>

namespace xyz {

namespace detail {
template <class T, class A>
struct control_block {
  virtual ~control_block() = default;
  virtual void destroy() = 0;
  virtual T* get() noexcept = 0;
  virtual const T* get() const noexcept = 0;
  virtual control_block<T, A>* clone() = 0;
};

template <class T, class U, class A>
class direct_control_block : public control_block<T, A> {
  U u_;
  [[no_unique_address]] A alloc_;

 public:
  template <class... Ts>
  direct_control_block(A alloc, Ts&&... ts)
      : u_(std::forward<Ts>(ts)...), alloc_(alloc) {}

  T* get() noexcept override { return &u_; }

  const T* get() const noexcept override { return &u_; }

  control_block<T, A>* clone() override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc_);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    direct_control_block<T, U, A>* mem = cb_alloc_traits::allocate(cb_alloc, 1);
    cb_alloc_traits::construct(cb_alloc, mem, alloc_, u_);
    return mem;
  }

  void destroy() override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc_);
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
    cb_traits::construct(cb_alloc, mem, A{});
    cb_ = mem;
  }

  template <class U, class... Ts>
  polymorphic(std::in_place_type_t<U>, Ts&&... ts) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto* mem = cb_traits::allocate(cb_alloc, 1);
    cb_traits::construct(cb_alloc, mem, A{}, std::forward<Ts>(ts)...);
    cb_ = mem;
  }

  polymorphic(const polymorphic& other) {
    assert(other.cb_ != nullptr);

    cb_ = other.cb_->clone();
  }

  polymorphic(polymorphic&& other) noexcept {
    assert(other.cb_ != nullptr);

    using std::swap;
    swap(cb_, other.cb_);
    cb_ = nullptr;
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

    using std::swap;
    swap(cb_, other.cb_);
    return *this;
  }

  T* operator->() noexcept {
    assert(cb_ != nullptr);
    return cb_->get();
  }

  const T* operator->() const noexcept {
    assert(cb_ != nullptr);
    return cb_->get();
  }

  T& operator*() noexcept {
    assert(cb_ != nullptr);
    return *cb_->get();
  }

  const T& operator*() const noexcept {
    assert(cb_ != nullptr);
    return *cb_->get();
  }

  void swap(polymorphic& other) noexcept {
    using std::swap;
    swap(cb_, other.cb_);
  }

  friend void swap(polymorphic& lhs, polymorphic& rhs) noexcept {
    using std::swap;
    swap(lhs.cb_, rhs.cb_);
  }

 private:
  void reset() {
    if (cb_ != nullptr) {
      cb_->destroy();
    }
    cb_ = nullptr;
  }
};

}  // namespace xyz
#endif  // XYZ_POLYMORPHIC_WITH_ALLOCATORS_H
