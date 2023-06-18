#ifndef XYZ_POLYMORPHIC_H_
#define XYZ_POLYMORPHIC_H_

#include <exception>
#include <utility>

#include "copy_and_delete.h"

namespace xyz {

class bad_polymorphic_access : public std::exception {
 public:
  const char *what() const noexcept override {
    return "bad_polymorphic_access";
  }
};

class bad_polymorphic_construction : public std::exception {
 public:
  bad_polymorphic_construction() noexcept = default;

  const char* what() const noexcept override {
    return "Dynamic and static type mismatch in polymorphic "
           "construction";
  }
};

template <typename T, typename A = std::allocator<T>>
class control_block {
 public:
  virtual ~control_block() = default;
  virtual control_block *clone() const = 0;
  virtual void *get() noexcept = 0;
  virtual const void *get() const noexcept = 0;
  virtual T *operator->() noexcept = 0;
};

template <typename T, typename A = std::allocator<T>>
class polymorphic {
  control_block<T, A> *cb_;

 public:
  // Default constructor
  polymorphic();

  polymorphic(std::nullptr_t);

  // Copy constructor
  polymorphic(const polymorphic &p);

  // Move constructor
  polymorphic(polymorphic &&p);

  // Value constructor
  template <typename U, typename... Ts>
  polymorphic(std::in_place_type_t<U>, Ts &&...ts);

  // Pointer constructor
  template <typename U, typename C = default_copy<T>,
            typename D = default_delete<T>>
  polymorphic(U *, C c = C{}, D d = D{});

  // Converting constructors
  template <typename U>
  polymorphic(const polymorphic<U, A> &p);

  template <typename U>
  polymorphic(polymorphic<U, A> &&p);

  // Destruction.
  ~polymorphic();

  // Assignment
  polymorphic &operator=(const polymorphic &p);

  polymorphic &operator=(polymorphic &&p);

  // Converting assignment
  template <typename U>
  polymorphic &operator=(const polymorphic<U, A> &p);

  template <typename U>
  polymorphic &operator=(polymorphic<U, A> &&p);

  // Observers
  operator bool() const noexcept;

  // Accessors
  const T *operator->() const noexcept;
  const T &operator*() const noexcept;

  // Modifiers
  T *operator->() noexcept;
  T &operator*() noexcept;

  // Member swap
  void swap(polymorphic &p) noexcept;

  // Non-member swap
  friend void swap(polymorphic &lhs, polymorphic &rhs) noexcept;

  // Factory methods
  template <typename T_, typename U, typename A_, typename... Ts>
  friend polymorphic<T_, A_> make_polymorphic(Ts &&...ts);
};

template <class T, class U = T, class... Ts>
polymorphic<T> make_polymorphic(
    Ts&&... ts) {
  polymorphic<T> p;
  return p;
}

}  // namespace xyz

#endif  // XYZ_POLYMORPHIC_H_