#ifndef XYZ_INDIRECT_H_
#define XYZ_INDIRECT_H_

#include <exception>
#include <memory>
#include <utility>

#include "copy_and_delete.h"

namespace xyz {

template <class T, class = void>
struct copier_traits_deleter_base {};

/// \struct copier_traits_deleter_base<T, std::void_t<typename T::deleter_type>>
///     Helper specialisation of copier traits which defaults the deleter type
///     to T::deleter_type when present.
template <class T>
struct copier_traits_deleter_base<T, std::void_t<typename T::deleter_type>> {
  /// The associated deleter to be used with this copier.
  using deleter_type = typename T::deleter_type;
};

/// \struct copier_traits_deleter_base<U* (*)(V)>
///     Helper specialisation of copier traits which allows defaulting the
///     deleter type with function pointer equivalent matching signature of the
///     function when used with function pointer copier.
template <class U, class V>
struct copier_traits_deleter_base<U* (*)(V)> {
  /// The associated deleter to be used with this copier.
  using deleter_type = void (*)(U*);
};

// The user may specialize copier_traits<T> per [namespace.std]/2.
template <class T>
struct copier_traits : copier_traits_deleter_base<T, void> {};
    
/// \class bad_indirect_access
///     Exception type thrown upon a accessing an indirect with no
///     underlying value assigned.
class bad_indirect_access : public std::exception {
 public:
  /// Message describing the error.
  const char* what() const noexcept override {
    return "bad_indirect_access";
  }
};

namespace detail {

template <typename T, typename A, typename... Args>
constexpr T* allocate_object(A& a, Args&&... args) {
  using t_allocator =
      typename std::allocator_traits<A>::template rebind_alloc<T>;
  using t_traits = std::allocator_traits<t_allocator>;
  t_allocator t_alloc(a);
  T* mem = t_traits::allocate(t_alloc, 1);
  try {
    t_traits::construct(t_alloc, mem, std::forward<Args>(args)...);
    return mem;
  } catch (...) {
    t_traits::deallocate(t_alloc, mem, 1);
    throw;
  }
}

template <typename T, typename A>
constexpr void deallocate_object(A& a, T* p) {
  using t_allocator =
      typename std::allocator_traits<A>::template rebind_alloc<T>;
  using t_traits = std::allocator_traits<t_allocator>;
  t_allocator t_alloc(a);
  t_traits::destroy(t_alloc, p);
  t_traits::deallocate(t_alloc, p, 1);
};

/// \struct allocator_delete
///     Deleter type specialised for use with allocator types.
template <class T, class A>
struct allocator_delete : A {
  /// Construct with an allocator.
  constexpr allocator_delete(A& a) : A(a) {}

  /// Delete the input via the underlying allocator.
  constexpr void operator()(T* ptr) const noexcept {
    static_assert(0 < sizeof(T), "can't delete an incomplete type");
    detail::deallocate_object(*this, ptr);
  }
};

/// \struct allocator_copy
///     Copier type specialised for use with allocator types.
template <class T, class A>
struct allocator_copy : A {
  /// Construct with an allocator.
  constexpr allocator_copy(A& a) : A(a) {}

  /// The associated deleter to be used with this copier.
  using deleter_type = allocator_delete<T, A>;

  /// Create a copy of the input via the underlying allocator.
  constexpr T* operator()(T const& t) const {
    return detail::allocate_object<T>(*this, t);
  }
};

/// \struct exchange_on_move_ptr
///     Thin wrapper for a pointer to ensure moving of pointers results in a
///     exchange with nullptr.  Use ensures containing classes can rely on the
///     rule of zero for special memmber functions.
template <typename T>
struct exchange_on_move_ptr {
  /// Constructs an empty exchange_on_move_ptr.
  constexpr exchange_on_move_ptr() noexcept = default;

  /// Initialise the underlying pointer from the input.
  constexpr exchange_on_move_ptr(T* p) noexcept : ptr(p) {}

  /// Default copy construction of the underlying pointer.
  constexpr exchange_on_move_ptr(exchange_on_move_ptr const&) noexcept =
      default;

