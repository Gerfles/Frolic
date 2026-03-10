//>--- fc_window.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
struct SDL_Window;
#include <bits/stringfwd.h>
namespace fc { class FcConfig; }
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

   public:
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcWindow() = default;
     ~FcWindow() = default;
     FcWindow(const FcWindow&) = delete;
     FcWindow& operator=(const FcWindow) = delete;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     bool initWindow(FcConfig& config);
     const VkExtent2D ScreenSize();
     void createWindowSurface(const VkInstance& instance);
     SDL_Window* SDLwindow() const { return mWindow; }
     inline const VkSurfaceKHR& surface() const { return mSurface; }
     void close(VkInstance& instance);
  };

}// --- namespace fc --- (END)
