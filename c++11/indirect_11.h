#ifndef XYZ_INDIRECT_11_H
#define XYZ_INDIRECT_11_H

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

template <class T, class A>
class indirect;

template <class>
struct is_indirect : std::false_type {};

template <class T, class A>
struct is_indirect<indirect<T, A>> : std::true_type {};

template <class T, class A = std::allocator<T>>
class indirect {
  using allocator_traits = std::allocator_traits<A>;

 public:
  using value_type = T;
  using allocator_type = A;
  using pointer = typename allocator_traits::pointer;
  using const_pointer = typename allocator_traits::const_pointer;

  template <class AA = A,
            typename std::enable_if<std::is_default_constructible<AA>::value,
                                    int>::type = 0>
  indirect() {
    static_assert(std::is_default_constructible<T>::value, "");
    p_ = construct_from(alloc_);
  }

  indirect(std::allocator_arg_t, const A& alloc) : alloc_(alloc) {
    static_assert(std::is_default_constructible<T>::value, "");
    p_ = construct_from(alloc_);
  }

  template <
      class U, class... Us,
      typename std::enable_if<std::is_constructible<T, U&&, Us&&...>::value,
                              int>::type = 0,
      class AA = A,
      typename std::enable_if<std::is_default_constructible<AA>::value,
                              int>::type = 0,
      typename std::enable_if<
          !std::is_same<typename std::remove_const<
                            typename std::remove_reference<U>::type>::type,
                        indirect>::value,
          int>::type = 0>
  explicit indirect(U&& u, Us&&... us) {
    p_ = construct_from(alloc_, std::forward<U>(u), std::forward<Us>(us)...);
  }

  template <
      class U, class... Us,
      typename std::enable_if<std::is_constructible<T, U&&, Us&&...>::value,
                              int>::type = 0,
      typename std::enable_if<
          !std::is_same<typename std::remove_const<
                            typename std::remove_reference<U>::type>::type,
                        indirect>::value,
          int>::type = 0>
  indirect(std::allocator_arg_t, const A& alloc, U&& u, Us&&... us)
      : alloc_(alloc) {
    p_ = construct_from(alloc_, std::forward<U>(u), std::forward<Us>(us)...);
  }

  indirect(const indirect& other)
      : alloc_(allocator_traits::select_on_container_copy_construction(
            other.alloc_)) {
    static_assert(std::is_copy_constructible<T>::value, "");
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    p_ = construct_from(alloc_, *other);
  }

  indirect(std::allocator_arg_t, const A& alloc, const indirect& other)
      : alloc_(alloc) {
    static_assert(std::is_copy_constructible<T>::value, "");
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    p_ = construct_from(alloc_, *other);
  }