  /// Move construction, exchanges the underlying pointers.
  /// \note Resulting right hand side is null after the move.
  constexpr exchange_on_move_ptr(exchange_on_move_ptr&& rhs) noexcept
      : ptr(std::exchange(rhs.ptr, nullptr)) {}

  /// Default copy assignment of the underlying pointer.
  constexpr exchange_on_move_ptr& operator=(
      exchange_on_move_ptr const&) noexcept = default;

  /// Move assignment, exchanges the underlying pointers.
  /// \note Resulting right hand side is null after the move.
  constexpr exchange_on_move_ptr& operator=(
      exchange_on_move_ptr&& rhs) noexcept {
    ptr = std::exchange(rhs.ptr, nullptr);
    return *this;
  }

  /// Allows for copy-assignment from a raw pointer.
  constexpr exchange_on_move_ptr& operator=(T* p) noexcept {
    ptr = p;
    return *this;
  }

  /// Implicit conversion to underlying raw pointer type.
  constexpr explicit(false) operator T*() const noexcept { return ptr; }

  /// Alows for exchanging with any input pointer type.
  friend T* exchange(exchange_on_move_ptr& lhs, auto rhs) {
    return std::exchange(lhs.ptr, rhs);
  }

  T* ptr = nullptr;  ///< Underlying pointer to be exchanged.
};

template <typename T, typename C, typename D>
struct indirect_base;

/// \class indirect_base
///     We must specialise to handle the case where copier and deleter are the
///     same object and have no size.  In this case this we have
///     [[no_unique_address]] pointing to two objects of the same type which is
///     not allowed by the standard.  To support this we specialises away the
///     unnecessary duplication of the object to avoid unnecessary storage and
///     in-doing so resolve the any issues around use of [[no_unique_address]]:
///     https://en.cppreference.com/w/cpp/language/attributes/no_unique_address
/// \tparam T The underluing value type
/// \tparam CD The combined copier and deleter object
template <typename T, typename CD>
struct indirect_base<T, CD, CD> {
  exchange_on_move_ptr<T> mValue;  ///< The indirectly owned value.

#if defined (_MSC_VER)
  [[msvc::no_unique_address]] CD
      mCopierDeleterCombined;  ///< Functor customising the copying and deleting
                               ///< of the undelrying value.
#else
  [[no_unique_address]] CD
      mCopierDeleterCombined;  ///< Functor customising the copying and deleting
                               ///< of the undelrying value.
#endif

#if (__cpp_explicit_this_parameter >= 202110L)
  /// Access the copier.
  template <typename Self>
  [[nodiscard]] std::copy_cvref_t<Self, auto> getC(this Self&& self) {
    return mCopierDeleterCombined;
  }

  /// Access the deleter.
  template <typename Self>
  [[nodiscard]] std::copy_cvref_t<Self, auto> getD(this Self&& self) {
    return mCopierDeleterCombined;
  }
#else
  /// Access the copier.
  [[nodiscard]] auto& getC() & { return mCopierDeleterCombined; }

  /// Access the copier.
  [[nodiscard]] auto const& getC() const& { return mCopierDeleterCombined; }

  /// Access the copier.
  [[nodiscard]] auto&& getC() && { return std::move(mCopierDeleterCombined); }

  /// Access the copier.
  [[nodiscard]] auto const&& getC() const&& {
    return std::move(mCopierDeleterCombined);
  }

  /// Access the deleter.
  [[nodiscard]] auto& getD() & { return mCopierDeleterCombined; }

  /// Access the deleter.
  [[nodiscard]] auto const& getD() const& { return mCopierDeleterCombined; }

  /// Access the deleter.
  [[nodiscard]] auto&& getD() && { return std::move(mCopierDeleterCombined); }

  /// Access the deleter.
  [[nodiscard]] auto const&& getD() const&& {
    return std::move(mCopierDeleterCombined);
  }
#endif

