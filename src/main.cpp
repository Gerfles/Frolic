// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "frolic.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <iostream>
//#include <exception>
// vs. ??
#include <stdexcept>



int main(int argc, char* argv[])
{
   //TODO research which NDEBUG or _DEBUG the system actully uses
  #ifdef NDEBUG
   std::cout << "-Release Build-" << std::endl;
  #else
   std::cout << "-Debug Build-" << std::endl;
  #endif

   int type;
   type = 1;

   fc::Frolic frolic;

  try
  {
   frolic.run();
  }
  catch (const std::exception& err)
  {
    std::cerr << err.what() << '\n';
    return EXIT_FAILURE;
  }

  frolic.close();

  return EXIT_SUCCESS;
}

// TODO find out how assert works
//   #define assert(e)  \
//     ((void) ((e) ? ((void)0) : __assert (#e, __ASSERT_FILE_NAME, __LINE__)))
// #define __assert(e, file, line) \
//     ((void)printf ("%s:%d: failed assertion `%s'\n", file, line, e), abort())
