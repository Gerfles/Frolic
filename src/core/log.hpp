//>--- log.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "platform.hpp"
#include "fc_service.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{


  // Pointer-to-function type for Additional callback to print
  // TODO change to using
  typedef void (*PrintCallback)(const char*);

  struct LogService : public Service
  {
     PrintCallback printCallback = nullptr;
     static constexpr cstring kName = "frolic_log_service";

     FC_DECLARE_SERVICE(LogService);

     void print_format(cstring format, ... );
     void set_callback(PrintCallback callback);
  };

#if defined (_MSC_VER)
 #define fcPrint(format, ...) fc::LogService::instance()->print_format(format, __VA_ARGS__);
 #define fcPrintEndl(format, ...) fc::LogService::instance()->print_format(format, __VA_ARGS__); \
  fc::LogService::instance()->print_format("\n")
#else
  // TODO change many of the print statements when release ver. to Log instead
#define fcPrint(format, ...) fc::LogService::instance()->print_format(format, ## __VA_ARGS__)
#define fcPrintEndl(format, ...) fc::LogService::instance()->print_format(format, ## __VA_ARGS__); \
  fc::LogService::instance()->print_format("\n")
#endif

  // TODO implement throughout
#ifndef FC_DEBUG_LOG_FORMAT
/* #define FC_DEBUG_LOG_FORMAT(format, ...) */
  #define FC_DEBUG_LOG_FORMAT(format, ...)                \
    do                                                    \
    {                                                     \
      fcPrintEndl((format), __VA_ARGS__);                 \
    }                                                     \
    while (false)
  //
#endif

#ifndef  FC_DEBUG_LOG
  #define FC_DEBUG_LOG(str) FC_DEBUG_LOG_FORMAT("%s", (str))
#endif

} // --- namespace fc --- (END)
