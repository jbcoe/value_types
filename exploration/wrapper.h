#include <concepts>

namespace xyz {

// an approximation of indirect without allocators.
template <typename T>
class wrapper {
  T* t_;

 public:
  wrapper() : t_(new T()) {}

  wrapper(const wrapper& other) : t_(new T(*other.t_)) {}

  ~wrapper() { delete t_; }

  template <typename U>
  bool operator==(const wrapper<U>& other) const {
    return *t_ == *other.t_;
  }
};

// an approximation of a constrained indirect without allocators.
template <typename T>
class constrained_wrapper {
  T* t_;

 public:
  template <typename TT = T>
  constrained_wrapper()
    requires std::default_initializable<TT>
      : t_(new T()) {}

  constrained_wrapper(const constrained_wrapper& other)
    requires false;

  template <typename TT = T>
  constrained_wrapper(const constrained_wrapper& other)
    requires std::copy_constructible<TT>
      : t_(new T(*other.t_)) {}

  ~constrained_wrapper() { delete t_; }

  template <typename U>
  bool operator==(const constrained_wrapper<U>& other) const
    requires std::equality_comparable_with<T, U>
  {
    return *t_ == *other.t_;
  }
};

}  // namespace xyz
