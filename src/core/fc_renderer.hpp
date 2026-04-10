//> fc_renderer.hpp <//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_janitor.hpp"
#include "fc_frustum.hpp"
#include "fc_scene_renderer.hpp"
#include "fc_terrain.hpp"
#include "shadow_map.hpp"
#include "fc_skybox.hpp"
#include "fc_billboard_renderer.hpp"
#include "fc_normal_renderer.hpp"
#include "fc_gpu.hpp"
#include "fc_timer.hpp"
#include "fc_frame_assets.hpp"
#include "fc_types.hpp"
#include "fc_commands.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcCamera; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  // The chain of function that the renderer will call when asked to draw
  class FcDrawChain
  {

  };

  // TODO create the instance first and figure out what kind of bufferring we can have (double,
  // tripple, etc) then initialize all following objects to have that size so we don't need
  // to resize anything
  class FcRenderer
  {
   private:

     // *-*-*-*-*-*-*-*-*-*-*-*-*-   RENDERING SUBSYSTEMS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcBillboardRenderer mBillboardRenderer;
     FcBoundingBoxRenderer mBoundingBoxRenderer;
     FcNormalRenderer mNormalRenderer;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkDebugUtilsMessengerEXT debugMessenger;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   TODO   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // maybe move to frolic class
     FcGpu mGpu;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


     FcSwapChain mSwapchain;
     VkViewport mDynamicViewport{};
     VkRect2D mDynamicScissors{};
     VkDevice pDevice;
     /* VkExtent2D mDrawExtent; */
     // Right now we only have one draw image and depth image, but on a more developed
     // engine it could be significantly more, and re-creating all that can be a
     // considerable hassle. Instead, we create the draw and depth image at startup with
     // a preset size, and then draw into a section of it if the window is small, or
     // scale it up if the window is bigger. As we arent reallocating but just rendering
     // into a corner, we can also use this same logic to perform dynamic resolution,
     // which is a useful way of scaling performance, and can be handy for debugging.
     FcImage mDrawImage;
     FcImage mDepthImage; // <--Normally in the swapchain
     // Only needed if we are using a compute shader to draw to the draw image
     VkDescriptorSet mDrawImageDescriptor;
     //Fc[...]renderSystem m[...]Renderer;

     // TODO think about integrating into descriptorClerk
     VkDescriptorPool mImgGuiDescriptorPool;
     FcDescriptorCollection mDescriptorCollection;

     VkDescriptorSet sceneBindlessTextureSet;
     FcFrustum mFrustum;

     // TODO create agile version of data structures e.g. FcArray
     FcAllocator* pAllocator;

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   HELPERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void createInstance(VkApplicationInfo& appInfo, FcConfig& configOptions);
     /* bool areInstanceExtensionsSupported(const std::vector<const char*>& instanceExtensions); */
     /* bool areValidationLayersSupported(std::vector<const char*>& validationLayers); */

     void initImgui(VkFormat swapchainFormat, FcConfig& config);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DELETE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     /* VkDescriptorSetLayout mSceneDataDescriptorLayout; */
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     // TODO may want to add to sceneRenderer but need for shadow map
     // although shadow map may also be preferred to be added to scene renderer
     FcDrawCollection mDrawCollection;

     FcBuffer materialConstants;
     FcTerrainRenderer mTerrainRenderer;
     // // debugging effects

     FcCamera* pActiveCamera;
     SceneData mSceneData;


     // *-*-*-*-*-*-*-*-*-   CACHED TO PREVENT CREATION EACH FRAME   *-*-*-*-*-*-*-*-*- //
     VkRenderingAttachmentInfo mColorAttachment {};
     VkRenderingInfo mRenderInfo {};
     VkRenderingAttachmentInfo mDepthAttachment {};

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   TEMP??   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // TODO use new operator for janitor and other fcLocator entities
     FcJanitor mJanitor;

     // TODO change MAX_FRAME_DRAWS to MAX_SWAPCHAIN_BUFFERS

     // TODO rename semaphores to this
     // (vkCreateSemaphore(pDevice, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore));
     // (vkCreateSemaphore(pDevice, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore));
   public:
     FcCommandBuffer* mCurrentCommandBuffer;
     VkSemaphore mTimelineSemaphore {VK_NULL_HANDLE};
     VulkanImmediateCommands mImmediateCommands;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END TEMP   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     // TODO try and eliminate
     friend class FcCommandBuffer;
     // TODO Make these all private
     bool mDrawNormalVectors {false};
     bool mDrawBoundingBoxes {false};
     bool shouldDrawWireframe {false};
     int mBoundingBoxId {-1};

     FcShadowRenderer mShadowRenderer;
     /* void drawShadowMap(bool drawDebug); */
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   PROFILING   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcTimer mTimer;
     /* FcStats stats; */
     FcSkybox mSkybox;
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkImageMemoryBarrier2 mDrawImgColorAttachmentBarrier {};
     VkImageMemoryBarrier2 mDepthImgAttachmentOptimalBarrier {};
     VkImageMemoryBarrier2 mDrawImgWriteAccessBarrier {};

     FcSceneRenderer mSceneRenderer;
     void updateScene();

     //void setResizeFlag(bool shouldWindowResizeFlag) { mWindowResizeFlag = shouldWindowResizeFlag; }

     // TODO probably best to issue multiple command buffers, one for each task
     FcCommandBuffer& beginCommandBuffer();
     void submitCmdBuffer(FcCommandBuffer& cmdBuffer);
     void submitNonRenderCmdBuffer(FcCommandBuffer& cmdBuffer);
     SubmitHandle getCurrentCommandBuffer() { return mImmediateCommands.getNextSubmitHandle(); }
     void drawImGui();
     void initDefaults();//FcBuffer& sceneDataBuffer, SceneDataUbo* sceneData);
     void setColorTextureUse(bool enable);
     void setRoughMetalUse(bool enable);
     void setAmbientOcclussionUse(bool enable);
     void setNormalMapUse(bool enable);
     void setEmissiveTextureUse(bool enable);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     // TODO add move ctors Constructors, etc. - Prevent copying or altering -
     FcRenderer() = default;
     ~FcRenderer() = default;
     FcRenderer& operator=(const FcRenderer&) = delete;
     FcRenderer(const FcRenderer&) = delete;
     //
     int init(FcConfig& config, SceneData** pSceneData);
     //
     void beginFrame();
     void drawFrame();
     void endFrame();
     //
     void updateBindlessDescriptors();
     //
     inline void setActiveCamera(FcCamera* camera) { pActiveCamera = camera; }
     inline void addBillboard(FcBillboard& billboard) { mBillboardRenderer.addBillboard(billboard); }

     // - GETTERS -
     /* inline FrameAssets& getCurrentFrame() { return mFrames[mFrameNumber % MAX_FRAME_DRAWS]; } */

     inline FcDrawCollection& DrawCollection() { return mDrawCollection; }
     // float aspectRatio() { return static_cast<float>(mWindow.ScreenSize().width)
     //     / static_cast<float>(mWindow.ScreenSize().height); }
     // inline float ScreenWidth() { return mWindow.ScreenSize().width; }
     // inline float ScreenHeight() { return mWindow.ScreenSize().height; }
     // inline SDL_Window* Window() { return mWindow.SDLwindow(); }
     inline VkRenderPass RenderPass() { return mSwapchain.getRenderPass(); }
     inline int BoundingBox() { return mBoundingBoxId; }
     inline float& ExpansionFactor() { return mSceneRenderer.ExpansionFactor(); };
     inline const FcGpu& Gpu() const { return mGpu; }
     inline const FcSwapChain& Swapchain() { return mSwapchain; }
     inline FcStats& getStats() { return mDrawCollection.stats; }
     void shutDown();

  }; // ---     class FcRenderer --- (END)

} // - End - NAMESPACE fc //
