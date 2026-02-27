//>--- fc_window.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
struct SDL_Window;
#include <bits/stringfwd.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  class FcWindow
  {
   private:
      //TODO may want to include swapchain in window??
      //static void framebufferResizeCallback(SDL_Window* window, int width, int height);
     SDL_Window* mWindow{nullptr};
     VkSurfaceKHR mSurface;
     VkExtent2D mScreenSize;

   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcWindow() = default;
     ~FcWindow() = default;
     FcWindow(const FcWindow&) = delete;
     FcWindow& operator=(const FcWindow) = delete;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     bool initWindow(uint32_t width,
                     uint32_t height,
                     std::string windowName,
                     bool isFullscreen = false);
     const VkExtent2D ScreenSize();
     void createWindowSurface(const VkInstance& instance);
     SDL_Window* SDLwindow() const { return mWindow; }
     const VkSurfaceKHR& surface() const { return mSurface; }
     void close(VkInstance& instance);
  };

}// --- namespace fc --- (END)
