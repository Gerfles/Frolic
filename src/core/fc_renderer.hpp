#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_billboard_render_system.hpp"
#include "fc_camera.hpp"
#include "fc_descriptors.hpp"
#include "fc_model_render_system.hpp"
#include "fc_materials.hpp"
#include "fc_ui_render_system.hpp"
#include "fc_model.hpp"
#include "fc_font.hpp"
#include "fc_swapChain.hpp"
#include "fc_image.hpp"
#include "fc_gpu.hpp"
#include "fc_window.hpp"
#include "fc_pipeline.hpp"
#include "fc_janitor.hpp"
#include "fc_texture_atlas.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include <glm/vec3.hpp>
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
     VkDescriptorSet sceneDataDescriptorSet;
     fcJanitor janitor;
  };


  //static constexpr int MAX_FRAMES_IN_FLIGHT = 3; // used in swap chain
    constexpr unsigned int MAX_FRAME_DRAWS = 4;


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
     FcBuffer materialConstants;
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

      // Right now we only have one draw image and depth image, but on a more developed engine it could be
      // significantly more, and re-creating all that can be a considerable hassle. Instead, we create the
      // draw and depth image at startup with a preset size, and then draw into a section of it if the
      // window is small, or scale it up if the window is bigger. As we arent reallocating but just
      // rendering into a corner, we can also use this same logic to perform dynamic resolution, which is
      // a useful way of scaling performance, and can be handy for debugging.

     // ?? TODO should this be handled by the swapchain even though its used by compute
     FcImage mDrawImage;
      // TODO try and delete the following

     VkDescriptorSet mDrawImageDescriptor;
     FcImage mDepthImage; // <--Normally in the swapchain
      //Fc[...]renderSystem m[...]Renderer;
     bool mShouldWindowResize{false};
     int mFrameNumber {0};
      // ?? DELETE
     void initDrawImage();
     void initImgui();
     VkFence mImmediateFence;
     VkCommandPool mImmediateCommandPool;
     VkCommandBuffer mImmediateCmdBuffer;
     VkDevice pDevice;
      // TODO think about integrating into descriptorClerk
     VkDescriptorPool mImgGuiDescriptorPool;
      //std::vector<ComputeEffect> backgroundEffects;
     VkPipeline pDrawPipeline;
     VkPipelineLayout pDrawPipelineLayout;
     VkExtent2D mDrawExtent;
     float renderScale = 1.f;
     FcTextureAtlas textureAtlas;
     FcModel testModel;

     VkDescriptorSetLayout mSceneDataDescriptorLayout;
     VkDescriptorSetLayout mSingleImageDescriptorLayout;
     VkDescriptorSetLayout mBackgroundDescriptorlayout;

     FcBuffer mSceneDataBuffer;
     SceneData* pSceneData;
      // TODO try this single one instead of one in each frame
     VkDescriptorSet mSceneDataDescriptor;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULTS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcImage mWhiteTexture;
     FcImage mBlackTexture;
     FcImage mGreyTexture;
     FcImage mCheckerboardTexture;
     VkSampler mDefaultSamplerLinear;
     VkSampler mDefaultSamplerNearest;
     MaterialInstance defaultMaterialData;
     GLTFMetallicRoughness mMetalRoughMaterial;

   public:
      //void setResizeFlag(bool shouldWindowResizeFlag) { mWindowResizeFlag = shouldWindowResizeFlag; }
      // TODO probably best to issue multiple command buffers, one for each task
     bool shouldWindowResize() { return mShouldWindowResize; }
     VkCommandBuffer beginCommandBuffer();
     void submitCommandBuffer();
     void drawImGui(VkCommandBuffer cmd, VkImageView targetImageView);
      // TODO implement differently
     // FcPipeline mGradientPipeline;
     // FcPipeline mSkyPipeline;
     void initDefaults();
     float* getRenderScale() { return &renderScale; }
     void attachPipeline(FcPipeline* pipeline);
     void attachSceneData(SceneData* pSceneData) { this->pSceneData = pSceneData; }
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
     void drawSimple(ComputePushConstants& pushConstans);
     void drawGeometry(FcPipeline& pipeline);
      // - GETTERS -
     VkDescriptorSetLayout getSceneDescriptorLayout() { return mSceneDataDescriptorLayout; }
      // TODO delete this probably and place background pipeline in renderer
     VkDescriptorSetLayout getBackgroundDescriptorLayout() { return mBackgroundDescriptorlayout; }
     VkDescriptorSetLayout getSingleImageDescriptorLayout() { return mSingleImageDescriptorLayout; }
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
