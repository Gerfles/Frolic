#pragma once

// - FROLIC ENGINE -
//
// - EXTERNAL LIBRARIES -
#include "vulkan/vulkan_core.h"

// - STD LIBRARIES -
#include <string>
#include <vector>
#include <algorithm>

 // forward declarations
struct SDL_Window;

namespace fc {

  class FcWindow
  {
   private:
      //TODO may want to include swapchain in window but maybe not
      //static void framebufferResizeCallback(SDL_Window* window, int width, int height);
     SDL_Window* mWindow = nullptr;
     VkSurfaceKHR mSurface;
     VkExtent2D mScreenSize;
     bool mFrameBufferResized = false;
      // Helper Functions
   public:
     FcWindow() = default;
     ~FcWindow() = default;
     bool initWindow(uint32_t width = 800, uint32_t height = 600, bool isFullscreen = false
                     , std::string windowName = "Test Window");
      // delete assignment operator and copy constructor
     FcWindow(const FcWindow&) = delete;
     FcWindow& operator=(const FcWindow) = delete;

     const VkExtent2D ScreenSize();
     bool wasWindowResized() {return mFrameBufferResized;}
     void resetWindowResizedFlag() {mFrameBufferResized = false;}
     void createWindowSurface(const VkInstance& instance);
      // ?? not sure what's going on with the const here--don't think it should work for pointer?
     SDL_Window* SDLwindow() const { return mWindow; }
     const VkSurfaceKHR& surface() const { return mSurface; }
     void close(VkInstance& instance);
  };

} // NAMESPACE fc
