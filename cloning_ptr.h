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

namespace detail {

    struct impl_base {
        virtual ~impl_base() = default;
        virtual impl_base* clone() const = 0;
    };

    template <typename U>
    struct impl : public impl_base {
        template <typename... Ts>
                impl(Ts&&... ts) : m_value(std::forward<Ts>(ts)...) {}

        impl_base* clone() const override { return new impl(m_value); }
        U m_value;
    };

}

// VERSION WITH impl, but still fat.

// cloning_ptr can only be created initially by make_copying. After that it can be (deeply) copied and moved just like
// unique_ptr can't. There is no array version as this does not make sense with polymorphism. Would be cool though, if indexing was
// stepped correctly. OTOH most often you'd want to have different element types.
template<typename T> class cloning_ptr {
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

    cloning_ptr(const cloning_ptr& src) { *this = src; }
    template<typename X> cloning_ptr(const cloning_ptr<X>& src) { *this = src; }

    cloning_ptr(cloning_ptr&& src) noexcept { *this = std::move(src); }
    template<typename X> cloning_ptr(cloning_ptr<X>&& src) noexcept { *this = std::move(src); }
    ~cloning_ptr() { reset(); }
             
    cloning_ptr& operator=(const cloning_ptr& src) {
        reset();
        detail::impl_base* c = src.get_impl()->clone();
        m_offset = src.m_offset;
        m_ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(c) + m_offset);
        return *this;
    }
    template<typename X> cloning_ptr& operator=(const cloning_ptr<X>& src) {
        reset();
        detail::impl_base* c = src.get_impl()->clone();
        X* x = reinterpret_cast<X*>(reinterpret_cast<char*>(c) + src.m_offset);
        m_ptr = x;   // Offset add from X to T happens.
        m_offset = reinterpret_cast<char*>(m_ptr) - reinterpret_cast<char*>(c);
        return *this;
    }
    template<typename X> cloning_ptr& operator=(cloning_ptr<X>&& src) noexcept {
        reset();
        X* x = src.get();
        m_ptr = x;   // Offset add from X to T happens.
        m_offset = reinterpret_cast<char*>(m_ptr) - reinterpret_cast<char*>(src.get_impl());
        src.m_ptr = nullptr;
        src.m_offset = 0;
        return *this;
    }
    cloning_ptr& operator=(const nullptr_t& src) noexcept { reset(); }
    
    // Modifiers
    template <class U, class... Ts>
    void emplace(Ts&&... ts) {
        detail::impl<U>* p = new detail::impl<U>(std::forward<Ts>(ts)...);
        U* value = &p->m_value;
        m_ptr = value;      // Offset adjustment happens.
        m_offset = reinterpret_cast<char*>(m_ptr) - reinterpret_cast<char*>(p);
    }
    
    void reset() { delete get_impl(); m_ptr = nullptr; }
    void swap(cloning_ptr& other) noexcept { using std::swap; swap(m_offset, other.m_offset); swap(m_ptr, other.m_ptr); }

    // Observers
    const T* get() const { return m_ptr; }
    T* get() { return m_ptr; }
    operator bool() const { return m_ptr != nullptr; }

    T* operator->() noexcept { return m_ptr; }
    const T* operator->() const noexcept { return m_ptr; }

    T& operator*() noexcept { return *m_ptr; }
    const T& operator*() const noexcept { return *m_ptr; }

    // Creation helper
    template<class U, class... Ts>
    static cloning_ptr make(Ts&&... ts) { return cloning_ptr(std::in_place_type<U>, std::forward<Ts>(ts)...); }

    // Comparison is shallow, and as meaningless as for unique_ptr. Could be made deep, but that brings us further from pointer
    // semantics, and why didn't unique_ptr do this?
    auto operator<=>(const cloning_ptr& rhs) const { return m_ptr <=> rhs.m_ptr; }

//private:      -- Some friending would be required...
    detail::impl_base* get_impl() const {
        return reinterpret_cast<detail::impl_base*>(reinterpret_cast<char*>(m_ptr) - m_offset);
    }

    size_t m_offset = 0;            // Offset from the T m_ptr points to to the start of the impl_base containing it.
    T* m_ptr = nullptr;
};


template <typename T> void swap(cloning_ptr<T>& lhs, cloning_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}

// Add *_pointer_cast overloads here. These will come in both lvalue and rvalue versions, where the former clone if the cast was successful.

}  // namespace xyz

#endif  // XYZ_CLONING_PTR_H_