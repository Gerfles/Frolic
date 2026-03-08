//>--- main.cpp ---<//
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "frolic.hpp"
#include "core/utilities.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <iostream>
#include <exception>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


int main(int argc, char* argv[])
{
  fc::Frolic frolic;

  // initialize in subscope so our config gets taken off the stack once it's used
  {
    // First setup all needed parameters
    fc::FcConfig config;
    config.appVersionMajor = 0;
    config.appVersionMinor = 1;
    config.appVersionPatch = 0;
    config.windowWidth = 2100;
    config.windowHeight = 1600;
    config.mouseDeadzone = 50;
    config.applicationName = "Frolic Engine Test";  		   // Our application name
    // TODO implement  utilizing functors for actual function calls
    config.enableNonUniformScaline = false;

    // initialize engine
    frolic.init(config);
  }

  try
  {
   frolic.run();
  }
  catch (const std::exception& err)
  {
    std::cerr << err.what() << std::endl;

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
