#ifndef UTIL_PSTRUCT_H_
#define UTIL_PSTRUCT_H_

#include "base/platform.h"

// Macros to assist in creating packed structs that are aligned to byte
// boundaries of 1 or 2.
//
// Keeping pstruct etc.. lower-case breaks styling rules, but we make an
// exception to maintain visual similarity with keywords.

#if defined(__clang__) || defined(__GNUC__)
#if !defined(__i386__) && !defined(__amd64__)
#warning("Explicit struct alignments may be unsafe on this platform.")
#endif
#define pstruct struct __attribute__((packed))
#define aligned_struct(_N_) struct __attribute__((aligned(_N_)))
#elif defined(_MSC_VER)
#if !defined(__i386__) && !defined(__amd64__)
#pragma message("Explicit struct alignments may be unsafe on this platform.")
#endif
#define pstruct __pragma(pack(1)) struct
#define aligned_struct(_N_) __pragma(pack(_N_)) struct
#else
#error pstruct is not available on this compiler.
#endif

#endif  // UTIL_PSTRUCT_H_
