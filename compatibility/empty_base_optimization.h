#include <type_traits>

namespace xyz {
namespace detail {

template <class T, bool CanBeEmptyBaseClass =
                       std::is_empty<T>::value && !std::is_final<T>::value>
class empty_base_optimization {
 protected:
  empty_base_optimization() = default;
  empty_base_optimization(const T& t) : t_(t) {}
  empty_base_optimization(T&& t) : t_(std::move(t)) {}
  T& get() noexcept { return t_; }
  const T& get() const noexcept { return t_; }
  T t_;
};

template <class T>
class empty_base_optimization<T, true> : private T {
 protected:
  empty_base_optimization() = default;
  empty_base_optimization(const T& t) : T(t) {}
  empty_base_optimization(T&& t) : T(std::move(t)) {}
  T& get() noexcept { return *this; }
  const T& get() const noexcept { return *this; }
};

}  // namespace detail
}  // namespace xyz
