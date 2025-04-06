#pragma once

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_mesh.hpp"
#include "fc_frustum.hpp"
#include "fc_terrain.hpp"
#include "shadow_map.hpp"
#include "fc_skybox.hpp"
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
#include "fc_texture_atlas.hpp"
#include "fc_timer.hpp"
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
     VkDescriptorSet sceneDataDescriptorSet;
  };
  //static constexpr int MAX_FRAMES_IN_FLIGHT = 3; // used in swap chain
  // ?? No idea why drawing with 4 frames is faster than 3??
  constexpr unsigned int MAX_FRAME_DRAWS = 4;


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
     VkDescriptorSetLayout mSceneDataDescriptorLayout;
     // TODO think about integrating into descriptorClerk
     VkDescriptorPool mImgGuiDescriptorPool;
     FcPipeline* lastUsedPipeline = nullptr;
     MaterialInstance* lastUsedMaterial = nullptr;
     VkBuffer lastUsedIndexBuffer = VK_NULL_HANDLE;
     FrameData mFrames[MAX_FRAME_DRAWS];
     FcFrustum mFrustum;
     // TODO should probably pass this in each frame
     SceneData* pSceneData;
     // -*-*-*-*-*-*-*-*-*-   TODO REFACTOR, ENCAPSULATE, OR DELETE   -*-*-*-*-*-*-*-*-*- //
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
     FcPipeline mBoundingBoxPipeline;
     FcTerrain mTerrain;
     FcTextureAtlas textureAtlas;
     VkDescriptorSetLayout mBackgroundDescriptorlayout;
     void drawBackground(ComputePushConstants& pushConstans);
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     void createInstance(VkApplicationInfo& appInfo);
     bool areInstanceExtensionsSupported(const std::vector<const char*>& instanceExtensions);
     void createCommandPools();
     // void recordCommands(uint32_t currentFrame);
     void createSynchronization();
     void updateUseFlags(MaterialFeatures feature, bool enable);
     void initDrawImage();
     void initImgui();
      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULTS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     //SceneData mSceneData;
     DrawContext mainDrawContext;
     //std::vector<std::shared_ptr<FcMesh>> mTestMeshes;
     FcModel mTestMeshes;
     std::unordered_map<std::string, std::shared_ptr<Node>> loadedNodes;

   public:

     FcShadowMap mShadowMap;
     void drawShadowMap(bool drawDebug);
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   PROFILING   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcTimer mTimer;
     FrolicStats stats;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULTS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcImage mWhiteTexture;
     FcImage mBlackTexture;
     FcImage mGreyTexture;
     FcSkybox mSkybox;
     FcImage mCheckerboardTexture;
     VkSampler mDefaultSamplerLinear;
     VkSampler mDefaultSamplerNearest;
     MaterialInstance defaultMaterialData;
     GLTFMetallicRoughness mMetalRoughMaterial;
     std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;
     LoadedGLTF structure;
     LoadedGLTF structure2;
     glm::mat4 rotationMatrix{1.0f};
     int rotationSpeed{};
     float expansionFactor{0};
     bool mDrawNormalVectors {false};
     bool mDrawBoundingBoxes {false};
     bool drawWireframe {false};
     int mBoundingBoxId {-1};


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

     void initDefaults(FcBuffer& sceneDataBuffer, SceneData* sceneData);
     void attachPipeline(FcPipeline* pipeline);

     void setColorTextureUse(bool enable);
     void setRoughMetalUse(bool enable);
     void setAmbientOcclussionUse(bool enable);
     void setNormalMapUse(bool enable);
     void setEmissiveTextureUse(bool enable);
     void initNormalDrawPipeline(FcBuffer& sceneDataBuffer);
     void initBoundingBoxPipeline(FcBuffer& sceneDataBuffer);
     void drawNormals(VkCommandBuffer cmd, const RenderObject& surface);
     void drawBoundingBox(VkCommandBuffer cmd, const RenderObject& surface);
     void drawSurface(VkCommandBuffer cmd, const RenderObject& surface);

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

     void drawBillboards(glm::vec3 cameraPosition, uint32_t swapchainImgIndex, SceneData& ubo);
     void drawUI(std::vector<FcText>& UIelements, uint32_t swapchainImgIndex);

     void drawGeometry();
      // - GETTERS -
     GLTFMetallicRoughness* getMetalRoughMaterial() { return &mMetalRoughMaterial; }
     VkDescriptorSetLayout getSceneDescriptorLayout() { return mSceneDataDescriptorLayout; }
     VkDescriptorSetLayout SkyboxDescriptorLayout() { return mSkybox.DescriptorLayout(); }
      // TODO delete this probably and place background pipeline in renderer
     VkDescriptorSetLayout getBackgroundDescriptorLayout() { return mBackgroundDescriptorlayout; }
     /* VkDescriptorSetLayout getSingleImageDescriptorLayout() { return mSingleImageDescriptorLayout; } */
     FrameData& getCurrentFrame() { return mFrames[mFrameNumber % MAX_FRAME_DRAWS]; }
      // ?? is this used often enough to merit a member variable?
     float ScreenWidth() { return mWindow.ScreenSize().width; }
     float ScreenHeight() { return mWindow.ScreenSize().height; }
     SDL_Window* Window() { return mWindow.SDLwindow(); }
     float AspectRatio() { return (float)mSwapchain.getSurfaceExtent().width / (float)mSwapchain.getSurfaceExtent().height; }
     VkRenderPass RenderPass() { return mSwapchain.getRenderPass(); }
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
