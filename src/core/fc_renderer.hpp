#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_billboard_render_system.hpp"
#include "core/fc_camera.hpp"
#include "core/fc_model_render_system.hpp"
#include "core/fc_ui_render_system.hpp"
#include "fc_model.hpp"
#include "fc_font.hpp"
#include "fc_swapChain.hpp"
#include "fc_image.hpp"
#include "fc_gpu.hpp"
#include "fc_window.hpp"
#include "fc_pipeline.hpp"
#include "fc_janitor.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <stdexcept>
#include <vector>



namespace fc
{

  struct FrameData
  {
      // TODO might be better to make this an entire class with all methods and static ints (numFrame)
     VkCommandPool commandPool = VK_NULL_HANDLE;
     VkCommandBuffer commandBuffer;
     VkSemaphore imageAvailableSemaphore;
     VkSemaphore renderFinishedSemaphore;
     VkFence renderFence;
     fcJanitor janitor;
  };

//static constexpr int MAX_FRAMES_IN_FLIGHT = 3; // used in swap chain
  constexpr unsigned int MAX_FRAME_DRAWS = 2;


  class FcRenderer
  {
   private:

      // TODO determine if buffer count is necessary since we can just call mSwapchain.imageCount();
     int mBufferCount {0};
     FrameData mFrames[MAX_FRAME_DRAWS];
      //?? try seeing if we can conditionally declare variables when in debug
     VkDebugUtilsMessengerEXT debugMessenger;
     FcWindow mWindow;
     VkInstance mInstance = nullptr;
     FcGpu mGpu;
//     FcDescriptor mDescriptorManager;
     FcSwapChain mSwapchain;
     VkViewport mDynamicViewport;
     VkRect2D mDynamicScissors;
      // TODO delete (now in FrameData)
     // std::vector<VkSemaphore> mImageReadySemaphores;
     // std::vector<VkSemaphore> mRenderFinishedSemaphores;
      // std::vector<VkCommandBuffer> mCommandBuffers;
      //      std::vector<VkFence> mDrawFences;

      // - Model Rendering
     FcModelRenderSystem mModelRenderer;
     FcPipeline mModelPipeline;
      // - UI Rendering
     FcUIrenderSystem mUiRenderer;
     FcPipeline mUiPipeline;
      // - Billboard Rendering
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
     void createCommandPools();
//     void recordCommands(uint32_t currentFrame);
     void createSynchronization();

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // ?? TODO should this be handled by the swapchain
     FcImage mDrawImage;
      //Fc[...]renderSystem m[...]Renderer;
     FcPipeline mBackgroundPipeline;
     int mFrameNumber {0};
      // ?? DELETE
     void initDrawImage();
     void initImgui();
     VkFence mImmediateFence;
     VkCommandPool mImmediateCommandPool;
     VkCommandBuffer mImmediateCmdBuffer;
     VkDevice pDevice;
     VkDescriptorPool mImgGuiDescriptorPool;
   public:
      // TODO probably best to issue multiple command buffers, one for each task
     VkCommandBuffer beginCommandBuffer();
     void submitCommandBuffer();

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

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
     void endFrame(uint32_t swapchainImgIndex);

     void drawModels(uint32_t swapchainImgIndex, GlobalUbo& ubo);
     void drawBillboards(glm::vec3 cameraPosition, uint32_t swapchainImgIndex, GlobalUbo& ubo);
     void drawUI(std::vector<FcText>& UIelements, uint32_t swapchainImgIndex);
     void drawSimple(uint32_t swapchainImgIndex);
      // - GETTERS -
      FrameData& getCurrentFrame() { return mFrames[mFrameNumber % MAX_FRAME_DRAWS]; }
      // ?? is this used often enough to merit a member variable?
     float AspectRatio() { return (float)mSwapchain.getSurfaceExtent().width / (float)mSwapchain.getSurfaceExtent().height; }
     VkRenderPass RenderPass() { return mSwapchain.getRenderPass(); }
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
