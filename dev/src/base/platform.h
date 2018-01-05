#ifndef BASE_PLATFORM_H_
#define BASE_PLATFORM_H_

// Macros that consolidate and specify platform specific defines.

#define _MSVC_WARNING_FOPEN 4996
#define _MSVC_WARNING_INT_CONVERSION 4244

#if defined(_MSC_VER)         
#define _MSVC_DISABLE_WARNING(_N_) \
__pragma(warning(disable: ## _N_ ##))
#define _MSVC_ENABLE_WARNING(_N_) \
__pragma(warning(default: ## _N_ ##))
#else
#define _MSVC_DISABLE_WARNING(_N_)
#define _MSVC_ENABLE_WARNING(_N_)
#endif                        

#if defined(_MSC_VER)         
#if defined(_M_IX86)
#define __i386__
#elif defined(_M_AMD64)
#define __amd64__
#endif
#endif                        

#endif // BASE_PLATFORM_H_