#pragma once

#ifndef _HAS_CXX17
#ifdef _MSVC_LANG
#if _MSVC_LANG > 201402
#define _HAS_CXX17	1
#else /* _MSVC_LANG > 201402 */
#define _HAS_CXX17	0
#endif /* _MSVC_LANG > 201402 */
#else /* _MSVC_LANG */
#if __cplusplus > 201402
#define _HAS_CXX17	1
#else /* __cplusplus > 201402 */
#define _HAS_CXX17	0
#endif /* __cplusplus > 201402 */
#endif /* _MSVC_LANG */
#endif /* _HAS_CXX17 */

#ifndef _NODISCARD
#if _HAS_CXX17
#define _NODISCARD [[nodiscard]]	
#else
#define _NODISCARD	
#endif
#endif//_NODISCARD

#ifndef _CRT_STRINGIZE
#define _CRT_STRINGIZE_(x) #x
#define _CRT_STRINGIZE(x) _CRT_STRINGIZE_(x)
#endif

#ifndef _CRT_WIDE
#define _CRT_WIDE_(s) L ## s
#define _CRT_WIDE(s) _CRT_WIDE_(s)
#endif

#ifndef _CRT_CONCATENATE
#define _CRT_CONCATENATE_(a, b) a ## b
#define _CRT_CONCATENATE(a, b)  _CRT_CONCATENATE_(a, b)
#endif


#ifndef _CRT_UNPARENTHESIZE
#define _CRT_UNPARENTHESIZE_(...) __VA_ARGS__
#define _CRT_UNPARENTHESIZE(...)  _CRT_UNPARENTHESIZE_ __VA_ARGS__
#endif

#ifndef __has_cpp_attribute // vvv no attributes vvv
#define _LIKELY
#define _UNLIKELY
#elif __has_cpp_attribute(likely) >= 201803L && __has_cpp_attribute(unlikely) >= 201803L // ^^^ no attr / C++20 attr vvv
#define _LIKELY   [[likely]]
#define _UNLIKELY [[unlikely]]
#elif defined(__clang__) // ^^^ C++20 attributes / clang attributes and C++17 or C++14 vvv
#define _LIKELY   [[__likely__]]
#define _UNLIKELY [[__unlikely__]]
#else // ^^^ clang attributes and C++17 or C++14 / C1XX attributes and C++17 or C++14 vvv
#define _LIKELY
#define _UNLIKELY
#endif // ^^^ C1XX attributes and C++17 or C++14 ^^^


