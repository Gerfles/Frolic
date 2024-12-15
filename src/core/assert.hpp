// TODO find out how assert works
//   #define assert(e)  \
//     ((void) ((e) ? ((void)0) : __assert (#e, __ASSERT_FILE_NAME, __LINE__)))
// #define __assert(e, file, line) \
//     ((void)printf ("%s:%d: failed assertion `%s'\n", file, line, e), abort())

#pragma once

#include "log.hpp"

namespace frolic {

#define FCASSERT(condition) if(!(condition)) {   \
  fcPrint(FROLIC_FILELINE("FALSE\n"));            \
  FROLIC_DEBUG_BREAK                            \
}

#if defined (_MSC_VER)
#define FCASSERTM(condition, message, ...) if(!(condition)) {            \
  fcPrint(FROLIC_FILELINE(FROLIC_CONCAT(message, "\n")), __VA_ARGS__);    \
  FROLIC_DEBUG_BREAK                                                    \
}
#else
#define FCASSERTM(condition, message, ...) if (!(condition)) {           \
  fcPrint(FROLIC_FILELINE(FROLIC_CONCAT(message, "\n")), ## __VA_ARGS__); \
  FROLIC_DEBUG_BREAK                                                    \
}
#endif // MSCV

} // --- namespace frolic  --- (END)
