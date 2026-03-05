//>--- platform.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#if !defined(_MSC_VER)
#include <signal.h>
#endif
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  constexpr float PI = 3.14159265358f;
  constexpr float TWO_PI = PI * 2.0f;
  constexpr float PI_OVER_TWO = PI / 2.0f;
  constexpr float DEG_TO_RAD_FACTOR = PI / 180.0f;
  constexpr float DEG_TO_RAD_OVER_TWO_FACTOR = PI_OVER_TWO / 180.f;

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MACROS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#define ArraySize(array) (sizeof(array) / sizeof((array)[0]))

// TEST windows versions
#if defined(_MSC_VER) // OR (_WIN32)
//#define FROLIC_INLINE inline
#define    FC_FINLINE                          __forceinline
#define    FC_DEBUG_BREAK                      __debugbreak();
#define    FC_DISABLE_WARNING(warning_number)  __pragma(warning(disable : warning_number))
#define    FC_CONCAT(x, y)            x##y
#else // works in most POSIX systems and should be fine in macOS
#define    FC_FINLINE                          always_inline
#define    FC_DEBUG_BREAK                      raise(SIGTRAP);
#define    FC_CONCAT(x, y)            x y
#define    FC_INLINE                           inline
#endif // (_MSVC_VER) defined

  //

// unique names
#define    FC_UNIQUE_SUFFIX(PARAM)             FC_CONCAT(PARAM, __LINE__)

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
  static const u32 BINDLESS_TEXTURE_BIND_SLOT = 10;

  static const glm::mat4 ID_MATRIX{1.0f};
}
