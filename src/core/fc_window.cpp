#include "fc_window.hpp"
#include <SDL_video.h>

// - FROLIC ENGINE -

// - EXTERNAL LIBRARIES -
#include "SDL2/SDL.h"
//#include "SDL2/SDL_error.h"
// #include "SDL2/SDL_messagebox.h"
// #include "SDL2/SDL_video.h"
#include "SDL2/SDL_vulkan.h"
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace fc {


  bool FcWindow::initWindow(uint32_t width, uint32_t height, bool isFullscreen, std::string name)
  {
     //TODO not sure this is even necessary to store the window extent?
     //mScreenSize = {width, height};
     // SDL_Init() initializes assertions and crash protection
     // and then calls SDL_InitSubSystem(). TODO bypass those protections
     // by calling SDL_InitSubSystem() directly for release.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) < 0)
    {
      std::ostringstream errorMsg;
      errorMsg << "Failed to initialize SDL! SDL Error: " << SDL_GetError();
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL", errorMsg.str().c_str(), nullptr);

      return false;
    }

     // create fullscreen window
     // TODO On MacOS NSHighResolutionCapable must be set to true in the
     // application Info.plist for SDL_WINDOW_ALLOW_HIGHDPI to have any effect
    if (isFullscreen)
    {
      mWindow = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
                                 , width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
                                 | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_FULLSCREEN);
    }
    else
    {
      mWindow = SDL_CreateWindow(name.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED
                                 , width, height, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
                                 | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
    }

    if (mWindow == nullptr)
    {
      std::ostringstream errorMsg;
      errorMsg << "Window coulde not be created! SDL Error: " << SDL_GetError();
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL", errorMsg.str().c_str(), mWindow);
      return false;
    }

    VkExtent2D wsize = ScreenSize();
    std::cout << "mWindow dimensions init window: " << wsize.width
              << " x " << wsize.height << std::endl;

    return true;
  }



  const VkExtent2D FcWindow::ScreenSize()
  {
    int width, height;
     // SDL_GetWindowSize(mWindow, &width, &height);
    SDL_Vulkan_GetDrawableSize(mWindow, &width, &height);


    VkExtent2D winExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    return winExtent;
  }



  void FcWindow::createWindowSurface(const VkInstance& instance)
  {
    if (SDL_Vulkan_CreateSurface(mWindow, instance, &mSurface) != SDL_TRUE)
    {
      std::ostringstream errorMsg;
      errorMsg << "Surface could not be created! SDL Error: " << SDL_GetError();

      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL Error"
                               , errorMsg.str().c_str(), mWindow);

      throw std::runtime_error("failed to create window surface!");
    }

     // TODO Create ImGui framebuffers from here ?
    // int width, height;
    // SDL_GetWindowSize(mWindow, &width, &height);

  }


  void FcWindow::close(VkInstance& instance)
  {
    vkDestroySurfaceKHR(instance, mSurface, nullptr);
    SDL_DestroyWindow(mWindow);
    mWindow = nullptr;
    SDL_Quit();
  }

} // NAMESPACE fc
