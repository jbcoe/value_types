#include <memory>
#include <utility>

namespace xyz {
template <class T, class A = std::allocator<T>>
class indirect {
  T* p_;
  [[no_unique_address]] A alloc_;

  using t_traits = std::allocator_traits<A>;

 public:
  using value_type = T;

  indirect() {
    T* mem = t_traits::allocate(alloc_, 1);
    t_traits::construct(alloc_, mem);
    p_ = mem;
  }

  template <class... Ts>
  indirect(std::in_place_t, Ts&&... ts) {
    T* mem = t_traits::allocate(alloc_, 1);
    t_traits::construct(alloc_, mem, std::forward<Ts>(ts)...);
    p_ = mem;
  }

  indirect(const indirect& other) {
    assert(other.p_ != nullptr);
    T* mem = t_traits::allocate(alloc_, 1);
    t_traits::construct(alloc_, mem, *other);
    p_ = mem;
  }

  indirect(indirect&& other) noexcept : p_(nullptr) {
    assert(other.p_ != nullptr);
    using std::swap;
    swap(p_, other.p_);
  }

  ~indirect() { reset(); }

  indirect& operator=(const indirect& other) {
    assert(other.p_ != nullptr);
    indirect tmp(std::in_place, *other.p_);
    return *this;
  }

  indirect& operator=(indirect&& other) noexcept {
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
    using std::swap;
    swap(p_, other.p_);
  }

  friend void swap(indirect& lhs, indirect& rhs) noexcept {
    using std::swap;
    swap(lhs.p_, rhs.p_);
  }

 private:
  void reset() {
    if (p_ == nullptr) return;
    t_traits::destroy(alloc_, p_);
    t_traits::deallocate(alloc_, p_, 1);
    p_ = nullptr;
  }
};
}  // namespace xyz

template <class T>
struct std::hash<xyz::indirect<T>> {
  constexpr std::size_t operator()(const xyz::indirect<T>& key) const {
    return std::hash<typename xyz::indirect<T>::value_type>{}(*key);
  }
};
