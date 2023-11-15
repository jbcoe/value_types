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

#ifndef XYZ_EMPTY_BASE_OPTIMIZATION_H
#define XYZ_EMPTY_BASE_OPTIMIZATION_H

#include <type_traits>
#include <utility>
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

#endif  // XYZ_EMPTY_BASE_OPTIMIZATION_H
