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

template <typename T> class cloning_ptr;

template <typename T, typename X> cloning_ptr<T> dynamic_pointer_cast(const cloning_ptr<X>& src);
template <typename T, typename X> cloning_ptr<T> dynamic_pointer_cast(cloning_ptr<X>&& src);
template <typename T, typename X> cloning_ptr<T> static_pointer_cast(const cloning_ptr<X>& src);
template <typename T, typename X> cloning_ptr<T> static_pointer_cast(cloning_ptr<X>&& src);

namespace detail {

    // Impl class allowing cloning and destruction using virtual methods.
    struct impl_base {
        virtual ~impl_base() = default;
        virtual std::unique_ptr<impl_base> clone() const = 0;
    };

    // Concrete impl subclass containing a value of type U. It clones and destroys itself.
    template <typename U>
    struct impl : public impl_base {
        template <typename... Ts>
        impl(Ts&&... ts) : m_value(std::forward<Ts>(ts)...) {}

        std::unique_ptr<impl_base> clone() const override { return std::make_unique<impl>(m_value); }
        U m_value;
    };

    // Base class for the actual smart pointers. By letting it include the unique_ptr template instantiation work is
    // somewhat reduced, and comparison between pointers to different subclasses facilitated.
    class cloning_ptr_base {
    public:
        operator bool() { return m_impl != nullptr; }

        // Comparison is shallow, and as meaningless as for unique_ptr. As only the same pointer compares equal. Useful for storing in
        // set/map only.
        template <class X> auto operator<=>(const cloning_ptr_base& rhs) const {
            return m_impl <=> rhs.m_impl;
        }

    private:
        template <typename T> friend class cloning_ptr;
        template <typename T, typename X> friend cloning_ptr<T> xyz::dynamic_pointer_cast(const cloning_ptr<X>& src);
        template <typename T, typename X> friend cloning_ptr<T> xyz::dynamic_pointer_cast(cloning_ptr<X>&& src);
        template <typename T, typename X> friend cloning_ptr<T> xyz::static_pointer_cast(const cloning_ptr<X>& src);
        template <typename T, typename X> friend cloning_ptr<T> xyz::static_pointer_cast(cloning_ptr<X>&& src);

        cloning_ptr_base() = default;
        cloning_ptr_base(std::unique_ptr<impl_base> impl) : m_impl(std::move(impl)) {}
 
        std::unique_ptr<impl_base> m_impl;
    };
    
}

// cloning_ptr can only be created initially by make_copying. After that it can be (deeply) copied and moved just like
// unique_ptr can't. There is no array version as this does not make sense with polymorphism. Would be cool though, if indexing was
// stepped correctly. OTOH most often you'd want to have different element types.
template<typename T> class cloning_ptr : public detail::cloning_ptr_base {
public:
    cloning_ptr() = default;
    cloning_ptr(std::nullptr_t) {}

    // constructor could be removed now that we have make_cloning. Well, it would be private and
    // make_cloning would be a friend. That would not need un_place_type as make_cloning could set up the members.
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
        if (&src == this)
            return *this;

        const T* op = src.get();
        size_t offset = reinterpret_cast<const char*>(op) - reinterpret_cast<const char*>(src.m_impl.get());

        m_impl = src.m_impl->clone();
        m_ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(m_impl.get()) + offset);
        return *this;
    }
    template<typename X> cloning_ptr& operator=(const cloning_ptr<X>& src) {
        if (&src == static_cast<const detail::cloning_ptr_base*>(this))
            return *this;

        const T* op = src.get();
        size_t offset = reinterpret_cast<const char*>(op) - reinterpret_cast<const char*>(src.m_impl.get());

        m_impl = src.m_impl->clone();
        m_ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(m_impl.get()) + offset);
        return *this;
    }
    template<typename X> cloning_ptr& operator=(cloning_ptr<X>&& src) noexcept {
        if (&src == static_cast<const detail::cloning_ptr_base*>(this))
            return *this;

        m_ptr = src.get();
        m_impl = std::move(src.m_impl);
        src.__clear_ptr();
        return *this;
    }
    cloning_ptr& operator=(const nullptr_t& src) noexcept { reset(); }
    
    // Modifiers

    // emplace could be removed now that we have make_cloning.
    template <class U, class... Ts>
    U& emplace(Ts&&... ts) {
        auto impl = std::make_unique<detail::impl<U>>(std::forward<Ts>(ts)...);
        U& ret = impl->m_value;
        m_ptr = &ret;      // Offset adjustment U -> T happens.
        m_impl = std::move(impl);
        return ret;
    }
    
    void swap(cloning_ptr& other) noexcept { using std::swap; swap(m_impl, other.m_impl); swap(m_ptr, other.m_ptr); }
    void reset() { m_ptr = nullptr; m_impl.reset(); }

    // Observers
    const T* get() const { return m_ptr; }
    T* get() { return m_ptr; }

    T* operator->() noexcept { return m_ptr; }
    const T* operator->() const noexcept { return m_ptr; }

    T& operator*() noexcept { return *m_ptr; }
    const T& operator*() const noexcept { return *m_ptr; }

    void __clear_ptr() { m_ptr = nullptr; }         // Internal but public due to problems with friend declarations below.

