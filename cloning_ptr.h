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

#ifndef XYZ_CLONING_PTR_H_
#define XYZ_CLONING_PTR_H_

#include <cassert>
#include <concepts>
#include <memory>
#include <utility>

namespace xyz {


// VERSION WITH impl, but still fat.

// cloning_ptr can only be created initially by make_copying. After that it can be (deeply) copied and moved just like
// unique_ptr can't. There is no array version as this does not make sense with polymorphism. Would be cool though, if indexing was
// stepped correctly. OTOH most often you'd want to have different element types.
template<typename T> class cloning_ptr {
  struct impl_base {
    virtual void clone(cloning_ptr& src) = 0;
    virtual ~impl_base() = default;
  };

  template <typename U>
  struct impl : public impl_base {
    template <typename... Ts>
    impl(Ts&&... ts) : m_value(std::forward<Ts>(ts)...) {}

    void clone(cloning_ptr& dest) override {
      auto p = std::make_unique<impl<U>>(m_value);
      dest.m_ptr = &p->m_value;
      dest.m_impl = std::move(p);
    }

    U m_value;
  };

 public:
    cloning_ptr() = default;
    cloning_ptr(std::nullptr_t) {}

    template <class U, class... Ts>
    explicit cloning_ptr(std::in_place_type_t<U>, Ts&&... ts)
        requires std::constructible_from<U, Ts&&...> &&
                 std::copy_constructible<U> &&
                 (std::derived_from<U, T> || std::same_as<U, T>)
    {
        emplace<U>(std::forward<Ts>(ts)...);
    }

    cloning_ptr(const cloning_ptr& src) { src.m_impl->clone(*this); }
    cloning_ptr(cloning_ptr&& src) noexcept : m_impl(std::move(src.m_impl)), m_ptr(src.m_ptr) {}
              
    cloning_ptr& operator=(const cloning_ptr& src) {
        src.m_impl->clone(*this);       // My old value is deleted thanks to unique_ptr.
        return *this;
    }
    cloning_ptr& operator=(cloning_ptr&& src) noexcept {
        m_impl = std::move(src.m_impl);
        m_ptr = src.m_ptr;
        return *this;
    }
    cloning_ptr& operator=(const nullptr_t& src) noexcept { reset(); }
    
    // Modifiers
    template <class U, class... Ts>
    void emplace(Ts&&... ts) {
        auto p = std::make_unique<impl<U>>(std::forward<Ts>(ts)...);
        m_ptr = &p->m_value;
        m_impl = std::move(p);
    }
    
    void reset() { m_impl.reset(); m_ptr = nullptr; }
    void swap(cloning_ptr& other) noexcept { using std::swap; swap(m_impl, other.m_impl); swap(m_ptr, other.m_ptr); }

    // Observers
    const T* get() const { return m_ptr; }
    T* get() { return m_ptr; }
    operator bool() const { return m_impl != nullptr; }

    T* operator->() noexcept { return m_ptr; }
    const T* operator->() const noexcept { return m_ptr; }

    T& operator*() noexcept { return *m_ptr; }
    const T& operator*() const noexcept { return *m_ptr; }

    // Creation helper
    template<class U, class... Ts>
    static cloning_ptr make(Ts&&... ts) { return cloning_ptr(std::in_place_type<U>, std::forward<Ts>(ts)...); }

    // Comparison
    auto operator<=>(const cloning_ptr& rhs) const { return m_ptr <=> rhs.m_ptr; }

private:
    std::unique_ptr<impl_base> m_impl;
    T* m_ptr = nullptr;
};


template <typename T> void swap(cloning_ptr<T>& lhs, cloning_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}


}  // namespace xyz

#endif  // XYZ_CLONING_PTR_H_