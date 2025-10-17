
//#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "frolic.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <iostream>
//#include <exception>
// vs. ??


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

// TODO with all submodules (fast_gltf, stb, etc)
// warning: adding embedded git repository: external/tlsf
// hint: You've added another git repository inside your current repository.
// hint: Clones of the outer repository will not contain the contents of
// hint: the embedded repository and will not know how to obtain it.
// hint: If you meant to add a submodule, use:
// hint:
// hint:   git submodule add <url> external/tlsf
// hint:
// hint: If you added this path by mistake, you can remove it from the
// hint: index with:
// hint:
// hint:   git rm --cached external/tlsf
// hint:
// hint: See "git help submodule" for more information.
// hint: Disable this message with "git config set advice.addEmbeddedRepo false"