private:
    // I can't get these friend declarations to work... maybe a MSVC bug.
    //template <typename U>
    //friend cloning_ptr<U>& cloning_ptr<U>::operator=(const cloning_ptr<T>&);
    //template <typename U>
    //friend cloning_ptr<U>& cloning_ptr<U>::operator=(cloning_ptr<T>&&);
    template <typename T, typename X> friend cloning_ptr<T> dynamic_pointer_cast(const cloning_ptr<X>& src);
    template <typename T, typename X> friend cloning_ptr<T> dynamic_pointer_cast(cloning_ptr<X>&& src);
    template <typename T, typename X> friend cloning_ptr<T> static_pointer_cast(const cloning_ptr<X>& src);
    template <typename T, typename X> friend cloning_ptr<T> static_pointer_cast(cloning_ptr<X>&& src);

    T* m_ptr = nullptr;
};


template <typename T> void swap(cloning_ptr<T>& lhs, cloning_ptr<T>& rhs) noexcept {
    lhs.swap(rhs);
}


template <typename L, typename R> auto operator<=>(const cloning_ptr<L>& lhs, const cloning_ptr<R>& rhs)
    requires std::is_same_v<L, R> || std::is_base_of_v<L, R> || std::is_base_of_v<R, L>
{
    return static_cast<const detail::cloning_ptr_base*>(lhs) <=> static_cast<const detail::cloning_ptr_base*>(rhs);
}


// Creation helper
template<class U, class... Ts>
static cloning_ptr<U> make_cloning(Ts&&... ts) 
{
    return cloning_ptr<U>{std::in_place_type<U>, std::forward<Ts>(ts)...};
}


// *_pointer_cast overloads. These come both lvalue and rvalue versions, where the former clone if the cast was successful.

template<typename T, typename X>
cloning_ptr<T> dynamic_pointer_cast(const cloning_ptr<X>& src) {
    // Early check to avoid copying if downcast can't be done. Also catches
    // that src is null.
    auto tp = dynamic_cast<const T*>(src.get());
    if (tp == nullptr) 
        return {};

    size_t offset = reinterpret_cast<const char*>(tp) - reinterpret_cast<const char*>(src.m_impl.get());

    cloning_ptr<T> ret;
    ret.m_impl = src.m_impl->clone();
    ret.m_ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(ret.m_impl.get()) + offset);
    return ret;
}

template <typename T, typename X>
cloning_ptr<T> dynamic_pointer_cast(cloning_ptr<X>&& src)
{
    auto tp = dynamic_cast<const T*>(src.get());
    if (tp == nullptr)
        return {};   // src retains its value.

    size_t offset = reinterpret_cast<const char*>(tp) - reinterpret_cast<const char*>(src.m_impl.get());

    cloning_ptr<T> ret;
    ret.m_impl = std::move(src.m_impl);
    ret.m_ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(ret.m_impl.get()) + offset);
    return ret;
}


template <typename T, typename X>
cloning_ptr<T> static_pointer_cast(const cloning_ptr<X>& src) {
    // Early check to avoid copying if downcast can't be done. Also catches
    // that src is null.
    auto tp = static_cast<const T*>(src.get());
    if (tp == nullptr) 
        return {};

    size_t offset = reinterpret_cast<const char*>(tp) - reinterpret_cast<const char*>(src.m_impl.get());

    cloning_ptr<T> ret;
    ret.m_impl = src.m_impl->clone();
    ret.m_ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(ret.m_impl.get()) + offset);
    return ret;
}

template <typename T, typename X>
cloning_ptr<T> static_pointer_cast(cloning_ptr<X>&& src) {
    auto tp = static_cast<const T*>(src.get());
    if (tp == nullptr) 
        return {};  // src retains its value.

    size_t offset = reinterpret_cast<const char*>(tp) - reinterpret_cast<const char*>(src.m_impl.get());

    cloning_ptr<T> ret;
    ret.m_impl = std::move(src.m_impl);
    ret.m_ptr = reinterpret_cast<T*>(reinterpret_cast<char*>(ret.m_impl.get()) + offset);
    return ret;
}

}  // namespace xyz

#endif  // XYZ_CLONING_PTR_H_
