#pragma once


// - FROLIC ENGINE -
//#include "core/fc_font.hpp"
#include "core/fc_billboard_render_system.hpp"
#include "core/fc_camera.hpp"
#include "core/fc_model_render_system.hpp"
#include "core/fc_ui_render_system.hpp"
#include "fc_model.hpp"
#include "fc_font.hpp"
#include "fc_descriptors.hpp"
#include "fc_swapChain.hpp"
#include "fc_image.hpp"
#include "fc_gpu.hpp"
#include "fc_window.hpp"
#include "fc_pipeline.hpp"
//#include "fc_mesh.hpp"
// - EXTERNAL LIBRARIES -
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <_types/_uint32_t.h>
#include <stdexcept>
#include <vector>



namespace fc
{
   //static constexpr int MAX_FRAMES_IN_FLIGHT = 3; // used in swap chain
  const int MAX_FRAME_DRAWS = 2;

  
  class FcRenderer
  {
   private:

     int mCurrentFrame = 0;
      // TODO determine if buffer count is necessary since we can just call mSwapchain.imageCount();
     int mBufferCount = 0;
     
      //?? try seeing if we can conditionally declare variables when in debug
     VkDebugUtilsMessengerEXT debugMessenger;
     FcWindow mWindow;
     VkInstance mInstance = nullptr;
     FcGpu mGpu;
//     FcDescriptor mDescriptorManager;
     FcSwapChain mSwapchain;
     std::vector<VkSemaphore> mImageReadySemaphores;
     std::vector<VkSemaphore> mRenderFinishedSemaphores;
     
     VkViewport mDynamicViewport;
     VkRect2D mDynamicScissors;

     std::vector<VkCommandBuffer> mCommandBuffers;
     std::vector<VkFence> mDrawFences;

      // TODO find a better way for tighter coupling - maybe friend function
      // Descriptors      // ADDED TO PIPELINE FOR NOW

      // PERFORMANCE tracking

     
      // camera

      // - MODEL RENDERING SYSTEM -
//     FcCamera mCamera;
     FcModelRenderSystem mModelRenderer;
     FcPipeline mModelPipeline;
     
      // - UI RENDERING SYSTEM -
     FcUIrenderSystem mUiRenderer;
     FcPipeline mUiPipeline;
     
     FcBillboardRenderSystem mBillboardRenderer;
     FcPipeline mBillboardPipeline;
     
     // Allocate Functions
     
     // Debugging validation layers
#ifdef NDEBUG
       const bool enableValidationLayers = false;
#else
     const bool enableValidationLayers = true;
#endif
     void createInstance(VkApplicationInfo& appInfo);
     bool areInstanceExtensionsSupported(const std::vector<const char*>& instanceExtensions);
     void createCommandBuffers();
     void recordCommands(uint32_t currentFrame);
     void createSynchronization();
   public:
     // Constructors, etc. - Prevent copying or altering -
     FcRenderer() = default;
     ~FcRenderer() = default;
     FcRenderer& operator=(const FcRenderer&) = delete;
     FcRenderer(const FcRenderer&) = delete;
      //
     int init(VkApplicationInfo& appInfo, VkExtent2D screenSize);
      //
     void handleWindowResize();
      //FcDescriptor& DescriptorManager() { return mDescriptorManager; }
     uint32_t beginFrame();
     void endFrame(uint32_t currentFrame);
     void drawModels(uint32_t swapChainImageIndex, GlobalUbo& ubo);
     void drawBillboards(glm::vec3 cameraPosition, uint32_t swapchainImageIndex, GlobalUbo& ubo);
     void drawUI(std::vector<FcText>& UIelements, uint32_t swapchainImageIndex);
      // - GETTERS -

      // ?? is this used often enough to merit a member variable?
     float AspectRatio() { return (float)mSwapchain.getSurfaceExtent().width / (float)mSwapchain.getSurfaceExtent().height; }
     VkRenderPass& RenderPass() { return mSwapchain.getRenderPass(); }
     uint32_t BufferCount() const { return mBufferCount; }
     const FcGpu& Gpu() const { return mGpu; }
     const FcSwapChain& Swapchain() { return mSwapchain; }
//     FcDescriptor& Descriptors() { return mDescriptorManager; }
     void shutDown();
  };

} // - End - NAMESPACE fc //


// TODO //
// create the instance first and figure out what kind of bufferring we can have
// (double, tripple, etc) then initialize all following objects to have that
// size so we don't need to resize anything

