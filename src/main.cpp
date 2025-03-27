
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
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

   fc::initEnv();

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