  indirect(indirect&& other) noexcept : p_(nullptr), alloc_(other.alloc_) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    std::swap(p_, other.p_);
  }

  indirect(std::allocator_arg_t, const A& alloc, indirect&& other) noexcept
      : p_(nullptr), alloc_(alloc) {
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    std::swap(p_, other.p_);
  }

  ~indirect() { reset(); }

  indirect& operator=(const indirect& other) {
    if (this == &other) return *this;
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    static_assert(std::is_copy_constructible<T>::value, "");
    static_assert(std::is_copy_assignable<T>::value, "");
    if (allocator_traits::propagate_on_container_copy_assignment::value) {
      if (alloc_ != other.alloc_) {
        reset();  // using current allocator.
        alloc_ = other.alloc_;
      }
    }
    if (alloc_ == other.alloc_) {
      if (p_ != nullptr) {
        *p_ = *other.p_;
        return *this;
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    p_ = construct_from(alloc_, *other);
    return *this;
  }

  indirect& operator=(indirect&& other) noexcept(
      allocator_traits::propagate_on_container_move_assignment::value ||
      allocator_traits::is_always_equal::value) {
    if (this == &other) return *this;
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE
    static_assert(std::is_copy_constructible<T>::value, "");

    if (allocator_traits::propagate_on_container_move_assignment::value) {
      if (alloc_ != other.alloc_) {
        reset();  // using current allocator.
        alloc_ = other.alloc_;
      }
    }
    reset();  // We may not have reset above and it's a no-op if valueless.
    if (alloc_ == other.alloc_) {
      std::swap(p_, other.p_);
    } else {
      p_ = construct_from(alloc_, std::move(*other));
    }
    return *this;
  }

  const T& operator*() const& noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return *p_;
  }

  T& operator*() & noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return *p_;
  }

  T&& operator*() && noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  const T&& operator*() const&& noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return std::move(*p_);
  }

  const_pointer operator->() const noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return p_;
  }

  pointer operator->() noexcept {
    assert(p_ != nullptr);  // LCOV_EXCL_LINE
    return p_;
  }

  bool valueless_after_move() const noexcept { return p_ == nullptr; }

  allocator_type get_allocator() const noexcept { return alloc_; }

  void swap(indirect& other) noexcept(
      std::allocator_traits<A>::propagate_on_container_swap::value ||
      std::allocator_traits<A>::is_always_equal::value) {
    assert(p_ != nullptr);        // LCOV_EXCL_LINE
    assert(other.p_ != nullptr);  // LCOV_EXCL_LINE

    if (allocator_traits::propagate_on_container_swap::value) {
      // If allocators move with their allocated objects we can swap both.
      std::swap(alloc_, other.alloc_);
      std::swap(p_, other.p_);
      return;
    } else /*  */ {
      if (alloc_ == other.alloc_) {
        std::swap(p_, other.p_);
      } else {
        unreachable();  // LCOV_EXCL_LINE
      }
    }
  }

  friend void swap(indirect& lhs,
                   indirect& rhs) noexcept(noexcept(lhs.swap(rhs))) {
    lhs.swap(rhs);
  }

  template <class U, class AA>
  friend bool operator==(const indirect<T, A>& lhs,
                         const indirect<U, AA>& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs == *rhs;
  }

  template <class U, class AA>
  friend bool operator!=(const indirect<T, A>& lhs,
                         const indirect<U, AA>& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs != *rhs;
  }

  template <class U, class AA>
  friend bool operator<(const indirect<T, A>& lhs, const indirect<U, AA>& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs < *rhs;
  }

  template <class U, class AA>
  friend bool operator>(const indirect<T, A>& lhs, const indirect<U, AA>& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs > *rhs;
  }

  template <class U, class AA>
  friend bool operator<=(const indirect<T, A>& lhs,
                         const indirect<U, AA>& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs <= *rhs;
  }

  template <class U, class AA>
  friend bool operator>=(const indirect<T, A>& lhs,
                         const indirect<U, AA>& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs >= *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator==(const indirect<T, A>& lhs, const U& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs == rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator==(const U& lhs, const indirect<T, A>& rhs) {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs == *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator!=(const indirect<T, A>& lhs, const U& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs != rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator!=(const U& lhs, const indirect<T, A>& rhs) {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs != *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator<(const indirect<T, A>& lhs, const U& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs < rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator<(const U& lhs, const indirect<T, A>& rhs) {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs < *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator>(const indirect<T, A>& lhs, const U& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs > rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator>(const U& lhs, const indirect<T, A>& rhs) {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs > *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator<=(const indirect<T, A>& lhs, const U& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs <= rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator<=(const U& lhs, const indirect<T, A>& rhs) {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs <= *rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator>=(const indirect<T, A>& lhs, const U& rhs) {
    assert(!lhs.valueless_after_move());  // LCOV_EXCL_LINE
    return *lhs >= rhs;
  }

  template <class U,
            typename std::enable_if<!is_indirect<U>::value, int>::type = 0>
  friend bool operator>=(const U& lhs, const indirect<T, A>& rhs) {
    assert(!rhs.valueless_after_move());  // LCOV_EXCL_LINE
    return lhs >= *rhs;
  }

 private:
  pointer p_;

  A alloc_;  // TODO: Apply EBCO

  void reset() noexcept {
    if (p_ == nullptr) return;
    destroy_with(alloc_, p_);
    p_ = nullptr;
  }

  template <typename... Ts>
  static T* construct_from(A alloc, Ts&&... ts) {
    T* mem = allocator_traits::allocate(alloc, 1);
    try {
      allocator_traits::construct(alloc, mem, std::forward<Ts>(ts)...);
      return mem;
    } catch (...) {
      allocator_traits::deallocate(alloc, mem, 1);
      throw;
    }
  }

  static void destroy_with(A alloc, T* p) {
    allocator_traits::destroy(alloc, p);
    allocator_traits::deallocate(alloc, p, 1);
  }
};

}  // namespace xyz

template <class T, class Alloc>
struct std::hash<xyz::indirect<T, Alloc>> {
  std::size_t operator()(const xyz::indirect<T, Alloc>& key) const {
    return std::hash<typename xyz::indirect<T, Alloc>::value_type>{}(*key);
  }
};

#endif  // XYZ_INDIRECT_11_H