  /// Swaps the indirectly owned objects.
  constexpr void swap(indirect_base& rhs) noexcept(
      std::is_nothrow_swappable_v<CD>) {
    using std::swap;
    swap(mValue, rhs.mValue);
    swap(mCopierDeleterCombined, rhs.mCopierDeleterCombined);
  }
};

template <typename T, typename C, typename D>
struct indirect_base {
  exchange_on_move_ptr<T> mValue;  ///< The indirectly owned value.

#if defined(_MSC_VER)
  [[msvc::no_unique_address]] C
      mCopier;  ///< The copier functor to customise how the underlying value is
                ///< copied.
  [[msvc::no_unique_address]] D
      mDeleter;  ///< The deleter functor to customise how the underlying value
                 ///< is deleted.
#else
  [[no_unique_address]] C mCopier;  ///< The copier functor to customise how the
                                    ///< underlying value is copied.
  [[no_unique_address]] D mDeleter;  ///< The deleter functor to customise how
                                     ///< the underlying value is deleted.
#endif

#if (__cpp_explicit_this_parameter >= 202110L)
  /// Access the copier.
  template <typename Self>
  [[nodiscard]] std::copy_cvref_t<Self, auto> getC(this Self&& self) {
    return mCopier;
  }

  /// Access the deleter.
  template <typename Self>
  [[nodiscard]] std::copy_cvref_t<Self, auto> getD(this Self&& self) {
    return mDeleter;
  }
#else
  /// Access the copier.
  [[nodiscard]] auto& getC() & { return mCopier; }

  /// Access the copier.
  [[nodiscard]] auto const& getC() const& { return mCopier; }

  /// Access the copier.
  [[nodiscard]] auto&& getC() && { return std::move(mCopier); }

  /// Access the copier.
  [[nodiscard]] auto const&& getC() const&& { return std::move(mCopier); }

  /// Access the deleter.
  [[nodiscard]] auto& getD() & { return mDeleter; }

  /// Access the deleter.
  [[nodiscard]] auto const& getD() const& { return mDeleter; }

  /// Access the deleter.
  [[nodiscard]] auto&& getD() && { return std::move(mDeleter); }

  /// Access the deleter.
  [[nodiscard]] auto const&& getD() const&& { return std::move(mDeleter); }
#endif

  /// Swaps the indirectly owned objects.
  constexpr void swap(indirect_base& rhs) noexcept(
      std::is_nothrow_swappable_v<C>&& std::is_nothrow_swappable_v<D>) {
    using std::swap;
    swap(mValue, rhs.mValue);
    swap(mCopier, rhs.mCopier);
    swap(mDeleter, rhs.mDeleter);
  }
};

}  // namespace detail

/// \class indirect
///     Implements P1950R2, a free-store-allocated value type for C++
/// \tparam T The underlying value type.
/// \tparam Copier The copier functor to customise how the underlying value is
/// copied. \tparam Deleter The deleter functor to customise how the underlying
/// value is deleted.
// template <typename T, std::invocable<T const&> Copier = default_copy<T>,
// std::invocable<T*> Deleter = typename copier_traits<Copier>::deleter_type>
// requires std::same_as<std::invoke_result_t<Copier, T const&>, T*>
template <typename T, typename Copier = default_copy<T>,
          typename Deleter = typename copier_traits<Copier>::deleter_type>
class indirect : public detail::indirect_base<T, Copier, Deleter> {
  using base_type = detail::indirect_base<T, Copier, Deleter>;

 public:
  using value_type = T;  ///< Underlying value type
  using copier_type =
      Copier;  ///< Copier object customising copying of underlying value
  using deleter_type =
      Deleter;  ///< Deleter object customising deleting of underlying value

  /// Constructs an empty indirect.
  constexpr indirect() noexcept(
      std::is_nothrow_default_constructible_v<copier_type>&&
          std::is_nothrow_default_constructible_v<deleter_type>) = default;

  /// Inplace construction of the indirect value.
  /// \note Conditionally enabled only when the input parameters match the
  /// requires parameter of the underlying types
  ///       constructors.
  /// \param[in] ts Forwarded parameters to underlying types constructor.
  template <class... Ts>
    requires(std::is_constructible_v<T, Ts...>)
  constexpr explicit indirect(std::in_place_t, Ts&&... ts)
      : base_type{new T(std::forward<Ts>(ts)...)} {}

