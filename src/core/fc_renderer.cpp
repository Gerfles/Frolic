//>--- fc_renderer.cpp ---<//
#include "fc_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_locator.hpp"
#include "fc_assert.hpp"
#include "fc_descriptors.hpp"
#include "fc_defaults.hpp"
#include "fc_camera.hpp"
#include "fc_memory.hpp"
#include "fc_cvar_system.hpp"
#include "utilities.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <SDL_events.h>
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_log.h>
#include <SDL_version.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


// TODO (note may no longer be relevant) All of the helper functions that submit commands
// so far have been set up to execute synchronously by waiting for the queue to become
// idle. For practical applications it is recommended to combine these operations in a
// single command buffer and execute them asynchronously for higher throughput, especially
// the transitions and copy in the createTextureImage function. Try to experiment with
// this by creating a setupCommandBuffer that the helper functions record commands into,
// and add a flushSetupCommands to execute the commands that have been recorded so
// far. It's best to do this after the texture mapping works to check if the texture
// resources are still set up correctly.
namespace fc
{
  // FcRenderer::FcRenderer()
  // {
  //    // TODO allow a passed struct that will initialize the window and app "stuff"
  //    // including a pointer to a VK_STRUCTURE type appinfo
  // }

  int FcRenderer::init(FcConfig& config, SceneDataUbo** pSceneData)
  {
    // TODO make this (try) more robust
    try
    {
      mWindow.initWindow(config);

      // Application Specs for developer use
      VkApplicationInfo appInfo{};
      appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      appInfo.pApplicationName   = config.applicationName.c_str();
      appInfo.applicationVersion = VK_MAKE_VERSION(config.appVersionMajor,
                                                   config.appVersionMinor,
                                                   config.appVersionPatch);
      appInfo.pEngineName        = "Frolic Engine";
      appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
      // Make sure we let vulkan know which version we intend to use
      appInfo.apiVersion         = VK_API_VERSION_1_3;

      // First we need a vulkan instance to do anything else
      createInstance(appInfo, config);

      SDL_version compiled;
      SDL_VERSION(&compiled);

      SDL_version linked;
      SDL_GetVersion(&linked);
      // Pring version info
      fcPrintEndl("SDL version(s): %u.%u.%u (compiled),  %u.%u.%u. (linked)\n",
                  compiled.major, compiled.minor, compiled.patch, linked.major, linked.minor, linked.patch);

      // create the surface (interface between Vulkan and window (SDL))
      mWindow.createWindowSurface(mInstance);

      // retrieve the physical device then create the logical device to interface with GPU & command pool
      if ( !mGpu.init(mInstance, config))
      {
        throw std::runtime_error("Failed to initialize GPU device!");
      }

      FcLocator::provide(&mGpu);
      FcLocator::provide(this);

      pDevice = FcLocator::Gpu().getVkDevice();

      // create the swapchain & renderpass & frambuffers & depth buffer
      VkFormat swapchainImageFormat = mSwapchain.init(mGpu, config);

      // -*-*-*-*-*-*-*-*-*-*-*-*-   INITIALIZE DESCRIPTORS   -*-*-*-*-*-*-*-*-*-*-*-*- //
      FcDescriptorClerk* descClerk = new FcDescriptorClerk;

      // register the descriptor with the locator
      FcLocator::provide(descClerk);

      // TODO document the pool ratios better
      std::vector<PoolSizeRatio> poolRatios = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8}
      };

      //
      descClerk->initDescriptorPools(1024, poolRatios);

      initDrawImage();

      // create the command pool for later allocating command from. Also create the command buffers
      createCommandPools();

      // create the graphics pipeline && create/attach descriptors create the uniform
      // buffers & initialize the descriptor sets that tell the pipeline about our uniform
      // buffers here we want to create a descriptor set for each swapchain image we have
      // TODO rename from create to init maybe
      // TODO determine is descriptorclerk should be a local variable or heap variable as is

      initImgui(swapchainImageFormat);

      // create the command buffers& command Pool
      //mPipeline.createUniformBuffers(&mGpu, mSwapchain, sizeof(UboViewProjection));
      // setup draw
      //mPipeline.createDescriptorPool();
      //mPipeline.createDescriptorSets();

      // synchronize the commandbuffers and swapchain
      createSynchronization();
      // VulkanImmediateCommands sync {mGpu.getVkDevice()
      //                             , mGpu.getQueues().graphicsFamily
      //                             , "SYNC"
      // };



      // NEW
      mImmediateCommands.init(pDevice, mGpu.getQueues().graphicsFamily, "SYNC");

      //
      FcLocator::provide(&mJanitor);


      // initialze the dynamic mDynamicViewport and mDynamicScisors
      mDynamicViewport.x = 0;
      mDynamicViewport.y = 0;
      mDynamicViewport.width = mDrawExtent.width;
      mDynamicViewport.height = mDrawExtent.height;
      mDynamicViewport.minDepth = 0.0f;
      mDynamicViewport.maxDepth = 1.0f;

