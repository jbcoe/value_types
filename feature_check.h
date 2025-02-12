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

#ifndef XYZ_FEATURE_CHECK_H
#define XYZ_FEATURE_CHECK_H

// The purpose of this header is to provide macros checking whether certain C++
// features (like std::optional) are available.

//
// XYZ_HAS_STD_OPTIONAL
// The macro is defined, when std::optional is available.
//
#ifdef XYZ_HAS_STD_OPTIONAL
#error "XYZ_HAS_STD_OPTIONAL is already defined"
#endif  // XYZ_HAS_STD_OPTIONAL

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define XYZ_HAS_STD_OPTIONAL
#endif  //(__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >=
        // 201703L)

//
// XYZ_HAS_STD_IN_PLACE_TYPE_T
// The macro is defined, when std::in_place_type_t is available.
//

#ifdef XYZ_HAS_STD_IN_PLACE_TYPE_T
#error "XYZ_HAS_STD_IN_PLACE_TYPE_T is already defined"
#endif  // XYZ_HAS_STD_IN_PLACE_TYPE_T

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define XYZ_HAS_STD_IN_PLACE_TYPE_T
#endif  //(__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >=
        // 201703L)

#ifdef XYZ_HAS_STD_IN_PLACE_T
#error "XYZ_HAS_STD_IN_PLACE_T is already defined"
#endif  // XYZ_HAS_STD_IN_PLACE_T

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define XYZ_HAS_STD_IN_PLACE_T
#endif  //(__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >=
        // 201703L)

#if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define XYZ_HAS_TEMPLATE_ARGUMENT_DEDUCTION
#endif  //(__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >=
        // 201703L)

//
// XYZ_HAS_STD_MEMORY_RESOURCE
// The macro is defined, when the header <memory_resource> and its content is
// available.
//

#ifdef __has_include
#if (__cplusplus >= 201703L) && __has_include(<memory_resource>)
#include <memory_resource>
#if __cpp_lib_memory_resource >= 201603L
#define XYZ_HAS_STD_MEMORY_RESOURCE
#endif  // __cpp_lib_memory_resource >= 201603L
#endif  //(__cplusplus >= 201703L) && __has_include(<memory_resource>)
#endif  //__has_include

//
// XYZ_HAS_STD_THREE_WAY_COMPARISON
// The macro is defined when the three-way comparison operator (<=>) is
// available.
//

#ifdef XYZ_HAS_STD_THREE_WAY_COMPARISON
#error "XYZ_HAS_STD_THREE_WAY_COMPARISON is already defined"
#endif  // XYZ_HAS_STD_THREE_WAY_COMPARISON

#if (__cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#define XYZ_HAS_STD_THREE_WAY_COMPARISON
#endif  //(__cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >=
        // 202002L)

#endif  // XYZ_FEATURE_CHECK_H
