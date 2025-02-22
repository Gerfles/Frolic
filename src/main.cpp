
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
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
   // Cmake injects the NDEBUG symbols by default
  #ifndef NDEBUG
   std::cout << "\n-Debug Build-" << std::endl;
  #else
   std::cout << "\n-Release Build-" << std::endl;
  #endif

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
