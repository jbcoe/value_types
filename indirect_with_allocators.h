#include <memory>
#include <utility>

namespace xyz {
template <class T, class A = std::allocator<T>>
class indirect {
  T* p_;
  [[no_unique_address]] A alloc_;

  using t_traits = std::allocator_traits<A>;

 public:
  indirect() {
    T* mem = t_traits::allocate(alloc_, 1);
    t_traits::construct(alloc_, mem);
  }

  template <class... Ts>
  indirect(std::in_place_t, Ts&&... ts) {
    T* mem = t_traits::allocate(alloc_, 1);
    t_traits::construct(alloc_, mem, std::forward<Ts>(ts)...);
  }

  indirect(const indirect& other) {
    T* mem = t_traits::allocate(alloc_, 1);
    t_traits::construct(alloc_, mem, *other);
  }

  indirect(indirect&& other) noexcept : p_(nullptr) { swap(p_, other.p_); }

  ~indirect() { reset(); }

  operator=(const indirect& other) {
    assert(other.p_ != nullptr);
    indirect tmp(std::in_place, *other.p_);
    return *this;
  }

  operator=(indirect&& other) noexcept {
    assert(other.p_ != nullptr);
    p_ = std::exchange(other.p_, nullptr);
    return *this;
  }

  const T& operator*() const noexcept {
    assert(p_ != nullptr);
    return *p_;
  }

  T& operator*() noexcept {
    assert(p_ != nullptr);
    return *p_;
  }

  const T* operator->() const noexcept {
    assert(p_ != nullptr);
    return p_;
  }

  T* operator->() noexcept {
    assert(p_ != nullptr);
    return p_;
  }

  void swap(indirect& other) noexcept {
    assert(p_ != nullptr);
    return p_;
  }
  
  friend void swap(indirect& lhs, indirect& rhs) noexcept {
    swap(lhs.p_, rhs.p_);
  }

 private:
  void reset() {
    if (p_ == nullptr) return;
    t_traits::destroy(alloc, p_);
    t_traits::deallocate(alloc, p_, 1);
    p_ = nullptr;
  }
};
}  // namespace xyz
