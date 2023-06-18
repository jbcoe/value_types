#ifndef XYZ_INDIRECT_H_
#define XYZ_INDIRECT_H_

#include <exception>
#include <utility>

#include "copy_and_delete.h"

namespace xyz {

class bad_indirect_access : public std::exception {
 public:
  const char *what() const noexcept override { return "bad_indirect_access"; }
};

template <class T, class = void>
struct copier_traits_deleter_base {};

template <class T>
struct copier_traits_deleter_base<T, std::void_t<typename T::deleter_type>> {
  using deleter_type = typename T::deleter_type;
};

template <class U, class V>
struct copier_traits_deleter_base<U *(*)(V)> {
  using deleter_type = void (*)(U *);
};

template <class T>
struct copier_traits : copier_traits_deleter_base<T, void> {};

template <typename T, typename A = std::allocator<T>,
          typename C = xyz::default_copy<T>,
          typename D = typename copier_traits<C>::deleter_type>
class indirect {
  T *ptr_;
  [[no_unique_address]] A allocator_;
  [[no_unique_address]] C copier_;
  [[no_unique_address]] D deleter_;

 public:
  using value_type = T;
  using allocator_type = A;
  using copier_type = C;
  using deleter_type = D;

  // Default constructor
  indirect();

  indirect(std::nullptr_t);

  // Copy constructor
  indirect(const indirect &p);

  // Move constructor
  indirect(indirect &&p);

  // Value constructor
  template <typename... Ts>
  indirect(std::in_place_t, Ts &&...ts);

  // Pointer constructor
  indirect(T *, C c = C{}, D d = D{});

  // Destruction.
  ~indirect();

  // Assignment
  indirect &operator=(const indirect &p);

  indirect &operator=(indirect &&p);

  // Observers
  operator bool() const noexcept;
  bool has_value() const noexcept;

  // Accessors
  const T *operator->() const noexcept;
  const T &operator*() const noexcept;
  const T &value() const;

  // Modifiers
  T *operator->() noexcept;
  T &operator*() noexcept;
  T &value();

  // Allocator access
  const allocator_type &get_allocator() const noexcept;
  allocator_type &get_allocator() noexcept;

  // Copier access
  const copier_type &get_copier() const noexcept;
  copier_type &get_copier() noexcept;

  // Deleter access
  const deleter_type &get_deleter() const noexcept;
  deleter_type &get_deleter() noexcept;

  // Member swap
  void swap(indirect &p) noexcept;

  // Non-member swap
  friend void swap(indirect &lhs, indirect &rhs) noexcept {
    lhs.swap(rhs);
  }
};

template <class T, class... Ts>
constexpr indirect<T> make_indirect(Ts&&... ts) {
  return indirect<T>(std::in_place_t{}, std::forward<Ts>(ts)...);
}

}  // namespace xyz

#endif  // XYZ_INDIRECT_H_