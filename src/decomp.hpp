#pragma once
// Helpers to improve the diff quality of DIFFBUILDs.
//
// A DIFFBUILD is a build of the project made not to be run, but to be diffed
// against the original TH06 to find inaccuracies in our reimplementation. To
// make a DIFFBUILD, one must call the makefile with DIFFBUILD=1
//
// This helper provides two macro groups: DIFFABLE_EXTERN and DIFFABLE_STATIC.
// All static variables should be defined and declared through those macros. So
// for instance, instead of
//
// ```
// Stage g_Stage;
// ```
//
// One should write
//
// ```
// DIFFABLE_STATIC(Stage, g_Stage);
// ```
//
// The first argument is the type, while the second argument is the name of the
// static variable.
//
// When assigning to an array, the `DIFFABLE_STATIC_ARRAY_ASSIGN` macro should
// be used like this:
//
// ```
// DIFFABLE_STATIC_ARRAY_ASSIGN(u32, 5, g_ArrayName) = { 0, 1, 2 };
// ```
//
// The SORTED variants are used as a workaround; MSVC's linker orders
// zero-initialized and uninitialized globals based on name in a hard to
// predict manner that would effectively require figuring out what ZUN called
// his variables, which is less than desirable for obvious reasons.
// This allows us to override the order the linker would normally choose.
// Is it pretty? No. But MSVC has unfortunately forced our hand.

#include "dxutil.hpp"

#define _MACRO_CATW(arg1, arg2, arg3) arg1##arg2##arg3
#define MACRO_CATW(arg1, arg2, arg3) _MACRO_CATW(arg1, arg2, arg3)
#define _MACRO_CAT(arg1, arg2) arg1##arg2
#define MACRO_CAT(arg1, arg2) _MACRO_CAT(arg1, arg2)
#define _MACRO_STR(arg) #arg
#define MACRO_STR(arg) _MACRO_STR(arg)

#ifdef DIFFBUILD
#define DIFFABLE_EXTERN(type, name) extern "C" type name
#define DIFFABLE_EXTERN_ARRAY(type, size, name) extern "C" type name[size]
#define DIFFABLE_STATIC(type, name) extern "C" type name
#define DIFFABLE_STATIC_ARRAY(type, size, name) extern "C" type name[size]
// This macro is meant to be used like so:
// DIFFABLE_STATIC_ARRAY_ASSIGN(u32, g_ArrayName) = 12;
//
// In diffbuild, we want to discard the content of the array, so we generate a
// second, fake static, that we store in a template<> to make sure it doesn't
// get instanciated.
#define DIFFABLE_STATIC_ASSIGN(type, name)                                                                             \
    extern "C" type name;                                                                                              \
    template <> type DIFFBUILD_HIDE_NAME_##name
// This macro is meant to be used like so:
// DIFFABLE_STATIC_ARRAY_ASSIGN(u32, 5, g_ArrayName) = { 0, 1, 2 };
//
// In diffbuild, we want to discard the content of the array, so we generate a
// second, fake static, that we store in a template<> to make sure it doesn't
// get instanciated.
#define DIFFABLE_STATIC_ARRAY_ASSIGN(type, size, name)                                                                 \
    extern "C" type name[size];                                                                                        \
    template <> type DIFFBUILD_HIDE_NAME_##name[size]
#define DIFFABLE_STATIC_SORTED(sort, type, name) DIFFABLE_STATIC(type, name)
#define DIFFABLE_STATIC_ARRAY_SORTED(sort, type, size, name) DIFFABLE_STATIC_ARRAY(type, size, name)
#define FILE_BSS_SORT(sort)
#else
#define DIFFABLE_EXTERN(type, name) extern type name
#define DIFFABLE_EXTERN_ARRAY(type, size, name) extern type name[size]
#define DIFFABLE_STATIC(type, name) type name
#define DIFFABLE_STATIC_ARRAY(type, size, name) type name[size]
#define DIFFABLE_STATIC_ASSIGN(type, name) type name
#define DIFFABLE_STATIC_ARRAY_ASSIGN(type, size, name) type name[size]
#define DIFFABLE_STATIC_SORTED(sort, type, name)                                                                       \
    __pragma(section(MACRO_STR(MACRO_CATW(.data$, sort, name)), read, write))                                          \
        __declspec(allocate(MACRO_STR(MACRO_CATW(.data$, sort, name))))                                                \
        DIFFABLE_STATIC(type, name)
