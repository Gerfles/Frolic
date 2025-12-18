#pragma once

#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <limits>
#include <stdint.h>
#include <sys/types.h>

#if !defined(_MSC_VER)
#include <signal.h>
#endif

namespace fc
{
  constexpr float PI = 3.14159265358f;
  constexpr float TWO_PI = PI * 2.0f;
  constexpr float PI_OVER_TWO = PI / 2.0f;
  constexpr float DEG_TO_RAD_FACTOR = PI / 180.0f;
  constexpr float DEG_TO_RAD_OVER_TWO_FACTOR = PI_OVER_TWO / 180.f;

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
#define    FROLIC_UNIQUE_SUFFIX(PARAM)             FROLIC_CONCAT(PARAM, __LINE__)

// *-*-*-*-*-*-*-*-*-*-*-*-*-   NATIVE TYPE TYPEDEFS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
  typedef  uint8_t   u8;
  typedef  uint16_t  u16;
  typedef  uint32_t  u32;
  typedef  uint64_t  u64;

  typedef  int8_t    i8;
  typedef  int16_t   i16;
  typedef  int32_t   i32;
  typedef  int64_t   i64;

// ?? Not sure this is correct for every implementation
// typedef  float     f32;
// typedef  double    f64;

  typedef  size_t    sizeT;

  typedef const char*  cstring;

  static const u64  U64_MAX  =  UINT64_MAX;
  static const u32  U32_MAX  =  UINT32_MAX;
  static const u16  U16_MAX  =  UINT16_MAX;
  static const u8   U8_MAX   =  UINT8_MAX;

  static const i64  I64_MAX  =  INT64_MAX;
  static const i32  I32_MAX  =  INT32_MAX;
  static const i16  I16_MAX  =  INT16_MAX;
  static const i8   I8_MAX   =  INT8_MAX;

  static const float FLOAT_MAX = std::numeric_limits<float>::max();
  static const float FLOAT_MIN = std::numeric_limits<float>::min();

  // TODO Place here for now but relocate into tweaks file
  // Total number of textures allowed to be bound dynamically
  static const u32 MAX_BINDLESS_RESOURCES = 1024;
  //?? not sure why we use 10 here
  static const uint32_t BINDLESS_TEXTURE_BIND_SLOT = 10;

  static const glm::mat4 ID_MATRIX{1.0f};
}
