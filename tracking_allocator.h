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

#ifndef XYZ_TRACKING_ALLOCATOR_H_
#define XYZ_TRACKING_ALLOCATOR_H_

namespace xyz {

template <typename T>
struct TrackingAllocator {
  unsigned* alloc_counter_;
  unsigned* dealloc_counter_;

  TrackingAllocator(unsigned* alloc_counter, unsigned* dealloc_counter)
      : alloc_counter_(alloc_counter), dealloc_counter_(dealloc_counter) {}

  template <typename U>
  TrackingAllocator(const TrackingAllocator<U>& other)
      : alloc_counter_(other.alloc_counter_),
        dealloc_counter_(other.dealloc_counter_) {}

  using value_type = T;

  template <typename Other>
  struct rebind {
    using other = TrackingAllocator<Other>;
  };

  T* allocate(std::size_t n) {
    ++(*alloc_counter_);
    std::allocator<T> default_allocator{};
    return default_allocator.allocate(n);
  }

  void deallocate(T* p, std::size_t n) {
    ++(*dealloc_counter_);
    std::allocator<T> default_allocator{};
    default_allocator.deallocate(p, n);
  }

  friend bool operator==(const TrackingAllocator& lhs,
                         const TrackingAllocator& rhs) noexcept {
    return lhs.alloc_counter_ == rhs.alloc_counter_ &&
           lhs.dealloc_counter_ == rhs.dealloc_counter_;
  }

  friend bool operator!=(const TrackingAllocator& lhs,
                         const TrackingAllocator& rhs) noexcept {
    return !(lhs == rhs);
  }
};

}  // namespace xyz

#endif  // XYZ_TRACKING_ALLOCATOR_H_