#define DIFFABLE_STATIC_ARRAY_SORTED(sort, type, size, name)                                                           \
    __pragma(section(MACRO_STR(MACRO_CATW(.data$, sort, name)), read, write))                                          \
        __declspec(allocate(MACRO_STR(MACRO_CATW(.data$, sort, name))))                                                \
        DIFFABLE_STATIC_ARRAY(type, size, name)
#define FILE_BSS_SORT(sort)                                                                                            \
    __pragma(section(MACRO_STR(MACRO_CATW(.data$, sort, __LINE__))))                                                   \
        __pragma(bss_seg(MACRO_STR(MACRO_CATW(.data$, sort, __LINE__))))
#endif

// Using __COUNTER__ would be better but makes PCH *really* slow
#define unique_name(prefix) MACRO_CAT(prefix, __LINE__)

// Generates a compile error without any global name pollution
#define STATIC_ASSERT(cond) struct { unsigned char : !!(cond); }
#define STATIC_ASSERT_NAME(name, cond) struct { unsigned char MACRO_CAT(assert_,name) : !!(cond); }

// just pretend we're living in C++11
#define alignof(type) __alignof(type)
#define ZUN_ASSERT_SIZE(type, size) STATIC_ASSERT_NAME(size_##type##_not_##size, sizeof(type) == (size))
#define ZUN_ASSERT_ALIGN(type, align) STATIC_ASSERT_NAME(align_##type##_not_##align, alignof(type) == (align))
#define ZUN_ASSERT_TYPE(type, size, align) ZUN_ASSERT_SIZE(type, size); ZUN_ASSERT_ALIGN(type, align)

#define unknown_name       unique_name(unknown_)
#define unreferenced_name  unique_name(unreferenced_)
#define unused_name        unique_name(unused_)

template<bool cond, typename T, typename F>
struct conditional {
    template<bool>
    struct impl {
        typedef F type;
    };
    template<>
    struct impl<true> {
        typedef T type;
    };
    typedef typename template impl<cond>::type type;
};

#pragma pack(push, 1)
template<unsigned int bytes>
struct TerribleNonGSBufferPaddingA {
    unsigned char pad[bytes];
};
template<unsigned int bytes>
struct TerribleNonGSBufferPaddingB {
    void* padA[bytes / 4];
    unsigned char padB[bytes % 4];
};
template<unsigned int bytes>
struct TerribleNonGSBufferPaddingC {
    void *padA[bytes / 4];
};
#pragma pack(pop)
#define TerribleNonGSBufferPadding(bytes) \
conditional<(bytes>=4),conditional<!!(bytes%4),TerribleNonGSBufferPaddingB<bytes>,TerribleNonGSBufferPaddingC<bytes>/**/>::type,TerribleNonGSBufferPaddingA<bytes>/**/>::type

// Used for blocks of data that still need research
#define unknown_fields(size) TerribleNonGSBufferPadding(size) unknown_name
#define unknown_bitfields(type, size) type : size
// Used for blocks of data that are known to be totally unused anywhere
#define unreferenced_fields(size) TerribleNonGSBufferPadding(size) unreferenced_name
#define unreferenced_bitfields(type, size) type : size
// Used for cases where data type is known despite no uses
#define unused_field(type) type unused_name
#define unused_array_field(type, size) type unused_name[size]

#if VALIDATE_ALIGNMENT_PADDING
#define alignment_padding(size) unreferenced_fields(size)
#define alignment_bitfields(type, size) type : size
#else
// Intentionally left blank to avoid potential effects
#define alignment_padding(size)
#define alignment_bitfields(type, size)
#endif

#define unreferenced_variable(type) type unreferenced_name
#define unreferenced_array_variable(type, size) type unreferenced_name[size]

typedef signed char i8;
typedef unsigned char u8;
typedef short i16;
typedef unsigned short u16;
typedef int i32;
typedef unsigned int u32;
typedef float f32;
typedef double f64;

#define ARRAY_SIZE(x)(sizeof(x) / sizeof(x[0]))
#define ARRAY_SIZE_SIGNED(x) ((i32)sizeof(x) / (i32)sizeof(x[0]))
