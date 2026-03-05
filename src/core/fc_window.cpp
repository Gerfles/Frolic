//>--- fc_window.cpp ---<//
#include "fc_window.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "log.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <string>
#include <iostream>
#include <sstream>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  bool FcWindow::initWindow(VkExtent2D screenDimensions, std::string name, bool isFullscreen)
  {
    // TODO allow application parameter to choose set.
    // TODO some work must be done in order for x11 to run corectly
    if (false)
    {
      if (!SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11"))
      {
        fcPrintEndl("Failed to set x11 override hint");
      }
    }

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

    // TODO On MacOS NSHighResolutionCapable must be set to true in the
    // application Info.plist for SDL_WINDOW_ALLOW_HIGHDPI to have any effect
    uint32_t windowCreateFlags = SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI;

    // create fullscreen window
    if (isFullscreen)
    {
      windowCreateFlags |= SDL_WINDOW_FULLSCREEN;
    }
    else
    {
      windowCreateFlags |= SDL_WINDOW_RESIZABLE;
    }

    mWindow = SDL_CreateWindow(name.c_str(),
                               SDL_WINDOWPOS_UNDEFINED,
                               SDL_WINDOWPOS_UNDEFINED,
                               screenDimensions.width, screenDimensions.height,
                               windowCreateFlags);

    if (mWindow == nullptr)
    {
      std::ostringstream errorMsg;
      errorMsg << "Window coulde not be created! SDL Error: " << SDL_GetError();
      SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL", errorMsg.str().c_str(), mWindow);
      return false;
    }

    VkExtent2D wsize = ScreenSize();
    fcPrintEndl("Window dimensions: %u x %u", wsize.width, wsize.height);
    return true;
  }



  const VkExtent2D FcWindow::ScreenSize()
  {
    int width, height;

    SDL_Vulkan_GetDrawableSize(mWindow, &width, &height);

    VkExtent2D winExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    return winExtent;
  }



  void FcWindow::createWindowSurface(const VkInstance& instance)
  {
    if (SDL_Vulkan_CreateSurface(mWindow, instance, &mSurface) != SDL_TRUE)
    {
      std::ostringstream errorMsg;
      errorMsg << "Surface could not be created!\n" << SDL_GetError();

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