      mDynamicScissors.offset = {0, 0};
      mDynamicScissors.extent.width = mDrawExtent.width;
      mDynamicScissors.extent.height = mDrawExtent.height;

      // *-*-*-*-*-*-*-*-*-*-*-*-   FRAME DATA INITIALIZATION   *-*-*-*-*-*-*-*-*-*-*-*- //
      mSceneDataBuffer.allocate(sizeof(SceneDataUbo), FcBufferTypes::Uniform);
      // TODO create temporary storage for this in descClerk so we can just write the
      // descriptorSet and layout on the fly and destroy layout if not needed
      // TODO see if layout is not needed.
      FcDescriptorBindInfo sceneDescriptorBinding{};
      sceneDescriptorBinding.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                        , VK_SHADER_STAGE_VERTEX_BIT
                                        // TODO DELETE after separating model from scene
                                        | VK_SHADER_STAGE_FRAGMENT_BIT
                                        | VK_SHADER_STAGE_GEOMETRY_BIT);

      // TODO find out if there is any cost associated with binding to multiple un-needed stages...
      //, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

      sceneDescriptorBinding.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                                          , mSceneDataBuffer, sizeof(SceneDataUbo), 0);

      // TODO find out if we need to keep descriptorLayout
      // create descriptorSet for sceneData
      mSceneDataDescriptorLayout = descClerk->createDescriptorSetLayout(sceneDescriptorBinding);

      // Allocate a descriptorSet to each frame buffer
      for (FrameAssets& frame : mFrames)
      {
        frame.sceneDataDescriptorSet =
          descClerk->createDescriptorSet(mSceneDataDescriptorLayout,
                                         sceneDescriptorBinding);
      }

      FcDefaults::init(pDevice);

      // *-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE RESOURCE POOLS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
      // TODO get rid of in favor of FcLocator pattern
      pAllocator = &MemoryService::instance()->systemAllocator;
      mDrawCollection.init(pAllocator);

    }
    catch (const std::runtime_error& err) {
      printf("ERROR: %s\n", err.what());
      return EXIT_FAILURE;
    }


    *pSceneData = &mSceneData;
    return EXIT_SUCCESS;
  }

  // TODO could pass pScene data here but probably better to just include this in mRenderer.init()
  void FcRenderer::initDefaults()//FcBuffer& sceneDataBuffer, SceneDataUbo* sceneData)
  {
    mShadowMap.init(mFrames);

    mTerrain.init("..//maps/terrain_heightmap_r16.ktx2");

    // FIXME should be able to load any kind of terrain map
    /* mTerrain.init("..//maps/iceland_heightmap.png"); */

    // TODO Organize initialization

    // set the uniform buffer for the material data
    materialConstants.allocate(sizeof(MaterialConstants), FcBufferTypes::Uniform);

    // write the buffer
    MaterialConstants* materialUniformData =
      (MaterialConstants*)materialConstants.getAddress();
    materialUniformData->colorFactors = glm::vec4{1,1,1,1};
    materialUniformData->metalRoughFactors = glm::vec4{1, 0.5, 0, 0};

    // TODO these should all be a part of frolic or game class, not the renderer
    mSkybox.loadTextures("..//models//skybox", ".jpg");
    // TODO should be more descriptive in name to show this has to happen after loadTextures
    mSkybox.init(mSceneDataDescriptorLayout, mFrames);
    //

    mSceneData.projection = pActiveCamera->Projection();

    // TODO get rid of scenedescriptorLayout (I believe)
    mSceneRenderer.init(mSceneDataDescriptorLayout, mSceneData.viewProj, mFrames);
    // TODO get rid of build pipeline calls and just do that within init
    // TODO see if we can decouple the scene descriptor layout
    mBoundingBoxRenderer.buildPipelines(mSceneDataDescriptorLayout);
    mNormalRenderer.buildPipelines(mSceneDataDescriptorLayout);
    mBillboardRenderer.buildPipelines(mFrames);
    // // TODO think about destroying layout here

    mSceneData.sunlightDirection = glm::vec4(43.1f, 25.f, 23.f, 1.f);
    glm::vec3 target{mSceneData.sunlightDirection.x, 0.0, mSceneData.sunlightDirection.z};

    mShadowMap.updateLightSource(mSceneData.sunlightDirection, target);
  }


  //
  void FcRenderer::initImgui(VkFormat swapchainFormat)
  {
    // Create the descriptor pool for IMGUI
    // probably oversized, doesn't seem to be in any imgui demo that was mentioned
    // TODO /TRY try and follow this up by recreating the example from ImgGUI site
    static const uint32_t maxPoolSets = 128;
    VkDescriptorPoolSize poolSizes[] =
      {
        {VK_DESCRIPTOR_TYPE_SAMPLER, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, maxPoolSets},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, maxPoolSets}
      };


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;

    // Finally create the descriptor pool
    VK_ASSERT(vkCreateDescriptorPool(pDevice, &poolInfo, nullptr, &mImgGuiDescriptorPool));

    // *-*-*-*-*-*-*-*-*-*-*-*-   INITIALIZE IMGUI LIBRARY   *-*-*-*-*-*-*-*-*-*-*-*- //
    // initialize core structures
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Conrols

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // initialize Platform / Renderer backends
    if (!ImGui_ImplSDL2_InitForVulkan(mWindow.SDLwindow()))
    {
      throw std::runtime_error("Failed to initialize ImGui within SDL2");
    }

    // initialize for Vulkan
    VkPipelineRenderingCreateInfoKHR pipelineRenderingInfo{};
    pipelineRenderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingInfo.colorAttachmentCount = 1;
    /* pipelineRenderingInfo.pColorAttachmentFormats = &mSwapchain.getFormat(); */
    pipelineRenderingInfo.pColorAttachmentFormats = &swapchainFormat;

    ImGui_ImplVulkan_InitInfo imGuiInfo{};
    imGuiInfo.Instance = mInstance;
    imGuiInfo.PhysicalDevice = mGpu.physicalDevice();
    imGuiInfo.Device = mGpu.getVkDevice();
    imGuiInfo.Queue = mGpu.graphicsQueue();
    imGuiInfo.DescriptorPool = mImgGuiDescriptorPool;
    // ?? Don't we have 2 and not 3 (so far)?
    imGuiInfo.MinImageCount = 3;
    imGuiInfo.ImageCount = 3;
    imGuiInfo.UseDynamicRendering = true;
    imGuiInfo.PipelineRenderingCreateInfo = pipelineRenderingInfo;
    // TODO change to discovered
    imGuiInfo.MSAASamples = FcLocator::Gpu().Properties().maxMsaaSamples;
    //imGuiInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&imGuiInfo))
    {
      throw std::runtime_error("Failed to initialize ImGui within Vulkan");
    }

    if (!ImGui_ImplVulkan_CreateFontsTexture())
    {
      throw std::runtime_error("Failed to create ImGui Fonts");
    }
  }


  //
  void FcRenderer::createInstance(VkApplicationInfo& appInfo, FcConfig& config)
  {
    // First determine all the extensions needed for SDL to run Vulkan instance
    // uint32_t sdlExtensionCount = 0;
    // SDL_Vulkan_GetInstanceExtensions(mWindow.SDLwindow(), &sdlExtensionCount, nullptr);
    // std::vector<const char*> extensions(sdlExtensionCount);
    // SDL_Vulkan_GetInstanceExtensions(mWindow.SDLwindow(), &sdlExtensionCount, extensions.data());

    // Include debug utilities only when required
#ifndef NDEBUG
    config.addInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    // Not working for now
    // if (enableValidationLayers)
    // {
    //   config.addInstanceExtension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    // }

    // Define a Create struct to initialize the vulkan instance
    VkInstanceCreateInfo instanceInfo{};

    // TODO change to platform dependent evaluation may not be needed with the above calls to SDL
#if defined (__APPLE__)
    // Only seems to be required for macOS implementation and only when validation layers added
    config.addInstanceExtension("VK_KHR_get_physical_device_properties2");
    config.addInstanceExtension("VK_KHR_portability_enumeration");
    instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    // TODO swap out names above
    config.addInstanceExtension(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    // if (config.isExtendedSwapchainColorSpaceEnabled())
    // config.addInstanceExtension(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);

    // TODO investigate why not available on linux
    /* config.addInstanceExtension(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME); */

    // TODO LOG the required and found extensions and provide alternate paths if extension unavailable
    FC_ASSERT(config.areInstanceExtensionsSupported());

    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = config.getInstanceExtensionCount();
    instanceInfo.ppEnabledExtensionNames = config.getInstanceExtensions();
    instanceInfo.enabledLayerCount = config.getValidationLayerCount();
    instanceInfo.ppEnabledLayerNames = config.getValidationLayers();
    instanceInfo.pNext = config.getValidationLayerSettings();

    // Finally, call the vulkan function to create vulkan instance instance
    VK_ASSERT(vkCreateInstance(&instanceInfo, nullptr, &mInstance));

    // Give our instance handle to FcLocator to use with pointers to functions for Vulkan extensions etc.
    FcLocator::provide(mInstance);
  }  // END void FcRenderer::createInstance(...)


  //
  void FcRenderer::initDrawImage()
  {
    // Match our draw image to the window extent
    mDrawImage.createImage(mWindow.ScreenSize().width,
                           mWindow.ScreenSize().height, FcImageTypes::ScreenBuffer);

    // Create depth image (z buffer)
    mDepthImage.createImage(mWindow.ScreenSize().width, mWindow.ScreenSize().height
                       ,FcImageTypes::DepthBuffer);

    // TODO provide for these to change if VK_ERROR_OUT_OF_DATE_KHR, etc.
    mDrawExtent.height = std::min(mWindow.ScreenSize().height, mDrawImage.Height());
    mDrawExtent.width = std::min(mWindow.ScreenSize().width, mDrawImage.Width());

    // TODO create a function for allowing viewport and scisors to change
    // initialze the dynamic mDynamicViewport and mDynamicScisors
    mDynamicViewport.x = 0;
    mDynamicViewport.y = 0;
    mDynamicViewport.width = mDrawExtent.width;
    mDynamicViewport.height = mDrawExtent.height;
    mDynamicViewport.minDepth = 0.0f;
    mDynamicViewport.maxDepth = 1.0f;

    mDynamicScissors.offset = {0, 0};
    mDynamicScissors.extent.width = mDrawExtent.width;
    mDynamicScissors.extent.height = mDrawExtent.height;
  }

  // create the command buffers and command pools
  // TODO make two separate functions, one for pool creation, one for command buffer allocation
  void FcRenderer::createCommandPools()
  {
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FOR EACH FRAME   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.pNext = nullptr;
    commandPoolInfo.queueFamilyIndex = mGpu.getQueues().graphicsFamily;

    // Allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    // To support mult-threading, we need to add multiple command pools
    for (FrameAssets &frame : mFrames)
    {
      // TRY DELETE
      // VK_ASSERT(vkCreateCommandPool(pDevice, &commandPoolInfo, nullptr, &frame.commandPool));

      // // allocate command buffer from pool
      // allocInfo.commandPool = frame.commandPool;
      // VK_ASSERT(vkAllocateCommandBuffers(pDevice, &allocInfo, &frame.commandBuffer));
    }

    // TODO remove (in favor of VulkanImmediateCommands class)
    // // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GENERAL USE (IMMEDIATE)  -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // // allocate command pool for general renderer use
    // VK_ASSERT(vkCreateCommandPool(pDevice, &commandPoolInfo, nullptr, &mImmediateCommandPool));

    // // TODO try to create multiple command buffers for specific tasks that we can reuse...
    // // Allocate command buffer for immediate commands (ie. transitioning images/buffers to GPU)
    // allocInfo.commandPool = mImmediateCommandPool;

    // VK_ASSERT(vkAllocateCommandBuffers(pDevice, &allocInfo, &mImmediateCmdBuffer));

  } // --- FcRenderer::createCommandPools (_) --- (END)


  // TODO stage beginInfo, submit info, etc.
  // TODO SHOULD speed this up would by running on different queue than graphics queue so
  // we could overlap the execution from this with the main render loop
  // VkCommandBuffer FcRenderer::beginCommandBuffer()
  // {

  //   vkResetFences(pDevice, 1, &mImmediateFence);

  //   // information to be the command buffer record
  //   VkCommandBufferBeginInfo beginInfo{};
  //   beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  //   beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // becomes invalid after submit

  //   // begin recording transfer commands
  //   vkBeginCommandBuffer(mImmediateCmdBuffer, &beginInfo);

  //   // return commandBuffer;
  //   return mImmediateCmdBuffer;
  // }
  const CommandBufferWrapper& FcRenderer::beginCommandBuffer()
  {
    /* fcPrintEndl("Acquiring command buffer"); */
    // begin recording transfer commands
    const CommandBufferWrapper& cmdBuffer = mImmediateCommands.acquire();
    return cmdBuffer;
  }


  // void FcRenderer::submitCommandBuffer()
  // {
  //   // End commands
  //   vkEndCommandBuffer(mImmediateCmdBuffer);

  //   VkCommandBufferSubmitInfo cmdBufferSubmitInfo = {};
  //   cmdBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  //   cmdBufferSubmitInfo.commandBuffer = mImmediateCmdBuffer;

  //   // Queue submission information
  //   VkSubmitInfo2 submitInfo{};
  //   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  //   submitInfo.commandBufferInfoCount = 1;
  //   submitInfo.pCommandBufferInfos = &cmdBufferSubmitInfo;

  //   // Submit the command buffer to the queue
  //   VK_ASSERT(vkQueueSubmit2(mGpu.graphicsQueue(), 1, &submitInfo, mImmediateFence));

  //   vkWaitForFences(pDevice, 1, &mImmediateFence, true, U64_MAX);
  // }
  void FcRenderer::submitCommandBuffer(const CommandBufferWrapper& wrapper)
  {
    // const bool shouldPresent = hasSwapChain() && present;

    // if shouldPresent
    // {
    //   const u64
    // }
    // End commands
    mImmediateCommands.submit(wrapper);

    /* vkWaitForFences(pDevice, 1, &mImmediateFence, true, U64_MAX); */
  }


  // TODO should pass in variables from frolic.cpp here
  void FcRenderer::updateScene()
  {
    mTimer.start();

    // TODO not robust as getview must happen before inverseView
    //camera.setViewTarget(glm::vec3{0,0,4.0}, glm::vec3{0,0,-1});
    // TODO store/return position as a vec4
    mSceneData.eye = glm::vec4(pActiveCamera->Position(), 1.0f);
    mSceneData.view = pActiveCamera->getViewMatrix();
    // TODO pre-calculat this in camera
    mSceneData.viewProj = mSceneData.projection * mSceneData.view;

    // Use to bias shadow coordinates to match with vulkan
    // TODO should maybe incorporate clip into orthographic function
    const glm::mat4 clip = glm::mat4(0.5f, 0.0f, 0.0f, 0.0f,
                                     0.0f, 0.5f, 0.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 0.0f,
                                     0.5f, 0.5f, 0.0f, 1.0f);

    mSceneData.lighSpaceTransform = clip * mShadowMap.LightSpaceMatrix();
    mSceneDataBuffer.write(&mSceneData, sizeof(SceneDataUbo));

    // angle = rotationSpeed * 0.001f;
    // glm::vec3 rotationAxis = {0.f, -1.f, 0.f};


    // helmet.rotateInPlace(angle, rotationAxis);
    // /* structure2.rotateInPlace(angle, rotationAxis); */


    // // TODO could flag draw collection items with nodes that "know" they need to be updated
    // // then draw collection could do all the updating in one go.
    // helmet.update();
    // /* structure2.update(mDrawCollection); */


    // TODO
    // could update frustum by sending camera in and then could in turn be sent to
    // various rendering methods
    mFrustum.update(mSceneData.viewProj);
    /* mFrustum.normalize(); */
    mTerrain.update(mFrustum);

    // Update sceneUpdate time in ms
    mDrawCollection.stats.sceneUpdateTime = mTimer.elapsedTime();
  }


  //
  void FcRenderer::drawFrame()
  {
    // reset counters
    mDrawCollection.stats.objectsRendered = 0;
    mDrawCollection.stats.triangleCount = 0;
    // begin clock
    mTimer.start();

    FrameAssets& currentFrame = getCurrentFrame();

    // TRY DELETE
    /* VkCommandBuffer cmd = currentFrame.commandBuffer; */
    /* const CommandBufferWrapper& cmd = FcLocator::Renderer().beginCommandBuffer(); */

    // transition draw image from undefined layout to best format we can draw to
    mDrawImage.transitionLayout(mCurrentCommandBuffer, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    //
    mDepthImage.transitionLayout(mCurrentCommandBuffer, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    //
    mShadowMap.generateMap(mCurrentCommandBuffer, mDrawCollection);

    // TODO extract into builder...
    // begin a render pass connected to our draw image
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = mDrawImage.ImageView();
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR :
    //VK_ATTACHMENT_LOAD_OP_LOAD;

    // TODO Should use LOAD_OP_DONT_CARE here if we know the entire image will be written over
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // depth
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.imageView = mDepthImage.ImageView();
    depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil.depth = 0.f;

    //
    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, mDrawExtent};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = &depthAttachment;
    renderInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(mCurrentCommandBuffer, &renderInfo);

    vkCmdSetViewport(mCurrentCommandBuffer, 0, 1, &mDynamicViewport);
    vkCmdSetScissor(mCurrentCommandBuffer, 0, 1, &mDynamicScissors);

    // TODO implement without branches
    bool* shouldDrawShadowMap = CVarSystem::Get()->GetBoolCVar("shouldDrawShadowMap.bool");
    if (*shouldDrawShadowMap)
    {
      mShadowMap.drawDebugMap(mCurrentCommandBuffer, currentFrame);
    }
    else
    {
      mSceneRenderer.draw(mCurrentCommandBuffer, mDrawCollection, currentFrame, shouldDrawWireframe);

      // TODO condense this into an array of function pointers so that we can build
      // the specific 'pipeline' of function calls and avoid branches

      // Draw the bounding box around the object if enabled
      if (mDrawBoundingBoxes)
      {
        mBoundingBoxRenderer.draw(mCurrentCommandBuffer, mDrawCollection, currentFrame, mBoundingBoxId);
      }

      if(mDrawNormalVectors)
      {
        mNormalRenderer.draw(mCurrentCommandBuffer, mDrawCollection, currentFrame);
      }

      // TODO extrapolate functionality to frolic.cpp or cartridge
      // TODO have safegaurd for when the surface at index does not exist
      // only draw terrain when outside of building
      // const FcSurface& building = mDrawCollection.getSurfaceAtIndex(32);
      // // if (camera.isInside(building))...
      // if (building.isInBounds(mSceneData.eye))
      {
        mTerrain.draw(mCurrentCommandBuffer, mSceneData, shouldDrawWireframe);
      }

      // Draw the skybox last so that we can skip pixels with ANY object in front of it
      mSkybox.draw(mCurrentCommandBuffer, currentFrame);


      mBillboardRenderer.draw(mCurrentCommandBuffer, mSceneData, currentFrame);
    }

    vkCmdEndRendering(mCurrentCommandBuffer);

    // TODO call to resource manager Update(). Do any texture uploads along with deletions...
    // ?? For now update bindless textures here, after the rendering with textures has ended
    // TRY to relocate to a more descriptive/intuitive location if possible
    if (mDrawCollection.bindlessTextureUpdates.size())
    {
      // Handle deferred writes to bindless textures
      // TODO probably should define these as vectors of size bindlessTextureUpdates.size()
      VkWriteDescriptorSet bindlessDescriptorWrites[MAX_BINDLESS_RESOURCES];
      VkDescriptorImageInfo bindlessImageInfos[MAX_BINDLESS_RESOURCES];

      u32 currentWriteIndex = 0;
      for (i32 i = mDrawCollection.bindlessTextureUpdates.size() - 1; i >= 0; --i)
      {
        ResourceUpdate& textureToUpdate = mDrawCollection.bindlessTextureUpdates[i];

        // Update the descriptor set of each frame, since each frame has a separate D.S.
        for (FrameAssets& frame : mFrames)
        {
          // TRY only doing under the following circumstance
          /* if (textureToUpdate.current_frame == current_frame) */
          VkWriteDescriptorSet& descriptorWrite = bindlessDescriptorWrites[currentWriteIndex];
          descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
          descriptorWrite.descriptorCount = 1;
          descriptorWrite.dstArrayElement = textureToUpdate.handle;
          descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
          descriptorWrite.dstBinding = BINDLESS_DESCRIPTOR_SLOT;

          FcImage* texture;
          /* FCASSERT(textureToUpdate.handle == texture->Handle()); */

          if (textureToUpdate.type == ResourceDeletionType::Texture)
          {
            descriptorWrite.dstSet = frame.sceneBindlessTextureSet;
            texture = mDrawCollection.mTextures.get(textureToUpdate.handle);
          }
          else if (textureToUpdate.type == ResourceDeletionType::Billboard)
          {
            descriptorWrite.dstSet = frame.billboardDescriptorSet;
            texture = mDrawCollection.mBillboards.get(textureToUpdate.handle);
          }
          else
          {
            fcPrintEndl("Failed to Update Bindless Resource: ")
              /* std::cout << " << textureToUpdate.handle << std::endl; */
            descriptorWrite.dstSet = frame.billboardDescriptorSet;
            texture = &FcDefaults::Textures.checkerboard;
          }

          VkDescriptorImageInfo& imageInfo = bindlessImageInfos[currentWriteIndex];
          // TODO provide dummy image view
          imageInfo.imageView = texture->ImageView();
          imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
          imageInfo.sampler = texture->hasSampler() ?
                              texture->Sampler() : FcDefaults::Samplers.Linear;

          descriptorWrite.pImageInfo = &imageInfo;
          //
          ++currentWriteIndex;
        }

        textureToUpdate.currentFrame = U32_MAX;
        uint32_t size = mDrawCollection.bindlessTextureUpdates.size();
        mDrawCollection.bindlessTextureUpdates[i] = mDrawCollection.bindlessTextureUpdates[size - 1];
        mDrawCollection.bindlessTextureUpdates.pop_back();
      }

      if (currentWriteIndex)
      {
        vkUpdateDescriptorSets(pDevice, currentWriteIndex, bindlessDescriptorWrites, 0, nullptr);
      }
    }

    // ?? elapsed time should already be in ms
    mDrawCollection.stats.meshDrawTime = mTimer.elapsedTime() * 1000;
  }



  void FcRenderer::drawImGui(VkCommandBuffer cmd, VkImageView targetImageView)
  {
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = targetImageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea = VkRect2D{ VkOffset2D{0, 0}, mWindow.ScreenSize() };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = nullptr;
    renderInfo.pStencilAttachment = nullptr;

    // TODO eliminate separate calls to beging/end
    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
  }


  //
  // Create the Synchronization structures used in rendering each frame
  void FcRenderer::createSynchronization()
  {
    // *-*-*-*-*-*-*-*-*-*-*-   FRAME COMMAND SYNCHRONIZATION   *-*-*-*-*-*-*-*-*-*-*- //
    // mImageReadySemaphores.resize(MAX_FRAME_DRAWS);
    // mRenderFinishedSemaphores.resize(MAX_FRAME_DRAWS);
    // mDrawFences.resize(MAX_FRAME_DRAWS);

    // semaphore creation information
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // fence creation information
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    // we want this fence to start off signaled (open) so that it can go through the first draw function.
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (FrameAssets &frame : mFrames)
    {
      // TRY DELETE
      // // create 2 semaphores (one tells us the image is ready to draw to and one tells us when we're done drawing)
      VK_ASSERT(vkCreateSemaphore(pDevice, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore));
      VK_ASSERT(vkCreateSemaphore(pDevice, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore));

      // // create the fence that makes sure the draw commands of a a given frame is finished
      VK_ASSERT(vkCreateFence(pDevice, &fenceInfo, nullptr, &frame.renderFence));
    }


    // -*-*-*-*-*-*-*-*-*-*-*-*-   IMMEDIATE COMMAND SYNC   -*-*-*-*-*-*-*-*-*-*-*-*- //
    /* VK_ASSERT(vkCreateFence(pDevice, &fenceInfo, nullptr, &mImmediateFence)); */

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // create timeline semaphore
    mTimelineSemaphore = createTimelineSemaphore(pDevice, mSwapchain.imageCount() - 1, "Semaphore: mTimelineSemaphore");

    // Create aquire semaphores and fences
    // for (sizeT i = 0; i < mSwapchain.imageCount(); ++i)
    // {
    //   char debugName[256];
    //   snprintf(debugName, sizeof(debugName) - 1, "Semaphore: mAcquireSemaphore[%u]", i);
    //   mAcquireSemaphore[i] = createSemaphore(pDevice, debugName);

    //   //  TODO look into strlen instead of sizeof etc.
    //   snprintf(debugName, sizeof(debugName) - 1, "Fence: mAcquireFence[%u]            ", i);
    //   mAcquireFence[i] = createFence(pDevice, true, debugName);
    // }
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

  } // --- FcRenderer::createSynchronization (_) --- (END)

  ICommandBuffer& FcRenderer::beginFrame()
  {
    // call to update scene immediately (before waiting on fences)
    updateScene();

    mSwapchain.getCurrentFrame();

    mCurrentCommandBuffer = CommandBuffer(this);

    // ?? probably don't need to return DELETE
    return mCurrentCommandBuffer;
  } // _END_ beginFrame()


  // TODO fix nomenclature of currentFrame vs nextImageIndex
  // void FcRenderer::endFrame(uint32_t swapchainImageIndex)
  // {
  //   // alias for brevity
  //   VkCommandBuffer commandBuffer = getCurrentFrame().commandBuffer;

  //   // now that the draw has been done to the draw image,
  //   // transition it into transfer source layout so we can copy to the swapchain after
  //   mDrawImage.transitionLayout(commandBuffer,
  //                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  //                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

  //   // transiton the swapchain so it can best accept an image being copied to it
  //   mSwapchain.transitionImage(commandBuffer, swapchainImageIndex,
  //                              VK_IMAGE_LAYOUT_UNDEFINED,
  //                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

  //   // execute a copy from the draw image into the swapchain
  //   mSwapchain.getFcImage(swapchainImageIndex).copyFromImage(commandBuffer, &mDrawImage);

  //   // now transition swapchain image layout to attachment optimal so we can draw into it
  //   mSwapchain.transitionImage(commandBuffer, swapchainImageIndex
  //                              , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
  //                              , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
  //   //
  //   drawImGui(commandBuffer, mSwapchain.getFcImage(swapchainImageIndex).ImageView());

  //   // finally transition the swapchain image into presentable layout so we can present to surface
  //   mSwapchain.transitionImage(commandBuffer, swapchainImageIndex,
  //                              VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  //                              VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

  //   // stop recording to command buffer
  //   VK_ASSERT(vkEndCommandBuffer(commandBuffer));

  //   // prepare all the submit info for submiting commands to the queue
  //   VkCommandBufferSubmitInfo cmdBufferSubmitInfo = {};
  //   cmdBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
  //   cmdBufferSubmitInfo.commandBuffer = commandBuffer;

  //   VkSemaphoreSubmitInfo waitSemaphorInfo = {};
  //   waitSemaphorInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  //   waitSemaphorInfo.semaphore = getCurrentFrame().imageAvailableSemaphore;
  //   waitSemaphorInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
  //   waitSemaphorInfo.value = 1;

  //   VkSemaphoreSubmitInfo signalSemaphorInfo = {};
  //   signalSemaphorInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
  //   signalSemaphorInfo.semaphore = getCurrentFrame().renderFinishedSemaphore;
  //   signalSemaphorInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
  //   signalSemaphorInfo.value = 1;

  //   VkSubmitInfo2 submitInfo = {};
  //   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
  //   submitInfo.waitSemaphoreInfoCount = 1; // == nullptr ? 0 : 1 -> in build function
  //   submitInfo.pWaitSemaphoreInfos = &waitSemaphorInfo;
  //   submitInfo.signalSemaphoreInfoCount = 1; // == nullptr ? 0 : 1 -> in build function
  //   submitInfo.pSignalSemaphoreInfos = &signalSemaphorInfo;
  //   submitInfo.commandBufferInfoCount = 1;
  //   submitInfo.pCommandBufferInfos = &cmdBufferSubmitInfo;

  //   // Submit the command buffer to the queue
  //   VK_ASSERT(vkQueueSubmit2(mGpu.graphicsQueue(), 1, &submitInfo, getCurrentFrame().renderFence));

  //   // 3. present image to screen when it has signalled finished rendering
  //   VkPresentInfoKHR presentInfo = {};
  //   presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  //   presentInfo.waitSemaphoreCount = 1;                                       // Number of semaphores to wait on
  //   presentInfo.pWaitSemaphores = &getCurrentFrame().renderFinishedSemaphore; // semaphore to wait on
  //   presentInfo.swapchainCount = 1;                                           // number of swapchains to present to
  //   presentInfo.pSwapchains = &mSwapchain.vkSwapchain();                      // swapchain to present images to
  //   presentInfo.pImageIndices = &swapchainImageIndex;                         //index of images in swapchains to present

  //   VkResult result = vkQueuePresentKHR(mGpu.presentQueue(), &presentInfo);

  //   if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) // || mWindow.wasWindowResized())
  //   {
  //     fcPrintEndl("ERRROR OUT of date submit");
  //     // TODO  handle resize properly
  //     mShouldWindowResize = true;
  //     /* mWindow.resetWindowResizedFlag(); */
  //     /* handleWindowResize(); */
  //   }
  //   else if (result != VK_SUCCESS)
  //   {
  //     throw std::runtime_error("Faled to submit image to Vulkan Present Queue!");
  //   }

  //   // get next frame (use % MAX_FRAME_DRAWS to keep value below the number of frames we have in flight
  //   // increase the number of frames drawn
  //   mFrameNumber++;
  //   mDrawCollection.stats.frame = mFrameNumber;
  // }

  // ?? could pass vkCommandBuffer instead

  void FcRenderer::endFrame(ICommandBuffer& IcmdBuffer)
  {
    // TODO don't need this if using mCurrrentCommandBuffer

    CommandBuffer* cmdBuffer = static_cast<CommandBuffer*>(&IcmdBuffer);

    FC_ASSERT(cmdBuffer);
    FC_ASSERT(cmdBuffer->mRenderer);
    FC_ASSERT(cmdBuffer->mWrapper);

    const VkCommandBuffer cmd = cmdBuffer->getVkCommandBuffer();
    /* const VkCommandBuffer cmd = mCurrentCommandBuffer; */

    // now that we have finished drawing to our internal draw image, copy to swapchain and present
    mSwapchain.present(mDrawImage, cmd);

    // ?? should we do something with this handle, also, should it be lower in proceedure
    cmdBuffer->mLastSubmitHandle = mImmediateCommands.submit(*cmdBuffer->mWrapper);

    /* processDeferredTasks(); */
    mJanitor.flushBuffers();

    mCurrentCommandBuffer = {};

    // get next frame (use % MAX_FRAME_DRAWS to keep value below the number of frames we have in flight
    // increase the number of frames drawn
    /* mFrameNumber++; */
    /* mDrawCollection.stats.frame = mFrameNumber; */
    mDrawCollection.stats.frame = 0;
  }


  //
  void FcRenderer::handleWindowResize()
  {
    // On some platforms, it is normal that maxImageExtent may become (0, 0), for example when the
    // window is minimized. In such a case, it is not possible to create a swapchain due to the
    // Valid Usage requirements, so wait until the window is not minimized, for example, before
    // creating the new swapchain
    int width = 0, height = 0;
    SDL_Vulkan_GetDrawableSize(mWindow.SDLwindow(), &width, &height);
    // make sure to wait for finish resize
    while (width == 0 || height == 0)
    {
      SDL_Vulkan_GetDrawableSize(mWindow.SDLwindow(), &width, &height);
      SDL_WaitEvent(nullptr);
    }

    vkDeviceWaitIdle(pDevice);
    VkExtent2D winExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    //
    mSwapchain.reCreateSwapChain(winExtent);

    mShouldWindowResize = false;
  }


  void FcRenderer::shutDown()
  {
    /* std::cout << "calling: FcRenderer::shutDown" << std::endl; */
    // wait until no actions being run on device before destroying
    vkDeviceWaitIdle(pDevice);

    mDrawCollection.destroy();

    // Destroy data buffer with scene data
    mSceneDataBuffer.destroy();

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULTS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    FcDefaults::destroy();
    //mUiRenderer.destroy();

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   SCENE DATA   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    mSceneRenderer.destroy();
    mBoundingBoxRenderer.destroy();
    mNormalRenderer.destroy();
    mBillboardRenderer.destroy();

    vkDestroyDescriptorSetLayout(pDevice, mSceneDataDescriptorLayout, nullptr);

    // TODO should think about locating mImgGui into Descriptor Clerk
    FcLocator::DescriptorClerk().destroy();

    // close imGui
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(pDevice, mImgGuiDescriptorPool, nullptr);

    // TODO DELETE since we are no longer using per frame sync
    // ?? don't think that the reference is needed here
    for (FrameAssets &frame : mFrames)
    {
      // TODO should this have an allocator for the command pool
      // TRY DELETE
      // vkDestroyCommandPool(pDevice, frame.commandPool, nullptr);

      // vkDestroySemaphore(pDevice, frame.imageAvailableSemaphore, nullptr);
      // vkDestroySemaphore(pDevice, frame.renderFinishedSemaphore, nullptr);
      // vkDestroyFence(pDevice, frame.renderFence, nullptr);
    }

    // -*-*-*-*-*-*-*-*-*-*-   IMMEDIATE COMMAND ARCHITECTURE   -*-*-*-*-*-*-*-*-*-*- //
    /* vkDestroyCommandPool(pDevice, mImmediateCommandPool, nullptr); */
    /* vkDestroyFence(pDevice, mImmediateFence, nullptr); */

    // TODO conditionalize all elements that might not need destroying if outside
    // game is using engine and does NOT create all expected elements

    mSwapchain.destroy();

    mDrawImage.destroy();
    mDepthImage.destroy();

    mGpu.release(mInstance);

    mWindow.close(mInstance);

    if (mInstance != nullptr)
    {
      vkDestroyInstance(mInstance, nullptr);
    }
    // if (enableValidationLayers)
    // {
    //   /* DestroyDebugUtilsMessengerExt(mInstance, debugMessenger, nullptr); */
    // }
  }


} // END namespace fc
