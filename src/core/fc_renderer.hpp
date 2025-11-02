#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_draw_collection.hpp"
#include "fc_frustum.hpp"
#include "platform.hpp"
#include "fc_terrain.hpp"
#include "fc_camera.hpp"
#include "shadow_map.hpp"
#include "fc_skybox.hpp"
#include "fc_billboard_renderer.hpp"
#include "fc_model_render_system.hpp"
#include "fc_ui_render_system.hpp"
#include "fc_frame_assets.hpp"
#include "fc_bounding_box.hpp"
#include "fc_normal_renderer.hpp"
#include "fc_swapChain.hpp"
#include "fc_image.hpp"
#include "fc_gpu.hpp"
#include "fc_window.hpp"
#include "fc_pipeline.hpp"
#include "fc_timer.hpp"
#include "fc_scene.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include <glm/vec3.hpp>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <vector>


namespace fc
{

  static constexpr unsigned int MAX_FRAME_DRAWS = 3;
  static constexpr unsigned int BINDLESS_DESCRIPTOR_SLOT = 10;

  // TODO //
  // create the instance first and figure out what kind of bufferring we can have (double,
  // tripple, etc) then initialize all following objects to have that size so we don't need
  // to resize anything
    class FcRenderer
    {
     private:

#ifdef NDEBUG
       const bool enableValidationLayers = false;
#else
       const bool enableValidationLayers = true;
#endif
       VkDebugUtilsMessengerEXT debugMessenger;
       FcWindow mWindow;
       VkInstance mInstance = nullptr;
       FcGpu mGpu;
       FcSwapChain mSwapchain;
       VkViewport mDynamicViewport{};
       VkRect2D mDynamicScissors{};
       VkDevice pDevice;
       VkExtent2D mDrawExtent;
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
       bool mShouldWindowResize{false};
       u32 mFrameNumber {0};
       // TODO extrapolate into separate class
       VkFence mImmediateFence;

       VkCommandPool mImmediateCommandPool;
       VkCommandBuffer mImmediateCmdBuffer;

       // TODO think about integrating into descriptorClerk
       VkDescriptorPool mImgGuiDescriptorPool;
       std::vector<FrameAssets> mFrames {MAX_FRAME_DRAWS};
       FcFrustum mFrustum;

       // TODO create agile version of data structures e.g. FcArray
       FcAllocator* pAllocator;

       // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FUNCTIONS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
       void createInstance(VkApplicationInfo& appInfo);
       bool areInstanceExtensionsSupported(const std::vector<const char*>& instanceExtensions);
       void createCommandPools();
       // void recordCommands(uint32_t currentFrame);
       void createSynchronization();
       void updateUseFlags(MaterialFeatures feature, bool enable);
       void initDrawImage();
       void initImgui();

       // -*-*-*-*-*-*-*-*-*-   TODO REFACTOR, ENCAPSULATE, OR DELETE   -*-*-*-*-*-*-*-*-*- //

       // TODO may want to add to sceneRenderer but need for shadow map
       // although shadow map may also be preferred to be added to scene renderer
       FcDrawCollection mDrawCollection;
       // - Model Rendering
       FcModelRenderSystem mModelRenderer;
       FcPipeline mModelPipeline;
       // - UI Rendering
       FcUIrenderSystem mUiRenderer;
       FcPipeline mUiPipeline;
       // - Billboard Rendering
       FcBillboardRenderer mBillboardRenderer;
       FcBuffer materialConstants;
       FcTerrain mTerrain;
       glm::mat4 rotationMatrix{1.0f};
       // debugging effects
       FcBoundingBoxRenderer mBoundingBoxRenderer;
       FcNormalRenderer mNormalRenderer;
       FcCamera* pActiveCamera;
       SceneDataUbo mSceneData;
       FcBuffer mSceneDataBuffer;
       VkDescriptorSetLayout mSceneDataDescriptorLayout;

       // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   TEMP   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     public:

       // TODO Make these all private
       bool mDrawNormalVectors {false};
       bool mDrawBoundingBoxes {false};
       bool shouldDrawWireframe {false};
       int mBoundingBoxId {-1};


