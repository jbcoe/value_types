// An exploration of traits and concepts in C++ with incomplete types.

#include <concepts>
#include <map>
#include <type_traits>

namespace xyz::testing::traits_and_concepts {

class Incomplete;

// type-trait based checks will fail as type_traits cannot be evaluated on
// incomplete types. AppleClang 16.0.0 (clang-1600.0.26.6):
//  error: incomplete type 'xyz::testing::traits_and_concepts::Incomplete' used
//  in type trait expression
// static_assert(!std::is_copy_constructible_v<Incomplete>);
// static_assert(std::is_copy_constructible_v<Incomplete>);

// Concept check fails as AppleClang implements concept using type traits.
// AppleClang 16.0.0 (clang-1600.0.26.6):
//  error: incomplete type 'xyz::testing::traits_and_concepts::Incomplete' used
//  in type trait expression
// static_assert(!std::copy_constructible<Incomplete>);

// Define our own concept to avoid restrictions from type traits.
template <typename T>
concept CopyConstructible = requires(const T& t) {
  { T(t) } -> std::same_as<T>;
};

// The check below passes as the concept's requires clause is not satisfied for
// an incomplete type.
static_assert(!CopyConstructible<Incomplete>);

class Incomplete {};

// The check below will fail if CopyConstructible<Incomplete> was evaluated
// above as the value of a concept does not change from one point of use to
// another. If CopyConstructible<Incomplete> not was evaluated above, the check
// below will pass as Incomplete is now complete and satisfies the concept
// requirements. static_assert(CopyConstructible<Incomplete>);

}  // namespace xyz::testing::traits_and_concepts
