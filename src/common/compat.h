/* Portable compiler feature switches shared by the C11 modules. */
#ifndef TICKTRADE_COMPAT_H
#define TICKTRADE_COMPAT_H

#if defined(__GNUC__) || defined(__clang__)
#define COMPAT_NOINLINE __attribute__((noinline))
#define COMPAT_OPTIMIZER_GUARD(value) __asm__ volatile("" : "+r"(value))
#else
#define COMPAT_NOINLINE
#define COMPAT_OPTIMIZER_GUARD(value) ((void)(value))
#endif

#if defined(__SIZEOF_INT128__)
#define COMPAT_HAS_INT128 1
#else
#define COMPAT_HAS_INT128 0
#endif

#endif
