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


// cloning_ptr can only be created initially by make_copying. After that it can be (deeply) copied and moved just like
// unique_ptr can't. There is no array version as this does not make sense with polymorphism. Would be cool though, if indexing was
// stepped correctly. OTOH most often you'd want to have different element types.
template<typename T> class cloning_ptr {
public:
    cloning_ptr() = default;

    template <class U, class... Ts>
    explicit cloning_ptr(std::in_place_type_t<U>, Ts&&... ts)
        requires std::constructible_from<U, Ts&&...> &&
                 std::copy_constructible<U> &&
                 (std::derived_from<U, T> || std::same_as<U, T>) :
        m_ptr(std::make_unique<U>(std::forward<Ts>(ts)...)),
        m_cloner([](T& src)->std::unique_ptr<T> {
            // Create a unique_ptr<U>, which is the actual type of src. Then let the return statement cast this to a
            // unique_ptr<T>.
            return make_unique<U>(static_cast<const U&>(src));      // Problem: static_cast does not work from virtual bases. 
        })
    {
    }

    cloning_ptr(const cloning_ptr& src) : m_ptr(src.m_cloner(*src)), m_cloner(src.m_cloner) {}
    cloning_ptr(cloning_ptr&& src) noexcept : m_ptr(std::move(src.m_ptr)), m_cloner(src.m_cloner) {}
              
    cloning_ptr& operator=(const cloning_ptr& src) {
        m_ptr = src.m_cloner(*src);
        m_cloner = src.m_cloner;
    }
    cloning_ptr& operator=(cloning_ptr&& src) noexcept {
        m_ptr = std::move(src.m_ptr);
        m_cloner = src.m_cloner;
    }

    // Modifiers
    template <class U, class... Ts>
    void emplace(Ts&&... ts) { *this = make<U>(std::forward<Ts>(ts)...); }
    
    T* relese() { return m_ptr.release(); }
    void reset() { m_ptr.reset(); }
    void swap(cloning_ptr& other) noexcept { using std::swap; swap(m_ptr, other.m_ptr); swap(m_cloner, other.m_cloner); }

    // Observers
    T* get() const { return m_ptr.get(); }      // Shallow const like unique_ptr. For better or for worse.
    operator bool() { return m_ptr; }

    T* operator->() noexcept { return m_ptr.get(); }
    const T* operator->() const noexcept { return m_ptr.get(); }

    T& operator*() noexcept { return *m_ptr; }
    const T& operator*() const noexcept { return *m_ptr; }

    // Creation helper
    template<class U, class... Ts>
    static cloning_ptr make(Ts&&... ts) { return cloning_ptr(std::in_place_type<U>, std::forward<Ts>(ts)...); }

private:
    cloning_ptr(std::unique_ptr<T> src) : m_ptr(std::move(src)), m_cloner([](T& src)->std::unique_ptr<T> {
        return make_unique<T>(static_cast<const T&>(src));      // Problem: static_cast does not work from virtual bases. 
    })
    {}

    std::unique_ptr<T> m_ptr;
    std::unique_ptr<T> (*m_cloner)(T& src) = nullptr;
};


template <typename T> void swap(cloning_ptr<T>& lhs, cloning_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}


}  // namespace xyz

#endif  // XYZ_CLONING_PTR_H_