       void drawBackground(ComputePushConstants& pushConstans);
       FcShadowMap mShadowMap;
       /* void drawShadowMap(bool drawDebug); */
       // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   PROFILING   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
       FcTimer mTimer;
       /* FcStats stats; */
       FcSkybox mSkybox;
       // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULTS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
       // relocate to atlas
       // FcImage mWhiteTexture;
       // FcImage mBlackTexture;
       // FcImage mGreyTexture;
       // FcImage mCheckerboardTexture;
       // VkSampler mDefaultSamplerLinear;
       // VkSampler mDefaultSamplerNearest;
       //MaterialInstance defaultMaterialData;
       // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

       FcSceneRenderer mSceneRenderer;
       void updateScene();
       float aspectRatio() { return static_cast<float>(mWindow.ScreenSize().width)
           / static_cast<float>(mWindow.ScreenSize().height); }
       //void setResizeFlag(bool shouldWindowResizeFlag) { mWindowResizeFlag = shouldWindowResizeFlag; }
       // TODO probably best to issue multiple command buffers, one for each task
       bool shouldWindowResize() { return mShouldWindowResize; }
       VkCommandBuffer beginCommandBuffer();
       void submitCommandBuffer();
       void drawImGui(VkCommandBuffer cmd, VkImageView targetImageView);
       // TODO implement differently
       // FcPipeline mGradientPipeline;
       // FcPipeline mSkyPipeline;

       void initDefaults();//FcBuffer& sceneDataBuffer, SceneDataUbo* sceneData);
       void setColorTextureUse(bool enable);
       void setRoughMetalUse(bool enable);
       void setAmbientOcclussionUse(bool enable);
       void setNormalMapUse(bool enable);
       void setEmissiveTextureUse(bool enable);

       /* void initNormalDrawPipeline(FcBuffer& sceneDataBuffer); */
       /* void initBoundingBoxPipeline(FcBuffer& sceneDataBuffer); */
       /* void drawNormals(VkCommandBuffer cmd, const FcRenderObject& surface); */
       /* void drawBoundingBox(VkCommandBuffer cmd, const FcRenderObject& surface); */
       // DELETE
       /* void drawSurface(VkCommandBuffer cmd, const FcRenderObject& surface); */

       // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

       // TODO add move ctors Constructors, etc. - Prevent copying or altering -
       FcRenderer() = default;
       ~FcRenderer() = default;
       FcRenderer& operator=(const FcRenderer&) = delete;
       FcRenderer(const FcRenderer&) = delete;
       //
       int init(VkApplicationInfo& appInfo, VkExtent2D screenSize, SceneDataUbo** pSceneData);
       //
       void handleWindowResize();
       uint32_t beginFrame();
       void endFrame(uint32_t swapchainImgIndex);
       void drawUI(std::vector<FcText>& UIelements, uint32_t swapchainImgIndex);
       void drawFrame(bool drawDebugShadowMap);
       inline void setActiveCamera(FcCamera* camera) { pActiveCamera = camera; }
       // TODO add bilboards to draw collection
       inline void addBillboard(FcBillboard& billboard) { mBillboardRenderer.addBillboard(billboard); }

       // - GETTERS -
       /* FcSceneRenderer* getMetalRoughMaterial() { return &mSceneRenderer; } */
       // TODO delete this probably and place background pipeline in renderer
       inline FrameAssets& getCurrentFrame() { return mFrames[mFrameNumber % MAX_FRAME_DRAWS]; }
       inline FcDrawCollection& DrawCollection() { return mDrawCollection; }
       // ?? is this used often enough to merit a member variable?
       inline float ScreenWidth() { return mWindow.ScreenSize().width; }
       inline float ScreenHeight() { return mWindow.ScreenSize().height; }
       inline SDL_Window* Window() { return mWindow.SDLwindow(); }
       inline VkRenderPass RenderPass() { return mSwapchain.getRenderPass(); }
       inline int BoundingBox() { return mBoundingBoxId; }
       inline float& ExpansionFactor() { return mSceneRenderer.ExpansionFactor(); };
       inline const FcGpu& Gpu() const { return mGpu; }
       inline const FcSwapChain& Swapchain() { return mSwapchain; }
       inline FcStats& getStats() { return mDrawCollection.stats; }
       void shutDown();

    };

} // - End - NAMESPACE fc //
