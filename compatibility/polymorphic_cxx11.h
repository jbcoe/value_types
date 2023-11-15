#ifndef XYZ_POLYMORPHIC_H_
#define XYZ_POLYMORPHIC_H_

#include <cassert>
#include <memory>
#include <type_traits>
#include <utility>

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

struct NoPolymorphicSBO {};

template <class T>
struct in_place_type_t {};

namespace detail {
template <class T, class A>
struct control_block {
  using allocator_traits = std::allocator_traits<A>;

  typename allocator_traits::pointer p_;

  virtual ~control_block() = default;
  virtual void destroy(A& alloc) = 0;
  virtual control_block<T, A>* clone(A& alloc) = 0;
};

template <class T, class U, class A>
class direct_control_block final : public control_block<T, A> {
  U u_;

 public:
  ~direct_control_block() override = default;

  template <class... Ts>
  direct_control_block(Ts&&... ts) : u_(std::forward<Ts>(ts)...) {
    control_block<T, A>::p_ = &u_;
  }

  control_block<T, A>* clone(A& alloc) override {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<direct_control_block<T, U, A>>;
    cb_allocator cb_alloc(alloc);
    using cb_alloc_traits = std::allocator_traits<cb_allocator>;
    auto mem = cb_alloc_traits::allocate(cb_alloc, 1);
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
  using cblock_t = detail::control_block<T, A>;
  cblock_t* cb_;

  A alloc_;  // TODO: Apply EBCO

  using allocator_traits = std::allocator_traits<A>;

 public:
  using value_type = T;
  using allocator_type = A;
  using pointer = typename allocator_traits::pointer;
  using const_pointer = typename allocator_traits::const_pointer;

  polymorphic() {
    static_assert(std::is_default_constructible<T>::value, "");
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, T, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  template <
      class U, class... Ts,
      typename std::enable_if<std::is_constructible<U, Ts&&...>::value,
                              int>::type = 0,
      typename std::enable_if<std::is_copy_constructible<U>::value, int>::type =
          0,
      typename std::enable_if<std::is_base_of<T, U>::value, int>::type = 0>
  explicit polymorphic(in_place_type_t<U>, Ts&&... ts) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  template <
      class U, class... Ts,
      typename std::enable_if<std::is_constructible<U, Ts&&...>::value,
                              int>::type = 0,
      typename std::enable_if<std::is_copy_constructible<U>::value, int>::type =
          0,
      typename std::enable_if<std::is_base_of<T, U>::value, int>::type = 0>
  polymorphic(std::allocator_arg_t, const A& alloc, in_place_type_t<U>,
              Ts&&... ts)
      : alloc_(alloc) {
    using cb_allocator = typename std::allocator_traits<
        A>::template rebind_alloc<detail::direct_control_block<T, U, A>>;
    using cb_traits = std::allocator_traits<cb_allocator>;
    cb_allocator cb_alloc(alloc_);
    auto mem = cb_traits::allocate(cb_alloc, 1);
    try {
      cb_traits::construct(cb_alloc, mem, std::forward<Ts>(ts)...);
      cb_ = mem;
    } catch (...) {
      cb_traits::deallocate(cb_alloc, mem, 1);
      throw;
    }
  }

  polymorphic(const polymorphic& other)
      : alloc_(allocator_traits::select_on_container_copy_construction(
            other.alloc_)) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_->clone(alloc_);
  }

  polymorphic(std::allocator_arg_t, const A& alloc, const polymorphic& other)
      : alloc_(alloc) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_->clone(alloc_);
  }

  polymorphic(polymorphic&& other) noexcept : alloc_(other.alloc_) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_;
    other.cb_ = nullptr;
  }

  polymorphic(std::allocator_arg_t, const A& alloc,
              polymorphic&& other) noexcept
      : alloc_(alloc) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    cb_ = other.cb_;
    other.cb_ = nullptr;
  }

  ~polymorphic() { reset(); }

  polymorphic& operator=(const polymorphic& other) {
    if (this == &other) return *this;
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    if (allocator_traits::propagate_on_container_copy_assignment::value) {
      if (alloc_ != other.alloc_) {
        reset();  // using current allocator.
        alloc_ = other.alloc_;
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    cb_ = other.cb_->clone(alloc_);
    return *this;
  }

  polymorphic& operator=(polymorphic&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value ||
      allocator_traits::is_always_equal::value) {
    if (this == &other) return *this;
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE
    if (allocator_traits::propagate_on_container_move_assignment::value) {
      if (alloc_ != other.alloc_) {
        reset();  // using current allocator.
        alloc_ = other.alloc_;
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    if (alloc_ == other.alloc_) {
      std::swap(cb_, other.cb_);
    } else {
      cb_ = other.cb_->clone(alloc_);
    }
    return *this;
  }

  pointer operator->() noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  const_pointer operator->() const noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return cb_->p_;
  }

  T& operator*() noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  const T& operator*() const noexcept {
    assert(cb_ != nullptr);  // LCOV_EXCL_LINE
    return *cb_->p_;
  }

  bool valueless_after_move() const noexcept { return cb_ == nullptr; }

  allocator_type get_allocator() const noexcept { return alloc_; }

  void swap(polymorphic& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    assert(other.cb_ != nullptr);  // LCOV_EXCL_LINE

    if (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects we can swap both.
      std::swap(alloc_, other.alloc_);
      std::swap(cb_, other.cb_);
      return;
    } else /*  */ {
      if (alloc_ == other.alloc_) {
        std::swap(cb_, other.cb_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend void swap(polymorphic& lhs,
                   polymorphic& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
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

#endif  // XYZ_POLYMORPHIC_H_
