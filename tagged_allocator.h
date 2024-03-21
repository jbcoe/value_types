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

// Based on boilerplate from:
// https://howardhinnant.github.io/allocator_boilerplate.html

#ifndef XYZ_TAGGED_ALLOCATOR_H
#define XYZ_TAGGED_ALLOCATOR_H

namespace xyz {

template <typename T>
struct TaggedAllocator {
  using value_type = T;

  size_t tag;

  TaggedAllocator(size_t tag) : tag(tag) {}

  template <typename U>
  TaggedAllocator(const TaggedAllocator<U>& other) : tag(other.tag) {}

  template <typename Other>
  struct rebind {
    using other = TaggedAllocator<Other>;
  };

  // clang 17 and 18 seem to need the `construct` function to be explicitly
  // defined rather than having a default picked up from allocator traits. It
  // does nothing special.
  template <class U, class... Args>
  void construct(U* p, Args&&... args) {
    ::new (p) U(std::forward<Args>(args)...);
  }

  T* allocate(std::size_t n) {
    std::allocator<T> default_allocator{};
    return default_allocator.allocate(n);
  }

  void deallocate(T* p, std::size_t n) {
    std::allocator<T> default_allocator{};
    default_allocator.deallocate(p, n);
  }

  friend bool operator==(const TaggedAllocator& lhs,
                         const TaggedAllocator& rhs) noexcept {
    return lhs.tag == rhs.tag;
  }

  friend bool operator!=(const TaggedAllocator& lhs,
                         const TaggedAllocator& rhs) noexcept {
    return !(lhs == rhs);
  }
};

}  // namespace xyz

#endif  // XYZ_TAGGED_ALLOCATOR_H_
