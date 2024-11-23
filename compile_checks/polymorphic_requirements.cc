/* Copyright (c) 2016 The Value Types Authors. All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
==============================================================================*/

// A set of static assertions to check that various requirements on types are
// respected.

#include "polymorphic.h"

namespace xyz {

struct Nope {};

class AllCtors {
 public:
  AllCtors() = default;
  AllCtors(const AllCtors&) = default;
  AllCtors(const Nope&) = delete;
  template <typename... T>
  AllCtors(const T&...) {}
};

class NoCopy {
 public:
  NoCopy() = default;
  NoCopy(const NoCopy&) = delete;
  NoCopy(const Nope&) = delete;
  template <typename... T>
  NoCopy(const T&...) {}
};

class NoDefaultCtor {
 public:
  NoDefaultCtor() = delete;
  NoDefaultCtor(const NoDefaultCtor&) = default;
  NoDefaultCtor(const Nope&) = delete;
  template <typename... T>
  NoDefaultCtor(const T&...) {}
};

template <typename T>
class NoDefaultCtorAlloc : public std::allocator<T> {
 public:
  using typename std::allocator<T>::value_type;
  NoDefaultCtorAlloc() = delete;
  NoDefaultCtorAlloc(const NoDefaultCtorAlloc&) = default;
  NoDefaultCtorAlloc(const Nope&) = delete;
};

template <typename T, typename Alloc = std::allocator<T>, typename... Us>
consteval bool checks() {
  static_assert(std::is_same_v<typename Alloc::value_type, T>);
  using polymorphic_t = xyz::polymorphic<T, Alloc>;

  // If the allocator is not default constructible, then
  // non allocator-extended constructors are unavailable.
  if constexpr (!std::is_default_constructible_v<Alloc>) {
    static_assert(!std::is_constructible_v<polymorphic_t>);
    static_assert(!std::is_constructible_v<polymorphic_t, T>);
    static_assert(!std::is_constructible_v<polymorphic_t, Us...>);
  } else /* constexpr */ {
    // T is copy constructible and default constructible.
    if constexpr (std::is_default_constructible_v<T> &&
                  std::is_copy_constructible_v<T>) {
      static_assert(std::is_constructible_v<polymorphic_t>);
      static_assert(std::is_constructible_v<polymorphic_t, T>);
    }
    // T is not copy constructible. All constructors are unavailable.
    if constexpr (!std::is_copy_constructible_v<T>) {
      static_assert(!std::is_constructible_v<polymorphic_t>);
      static_assert(!std::is_constructible_v<polymorphic_t, T>);
      static_assert(!std::is_constructible_v<polymorphic_t, Us...>);
      static_assert(!std::is_constructible_v<polymorphic_t,
                                             std::in_place_type_t<T>, Us...>);
    } else /* constexpr */ {
      // T is default constructible.
      if constexpr (std::is_default_constructible_v<T>) {
        static_assert(std::is_constructible_v<polymorphic_t>);
      } else /* constexpr */ {
        static_assert(!std::is_constructible_v<polymorphic_t>);
      }
    }
  }
  // Alloc extended constructors.

  // T is copy constructible and default constructible.
  if constexpr (std::is_default_constructible_v<T> &&
                std::is_copy_constructible_v<T>) {
    static_assert(
        std::is_constructible_v<polymorphic_t, std::allocator_arg_t, Alloc>);
    static_assert(
        std::is_constructible_v<polymorphic_t, std::allocator_arg_t, Alloc, T>);
  }
  // T is not copy constructible. All constructors are unavailable.
  if constexpr (!std::is_copy_constructible_v<T>) {
    static_assert(
        !std::is_constructible_v<polymorphic_t, std::allocator_arg_t, Alloc>);
    static_assert(!std::is_constructible_v<polymorphic_t, std::allocator_arg_t,
                                           Alloc, T>);
    static_assert(!std::is_constructible_v<polymorphic_t, std::allocator_arg_t,
                                           Alloc, Us...>);
    static_assert(
        !std::is_constructible_v<polymorphic_t, std::allocator_arg_t, Alloc,
                                 std::in_place_type_t<T>, Us...>);
  } else /* constexpr */ {
    // T is default constructible.
    if constexpr (std::is_default_constructible_v<T>) {
      static_assert(
          std::is_constructible_v<polymorphic_t, std::allocator_arg_t, Alloc>);
    } else /* constexpr */ {
      static_assert(
          !std::is_constructible_v<polymorphic_t, std::allocator_arg_t, Alloc>);
    }
  }
  return true;
}

// Check requirements for types used in checks.

static_assert(!std::is_default_constructible_v<NoDefaultCtor>);
static_assert(!std::is_copy_constructible_v<NoCopy>);
static_assert(std::is_constructible_v<NoDefaultCtor, int>);
static_assert(std::is_constructible_v<NoCopy, int>);
static_assert(!std::is_constructible_v<AllCtors, Nope>);
static_assert(!std::is_constructible_v<NoDefaultCtor, Nope>);
static_assert(!std::is_constructible_v<NoCopy, Nope>);
static_assert(!std::is_default_constructible_v<NoDefaultCtorAlloc<int>>);

// Default constructible allocator.

static_assert(checks<NoCopy, std::allocator<NoCopy>, int>());
static_assert(checks<NoDefaultCtor, std::allocator<NoDefaultCtor>, int>());
static_assert(checks<AllCtors, std::allocator<AllCtors>, int>());

static_assert(checks<NoCopy, std::allocator<NoCopy>, Nope>());
static_assert(checks<NoDefaultCtor, std::allocator<NoDefaultCtor>, Nope>());
static_assert(checks<AllCtors, std::allocator<AllCtors>, Nope>());

static_assert(checks<NoCopy, std::allocator<NoCopy>, int, int>());
static_assert(checks<NoDefaultCtor, std::allocator<NoDefaultCtor>, int, int>());
static_assert(checks<AllCtors, std::allocator<AllCtors>, int, int>());

// Non-default constructible allocator.

static_assert(checks<NoCopy, NoDefaultCtorAlloc<NoCopy>, int>());
static_assert(checks<NoDefaultCtor, NoDefaultCtorAlloc<NoDefaultCtor>, int>());
static_assert(checks<AllCtors, NoDefaultCtorAlloc<AllCtors>, int>());

static_assert(checks<NoCopy, NoDefaultCtorAlloc<NoCopy>, Nope>());
static_assert(checks<NoDefaultCtor, NoDefaultCtorAlloc<NoDefaultCtor>, Nope>());
static_assert(checks<AllCtors, NoDefaultCtorAlloc<AllCtors>, Nope>());

static_assert(checks<NoCopy, NoDefaultCtorAlloc<NoCopy>, int, int>());
static_assert(
    checks<NoDefaultCtor, NoDefaultCtorAlloc<NoDefaultCtor>, int, int>());
static_assert(checks<AllCtors, NoDefaultCtorAlloc<AllCtors>, int, int>());

}  // namespace xyz
