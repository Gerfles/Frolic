//>--- log.cpp ---<//
#include "log.hpp"


// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#if defined(_MSC_VER)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#include <stdio.h>
#include <stdarg.h>
/* #include <ostream> */
#include <iostream>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


// TODO format according to Fc conventions
namespace fc
{
// TODO remove duplicate functionality provided by log etc.
// TODO clear output.txt each time and think about printing differently
inline void dprintf(const char* fmt, ...)
{
  va_list parms;
  static char buf[2048] = {0};

   // try to print in the allocated space
  va_start(parms, fmt);
  vsprintf(buf, fmt, parms);
  va_end(parms);

  // write the information out ot a txt file
  //  #if 0
  FILE *fp = fopen("output.txt", "w");
  fprintf(fp, "%s", buf);
  fclose(fp);
   // #endif

   // output to the visual studio window
   // OutputDebugStringA( buf );

}// --- dprintf (_) --- (END)


//
  LogService              s_log_service;

  static constexpr u32    k_string_buffer_size = 1024 * 1024;
  static char log_buffer[k_string_buffer_size];
  static void output_console( char* log_buffer_ ) {
    printf( "%s", log_buffer_ );
    std::flush(std::cout);
  }

#if defined(_MSC_VER)
  static void output_visual_studio( char* log_buffer_ ) {
    OutputDebugStringA( log_buffer_ );
  }
#endif

  LogService* LogService::instance() {
    return &s_log_service;
  }

  void LogService::print_format( cstring format, ... ) {
    va_list args;

    va_start( args, format );
#if defined(_MSC_VER)
    vsnprintf_s( log_buffer, ARRAY_SIZE( log_buffer ), format, args );
#else
    vsnprintf( log_buffer, ARRAY_SIZE( log_buffer), format, args );
#endif
    log_buffer[ ARRAY_SIZE( log_buffer ) - 1 ] = '\0';
    va_end( args );

    output_console( log_buffer );
#if defined(_MSC_VER)
    output_visual_studio( log_buffer );
#endif // _MSC_VER

    if ( printCallback )
      printCallback( log_buffer );
  }

  void LogService::set_callback( PrintCallback callback ) {
    printCallback = callback;
  }

} // --- namespace fc  --- (END)
