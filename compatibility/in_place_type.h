#ifndef XYZ_IN_PLACE_TYPE_H
#define XYZ_IN_PLACE_TYPE_H

namespace xyz {

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
using std::in_place_type_t;
#else
template <class T>
struct in_place_type_t {};
#endif  // (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >=
        // 201703L)

}  // namespace xyz

#endif  // XYZ_IN_PLACE_TYPE_H
