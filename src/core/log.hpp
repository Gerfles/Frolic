#pragma once

#include "platform.hpp"
#include "service.hpp"

namespace fc
{
   // Pointer-to-function type for Additional callback to print
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
  fc::LogService::instance()->print_format("\n");
#else
 #define fcPrint(format, ...) fc::LogService::instance()->print_format(format, ## __VA_ARGS__);
 #define fcPrintEndl(format, ...) fc::LogService::instance()->print_format(format, ## __VA_ARGS__); \
  fc::LogService::instance()->print_format("\n");
#endif

} // --- namespace fc --- (END)