  /// Constructs a indirect which owns takes ownership of the input t. The
  /// copier and deleter are default constructed. \note If t is null, creates an
  /// empty object.
  constexpr explicit indirect(T* t) noexcept(
      std::is_nothrow_default_constructible_v<copier_type>&&
          std::is_nothrow_default_constructible_v<deleter_type>)
    requires(std::is_default_constructible_v<copier_type> &&
             not std::is_pointer_v<copier_type> &&
             std::is_default_constructible_v<deleter_type> &&
             not std::is_pointer_v<deleter_type>)
      : base_type{t} {}

  /// Constructs a indirect which owns takes ownership of the input t. The
  /// copier is moved from c and deleter is default constructed. \note If t is
  /// null, creates an empty object.
  constexpr explicit indirect(T* t, copier_type c) noexcept(
      std::is_nothrow_move_constructible_v<copier_type>&&
          std::is_nothrow_default_constructible_v<deleter_type>)
    requires(std::is_move_constructible_v<copier_type> &&
             std::is_default_constructible_v<deleter_type> &&
             not std::is_pointer_v<deleter_type>)
      : base_type{t, std::move(c)} {}

  /// Constructs a indirect which owns takes ownership of the input t. The
  /// copier is moved from c and deleter is moved from d. \note If t is null,
  /// creates an empty object.
  constexpr explicit indirect(
      T* t, copier_type c,
      deleter_type
          d) noexcept(std::is_nothrow_move_constructible_v<copier_type>&&
                          std::is_nothrow_move_constructible_v<deleter_type>)
    requires(std::is_move_constructible_v<copier_type> &&
             std::is_move_constructible_v<deleter_type>)
      : base_type{t, std::move(c), std::move(d)} {}

  constexpr ~indirect() { reset(); }

  /// Copy constructor.
  /// \pre IsComplete<T> is false or std::is_copy_constructible_v<T> is true
  constexpr indirect(indirect const& i)
      // requires(!meta::concepts::IsComplete<T> or
      // std::is_copy_constructible_v<T>)
      : base_type(i) {
    this->mValue = i.make_raw_copy();
  }

  /// Move constructor.
  constexpr indirect(indirect&& i) noexcept(
      std::is_nothrow_move_constructible_v<copier_type>&&
          std::is_nothrow_move_constructible_v<deleter_type>)
      : base_type(std::move(i)) {}

  /// Copy assignment, assigns contents via the underlying copier
  /// \pre IsComplete<T> is false or std::is_copy_constructible_v<T> is true
  constexpr indirect& operator=(indirect const& i)
  //    requires(!meta::concepts::IsComplete<T> or
  //    std::is_copy_constructible_v<T>)
  {
    indirect temp(i);
    swap(temp);
    return *this;
  }

  /// Move assignement, assigns contents via moving from the right hand side to
  /// the left hand side.
  constexpr indirect& operator=(indirect&& i) noexcept(
      std::is_nothrow_move_assignable_v<copier_type>&&
          std::is_nothrow_move_assignable_v<deleter_type>) {
    if (this != &i) {
      reset();
      base_type::operator=(std::move(i));
    }
    return *this;
  }

#if (__cpp_explicit_this_parameter >= 202110L)

  /// Accesses the contained value.
  template <typename Self>
  [[nodiscard]] constexpr auto* operator->(this Self&& self) noexcept {
    return (this->mValue);
  }

  /// Dereferences pointer to the managed object.
  template <typename Self>
  [[nodiscard]] constexpr std::copy_cvref_t<Self, T>&& operator*(
      this Self&& self) noexcept {
    return *std::forward(self).mValue;
  }

  /// If *this contains a value, returns a reference to the contained value.
  /// Otherwise, throws a bad_indirect_access exception.
  template <typename Self>
  [[nodiscard]] constexpr auto& value(this Self&& self) {
    if (!this->mValue) throw bad_indirect_access();
    return *(this->mValue);
  }
#else
  /// Accesses the contained value.
  [[nodiscard]] constexpr T* operator->() noexcept { return this->mValue; }

  /// Accesses the contained value.
  [[nodiscard]] constexpr const T* operator->() const noexcept {
    return this->mValue;
  }

  /// Dereferences pointer to the managed object.
  [[nodiscard]] constexpr T& operator*() & noexcept { return *(this->mValue); }

  /// Dereferences pointer to the managed object.
  [[nodiscard]] constexpr const T& operator*() const& noexcept {
    return *(this->mValue);
  }

