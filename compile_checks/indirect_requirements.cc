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

#include "indirect.h"

namespace xyz {

struct Nope {};

class DeletedCopy {
 public:
  DeletedCopy() = default;
  DeletedCopy(const DeletedCopy&) = delete;
  DeletedCopy(const Nope&) = delete;
  template <typename... T>
  DeletedCopy(const T&...) {}
};

class DeletedDefaultConstructor {
 public:
  DeletedDefaultConstructor() = delete;
  DeletedDefaultConstructor(const DeletedDefaultConstructor&) = default;
  DeletedDefaultConstructor(const Nope&) = delete;
  template <typename... T>
  DeletedDefaultConstructor(const T&...) {}
};

template <typename T, typename Allocator = std::allocator<T>, typename... Us>
consteval bool checks() {
  static_assert(std::is_same_v<typename Allocator::value_type, T>);
  using indirect_t = xyz::indirect<T, Allocator>;
  // T must be copy constructible.
  if constexpr (!std::is_copy_constructible_v<T>) {
    static_assert(!std::is_constructible_v<indirect_t>);
    static_assert(!std::is_constructible_v<indirect_t, T>);
    static_assert(!std::is_constructible_v<indirect_t, Us...>);
  } else {
    // If the allocator is not default constructible, then
    // non allocator-extended constructors are unavailable.
    if constexpr (!std::is_default_constructible_v<Allocator>) {
      static_assert(!std::is_constructible_v<indirect_t>);
      static_assert(!std::is_constructible_v<indirect_t, T>);
      static_assert(!std::is_constructible_v<indirect_t, Us...>);
    } else /* constexpr */ {
      // T default constructible.
      if constexpr (std::is_default_constructible_v<T>) {
        static_assert(std::is_constructible_v<indirect_t>);
      } else /* constexpr */ {
        static_assert(!std::is_constructible_v<indirect_t>);
      }
      // T constructible from Us
      if constexpr (std::is_constructible_v<T, Us...>) {
        if constexpr (sizeof...(Us) == 1) {
          static_assert(std::is_constructible_v<indirect_t, Us...>);
        } else /* constexpr */ {
        }
        static_assert(
            std::is_constructible_v<indirect_t, std::in_place_t, Us...>);
      } else /* constexpr */ {
        static_assert(!std::is_constructible_v<indirect_t, Us...>);
        static_assert(
            !std::is_constructible_v<indirect_t, std::in_place_t, Us...>);
      }
    }

    // Allocator extended constructors.
    // T default constructible.
    if constexpr (std::is_default_constructible_v<T>) {
      static_assert(
          std::is_constructible_v<indirect_t, std::allocator_arg_t, Allocator>);
    } else /* constexpr */ {
      static_assert(!std::is_constructible_v<indirect_t, std::allocator_arg_t,
                                             Allocator>);
    }
    // T constructible from Us.
    if constexpr (std::is_constructible_v<T, Us...>) {
      if constexpr (sizeof...(Us) == 1) {
        static_assert(std::is_constructible_v<indirect_t, std::allocator_arg_t,
                                              Allocator, Us...>);
      } else /* constexpr */ {
      }
      static_assert(
          std::is_constructible_v<indirect_t, std::in_place_t,
                                  std::allocator_arg_t, Allocator, Us...>);
    } else /* constexpr */ {
      static_assert(!std::is_constructible_v<indirect_t, std::allocator_arg_t,
                                             Allocator, Us...>);
      static_assert(
          !std::is_constructible_v<indirect_t, std::allocator_arg_t, Allocator,
                                   std::in_place_t, Us...>);
    }
  }
  return true;
}

static_assert(!std::is_default_constructible_v<DeletedDefaultConstructor>);
static_assert(!std::is_copy_constructible_v<DeletedCopy>);
static_assert(std::is_constructible_v<DeletedDefaultConstructor, int>);
static_assert(std::is_constructible_v<DeletedCopy, int>);
static_assert(!std::is_constructible_v<DeletedDefaultConstructor, Nope>);
static_assert(!std::is_constructible_v<DeletedCopy, Nope>);

static_assert(checks<DeletedCopy, std::allocator<DeletedCopy>, int>());
static_assert(checks<DeletedDefaultConstructor,
                     std::allocator<DeletedDefaultConstructor>, int>());
static_assert(checks<DeletedCopy, std::allocator<DeletedCopy>, Nope>());
static_assert(checks<DeletedDefaultConstructor,
                     std::allocator<DeletedDefaultConstructor>, Nope>());
static_assert(checks<DeletedCopy, std::allocator<DeletedCopy>, int, int>());
static_assert(checks<DeletedDefaultConstructor,
                     std::allocator<DeletedDefaultConstructor>, int, int>());

}  // namespace xyz
