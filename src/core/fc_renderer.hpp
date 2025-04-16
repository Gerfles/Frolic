#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_frustum.hpp"
#include "fc_terrain.hpp"
#include "shadow_map.hpp"
#include "fc_skybox.hpp"
#include "fc_billboard_render_system.hpp"
#include "fc_descriptors.hpp"
#include "fc_model_render_system.hpp"
#include "fc_ui_render_system.hpp"
#include "fc_model.hpp"
//#include "fc_font.hpp"
#include "fc_swapChain.hpp"
#include "fc_image.hpp"
#include "fc_gpu.hpp"
#include "fc_window.hpp"
#include "fc_pipeline.hpp"
#include "fc_texture_atlas.hpp"
#include "fc_timer.hpp"
#include "fc_scene.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include <glm/vec3.hpp>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>


namespace fc
{
  struct FrolicStats
  {
     float frametime;
     int fpsAvg;
     int triangleCount;
     int objectsRendered;
     float sceneUpdateTime;
     float meshDrawTime;
  };


  struct FrameData
  {
      // TODO might be better to make this an entire class with all methods and static ints (numFrame)
     VkCommandPool commandPool = VK_NULL_HANDLE;
     VkCommandBuffer commandBuffer;
     VkSemaphore imageAvailableSemaphore;
     VkSemaphore renderFinishedSemaphore;
     VkFence renderFence;
     // TODO allocate all the descriptorSets for each frame (skybox, terrain, etc.)
     VkDescriptorSet sceneDataDescriptorSet;
     VkDescriptorSet shadowMapDescriptorSet;
     VkDescriptorSet skyBoxDescriptorSet;
  };
  //static constexpr int MAX_FRAMES_IN_FLIGHT = 3; // used in swap chain
  // ?? Tried 3 but No idea why drawing with 4 frames is faster than 3??
  constexpr unsigned int MAX_FRAME_DRAWS = 4;


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
     int mFrameNumber {0};
     // TODO extrapolate into separate class
     VkFence mImmediateFence;

     VkCommandPool mImmediateCommandPool;
     VkCommandBuffer mImmediateCmdBuffer;
     /* VkDescriptorSetLayout mSceneDataDescriptorLayout; */
     // TODO think about integrating into descriptorClerk
     VkDescriptorPool mImgGuiDescriptorPool;

     // DELETE lastUsed...
     // FcPipeline* lastUsedPipeline;
     // MaterialInstance* lastUsedMaterial;
     /* VkBuffer lastUsedIndexBuffer; */
     // DELETE
     int test{0};

     std::vector<FrameData> mFrames {MAX_FRAME_DRAWS};
     FcFrustum mFrustum;
     // // TODO should probably pass this in each frame
     // SceneData* pSceneData;

     void createInstance(VkApplicationInfo& appInfo);
     bool areInstanceExtensionsSupported(const std::vector<const char*>& instanceExtensions);
     void createCommandPools();
     // void recordCommands(uint32_t currentFrame);
     void createSynchronization();
     void updateUseFlags(MaterialFeatures feature, bool enable);
     void initDrawImage();
     void initImgui();

     // -*-*-*-*-*-*-*-*-*-   TODO REFACTOR, ENCAPSULATE, OR DELETE   -*-*-*-*-*-*-*-*-*- //
     // DELETE mainDrawContext
     /* DrawCollection mainDrawContext; */
     FcDrawCollection mDrawCollection;
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
     // debugging effects
     FcPipeline mNormalDrawPipeline;
     /* FcPipeline mBoundingBoxPipeline; */
     FcTerrain mTerrain;
     FcTextureAtlas textureAtlas;
     // TODO DELETE
     VkDescriptorSetLayout mBackgroundDescriptorlayout;
     glm::mat4 rotationMatrix{1.0f};

   public:

     // TODO Make these all private
     bool mDrawNormalVectors {false};
     bool mDrawBoundingBoxes {false};
     // Extract to separate modules
     /* float expansionFactor{0}; */
     int rotationSpeed{};
     /* int mBoundingBoxId {-1}; */
     bool drawWireframe {false};
     // bool mDrawNormalVectors {false};
     // bool mDrawBoundingBoxes {false};
     // DELETE eventually -> place models in draw calling function, not render class
//     LoadedGLTF structure;
     FcScene structure;
     FcScene structure2;

     //std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;
     void drawBackground(ComputePushConstants& pushConstans);
     // TODO extract to shadowmap class
     FcShadowMap mShadowMap;
     void drawShadowMap(bool drawDebug);
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   PROFILING   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcTimer mTimer;
     FrolicStats stats;
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
      //FcDescriptor& DescriptorManager() { return mDescriptorManager; }
     uint32_t beginFrame();
     void endFrame(uint32_t swapchainImgIndex);
     void drawBillboards(glm::vec3 cameraPosition, uint32_t swapchainImgIndex, SceneDataUbo& ubo);
     void drawUI(std::vector<FcText>& UIelements, uint32_t swapchainImgIndex);
     void drawGeometry();
      // - GETTERS -
     /* FcSceneRenderer* getMetalRoughMaterial() { return &mSceneRenderer; } */
     /* VkDescriptorSetLayout getSceneDescriptorLayout() { return mSceneDataDescriptorLayout; } */
     /* VkDescriptorSetLayout SkyboxDescriptorLayout() { return mSkybox.DescriptorLayout(); } */
      // TODO delete this probably and place background pipeline in renderer
     VkDescriptorSetLayout getBackgroundDescriptorLayout() { return mBackgroundDescriptorlayout; }
     FrameData& getCurrentFrame() { return mFrames[mFrameNumber % MAX_FRAME_DRAWS]; }
      // ?? is this used often enough to merit a member variable?
     float ScreenWidth() { return mWindow.ScreenSize().width; }
     float ScreenHeight() { return mWindow.ScreenSize().height; }
     SDL_Window* Window() { return mWindow.SDLwindow(); }
     VkRenderPass RenderPass() { return mSwapchain.getRenderPass(); }
     float& ExpansionFactor() { return mSceneRenderer.ExpansionFactor(); };
     const FcGpu& Gpu() const { return mGpu; }
     const FcSwapChain& Swapchain() { return mSwapchain; }
     void shutDown();
  };

} // - End - NAMESPACE fc //