  /// Dereferences pointer to the managed object.
  [[nodiscard]] constexpr T&& operator*() && noexcept {
    return std::move(*(this->mValue));
  }

  /// Dereferences pointer to the managed object.
  [[nodiscard]] constexpr const T&& operator*() const&& noexcept {
    return std::move(*(this->mValue));
  }

  /// If *this contains a value, returns a reference to the contained value.
  /// Otherwise, throws a bad_indirect_access exception.
  [[nodiscard]] constexpr T& value() & {
    if (!this->mValue) throw bad_indirect_access();
    return *(this->mValue);
  }

  /// If *this contains a value, returns a reference to the contained value.
  /// Otherwise, throws a bad_indirect_access exception.
  [[nodiscard]] constexpr T&& value() && {
    if (!this->mValue) throw bad_indirect_access();
    return std::move(*(this->mValue));
  }

  /// If *this contains a value, returns a reference to the contained value.
  /// Otherwise, throws a bad_indirect_access exception.
  [[nodiscard]] constexpr const T& value() const& {
    if (!this->mValue) throw bad_indirect_access();
    return *(this->mValue);
  }

  /// If *this contains a value, returns a reference to the contained value.
  /// Otherwise, throws a bad_indirect_access exception.
  [[nodiscard]] constexpr const T&& value() const&& {
    if (!this->mValue) throw bad_indirect_access();
    return std::move(*(this->mValue));
  }
#endif  // (__cpp_explicit_this_parameter >= 202110)

  /// Checks whether *this contains a value.
  explicit constexpr operator bool() const noexcept {
    return this->mValue != nullptr;
  }

  /// Checks whether *this contains a value.
  constexpr bool has_value() const noexcept { return this->mValue != nullptr; }

  /// Access the copier.
  constexpr copier_type& get_copier() noexcept { return this->getC(); }

  /// Access the copier.
  constexpr const copier_type& get_copier() const noexcept {
    return this->getC();
  }

  /// Access the deleter.
  constexpr deleter_type& get_deleter() noexcept { return this->getD(); }

  /// Access the deleter.
  constexpr const deleter_type& get_deleter() const noexcept {
    return this->getD();
  }

  /// Swaps the indirectly owned objects.
  constexpr void swap(indirect& rhs) noexcept(
      std::is_nothrow_swappable_v<base_type>) {
    this->base_type::swap(static_cast<base_type&>(rhs));
  }

  /// Specialises the std::swap algorithm.
  friend void swap(indirect& lhs,
                   indirect& rhs) noexcept(noexcept(lhs.swap(rhs)))
    requires std::is_swappable_v<copier_type> &&
             std::is_swappable_v<deleter_type>
  {
    lhs.swap(rhs);
  }

 private:
  constexpr T* make_raw_copy() const {
    return this->mValue ? this->getC()(*(this->mValue)) : nullptr;
  }

  constexpr void reset() noexcept {
    if (has_value()) {
      this->getD()(exchange(this->mValue, nullptr));
    }
  }
};

template <class T>
indirect(T*) -> indirect<T>;

template <class T, class... Ts>
constexpr indirect<T> make_indirect(Ts&&... ts) {
  return indirect<T>(std::in_place_t{}, std::forward<Ts>(ts)...);
}

template <class T, class A = std::allocator<T>, class... Ts>
constexpr auto allocate_indirect(std::allocator_arg_t, A& a, Ts&&... ts) {
  auto* u = detail::allocate_object<T>(a, std::forward<Ts>(ts)...);
  try {
    return indirect<T, detail::allocator_copy<T, A>,
                          detail::allocator_delete<T, A>>(u, {a}, {a});
  } catch (...) {
    detail::deallocate_object(a, u);
    throw;
  }
}

}  // namespace xyz

template <class T, class C, class D>
struct std::hash<::xyz::indirect<T, C, D>>
{
  constexpr std::size_t operator()(const ::xyz::indirect<T, C, D>& key) const noexcept(
      noexcept(std::hash<typename ::xyz::indirect<T, C, D>::value_type>{}(*key))) {
    return key ? std::hash<typename ::xyz::indirect<T, C, D>::value_type>{}(*key) : 0;
  }
};

#endif  // XYZ_INDIRECT_H_