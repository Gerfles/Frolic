#pragma once

#include <cstdint>
#include <stdint.h>
#include <sys/types.h>

#if !defined(_MSC_VER)
#include <signal.h>
#endif

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MACROS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#define ArraySize(array) (sizeof(array) / sizeof((array)[0]))

// TODO look into best practices
#if defined(_MSC_VER)
//#define FROLIC_INLINE inline
#define    FROLIC_FINLINE                          __forceinline
#define    FROLIC_DEBUG_BREAK                      __debugbreak();
#define    FROLIC_DISABLE_WARNING(warning_number)  __pragma(warning(disable : warning_number))
#define    FROLIC_CONCAT_OPERATOR(x, y)            x##y
#else
#define    FROLIC_FINLINE                          always_inline
#define    FROLIC_DEBUG_BREAK                      raise(SIGTRAP);
#define    FROLIC_CONCAT_OPERATOR(x, y)            x y
//#define  FROLIC_INLINE                           inline
#endif // MSVC

#define    FROLIC_STRINGTIZE(L)                    #L
#define    FROLIC_MAKESTRING(L)                    FROLIC_STRINGTIZE(L)
#define    FROLIC_CONCAT(x, y)                     FROLIC_CONCAT_OPERATOR(x, y)
#define    FROLIC_LINE_STRING                      FROLIC_MAKESTRING(__LINE__)
#define    FROLIC_FILELINE(MESSAGE)                __FILE__ "(" FROLIC_LINE_STRING ") : " MESSAGE

// unique names
#define    FROLIC_UNIQUE_SUFFIX(PARAM)             RAPTOR_CONCAT(PARAM, __LINE__)

// *-*-*-*-*-*-*-*-*-*-*-*-*-   NATIVE TYPE TYPEDEFS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
typedef  uint8_t   u8;
typedef  uint16_t  u16;
typedef  uint32_t  u32;
typedef  uint64_t  u64;

typedef  int8_t    i8;
typedef  int16_t   i16;
typedef  int32_t   i32;
typedef  int64_t   i64;

typedef  float     f32;
typedef  double    f64;

typedef  size_t    sizet;

typedef const char*  cstring;

static const u64  u64_max  =  UINT64_MAX;
static const u32  u32_max  =  UINT32_MAX;
static const u16  u16_max  =  UINT16_MAX;
static const u8   u8_max   =  UINT8_MAX;

static const i64  i64_max  =  INT64_MAX;
static const i32  i32_max  =  INT32_MAX;
static const i16  i16_max  =  INT16_MAX;
static const i8   i8_max   =  INT8_MAX;
