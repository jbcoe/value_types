#include <utility>

namespace xyz
{
    template <typename T, typename A>
    class indirect
    {
    public:
        // Default constructor
        indirect();

        // Copy constructor
        indirect(const indirect &p);

        // Move constructor
        indirect(indirect &&p);

        // Value constructor
        template <typename... Ts>
        indirect(std::in_place_t, Ts &&...ts);

        // Pointer constructor
        template <typename C, typename D>
        indirect(T*, C c, D d);

        // Destruction.
        ~indirect();

        // Assignment
        indirect &operator=(const indirect &p);

        indirect &operator=(indirect &&p);

        // Observers
        operator bool() const noexcept;

        // Accessors
        const T* operator->() const noexcept;
        const T& operator*() const noexcept;

        // Modifiers
        T* operator->() noexcept;
        T& operator*() noexcept;

        // Member swap
        void swap(indirect &p) noexcept;

        // Non-member swap
        friend std::swap(indirect &lhs, indirect &rhs) noexcept;

        // Factory methods
        template <typename T, typename U, typename... Ts>
        friend indirect<T, A> make_indirect(Ts &&...ts);
    };
} // namespace xyz