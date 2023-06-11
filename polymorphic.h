#include <utility>

namespace xyz
{
    template <typename T, typename A>
    class polymorphic
    {
    public:
        // Default constructor
        polymorphic();

        // Copy constructor
        polymorphic(const polymorphic &p);

        // Move constructor
        polymorphic(polymorphic &&p);

        // Value constructor
        template <typename U, typename... Ts>
        polymorphic(std::in_place_type_t<U>, Ts &&...ts);

        // Pointer constructor
        template <typename U, typename C, typename D>
        polymorphic(U *, C c, D d);

        // Converting constructors
        template <typename U>
        polymorphic(const polymorphic<U, A> &p);

        template <typename U>
        polymorphic(polymorphic<U, A> &&p);

        // Destruction.
        ~polymorphic();

        // Assignment
        polymorphic &operator=(const polymorphic &p);

        polymorphic &operator=(polymorphic &&p);

        // Converting assignment
        template <typename U>
        polymorphic &operator=(const polymorphic<U, A> &p);

        template <typename U>
        polymorphic &operator=(polymorphic<U, A> &&p);

        // Observers
        operator bool() const noexcept;

        // Accessors
        const T* operator->() const noexcept;
        const T& operator*() const noexcept;

        // Modifiers
        T* operator->() noexcept;
        T& operator*() noexcept;

        // Member swap
        void swap(polymorphic &p) noexcept;

        // Non-member swap
        friend std::swap(polymorphic &lhs, polymorphic &rhs) noexcept;

        // Factory methods
        template <typename T, typename U, typename... Ts>
        friend polymorphic<T, A> make_polymorphic(Ts &&...ts);
    };
} // namespace xyz