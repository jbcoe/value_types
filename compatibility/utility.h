#ifndef XYZ_UTILITY_H
#define XYZ_UTILITY_H

#ifdef XYZ_POLYMORPHIC_HAS_STD_IN_PLACE_TYPE
#error "XYZ_POLYMORPHIC_HAS_STD_IN_PLACE_TYPE is already defined"
#endif

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define XYZ_POLYMORPHIC_HAS_STD_IN_PLACE_TYPE
#endif

namespace xyz {

#ifdef XYZ_POLYMORPHIC_HAS_STD_IN_PLACE_TYPE
using std::in_place_type_t;
#else
template <class T>
struct in_place_type_t {};
#endif  // XYZ_POLYMORPHIC_HAS_STD_IN_PLACE_TYPE

}  // namespace xyz

#undef XYZ_POLYMORPHIC_HAS_STD_IN_PLACE_TYPE

#endif  // XYZ_UTILITY_H
