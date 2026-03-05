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
#include "frolic.hpp"
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

  int FcRenderer::init(FrolicConfig& config, SceneDataUbo** pSceneData)
  {
    // TODO get rid of this perhaps
    try
    {
      // TODO get rid of this within renderer and instead initialize in frolic.cpp probably or GPU
      FcLocator::init();

      VkExtent2D screenDims {config.windowWidth, config.windowHeight};
      FcLocator::provide(screenDims);

      mWindow.initWindow(screenDims, config.applicationName);

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

      // now we need a vulkan instance to do anything else
      createInstance(appInfo);

      //TODO should do this in a builder class that goes out of scope when no longer needed
      SDL_version compiled;
      SDL_VERSION(&compiled);

      SDL_version linked;
      SDL_GetVersion(&linked);
      // Pring version info
      SDL_Log("SDL version(s): %u.%u.%u (compiled),  %u.%u.%u. (linked)\n",
              compiled.major, compiled.minor, compiled.patch, linked.major, linked.minor, linked.patch);
      SDL_Log("Window Dimensions: %i x %i", config.windowWidth, config.windowHeight);

      // create the surface (interface between Vulkan and window (SDL))
      mWindow.createWindowSurface(mInstance);

      // retrieve the physical device then create the logical device to interface with GPU & command pool
      if ( !mGpu.init(mInstance, mWindow))
      {
        throw std::runtime_error("Failed to initialize GPU device!");
      }

      FcLocator::provide(&mGpu);
      FcLocator::provide(this);

      pDevice = FcLocator::Gpu().getVkDevice();

      // create the swapchain & renderpass & frambuffers & depth buffer
      mSwapchain.init(mGpu, mWindow.ScreenSize());

      // -*-*-*-*-*-*-*-*-*-*-*-*-   INITIALIZE DESCRIPTORS   -*-*-*-*-*-*-*-*-*-*-*-*- //
      FcDescriptorClerk* descClerk = new FcDescriptorClerk;

      // register the descriptor with the locator
      FcLocator::provide(descClerk);

      // TODO document the pool ratios better
      std::vector<PoolSizeRatio> poolRatios = { {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6},
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

      // create all of the standard pipelines that the renderer relies on
      // TODO not sure if we actually need a member modedlRenderer or if we can delete after creation
      // TODO standardize all pipelines and draw calls
      // mModelRenderer.createPipeline(mModelPipeline);
      // mBillboardRenderer.createPipeline(mBillboardPipeline);
      // mUiRenderer.createPipeline(mUiPipeline);

      initImgui();

      // create the command buffers& command Pool
      //mPipeline.createUniformBuffers(&mGpu, mSwapchain, sizeof(UboViewProjection));
      // setup draw
      //mPipeline.createDescriptorPool();
      //mPipeline.createDescriptorSets();

      // synchronize the commandbuffers and swapchain
      createSynchronization();

      //TODO isolate the following
      // Create a mesh
      // Vertex Data

      // Setup elements of UI rendering
      // FcCamera UIcamera;
      // UIcamera.setViewDirection(glm::vec3(0.f, 0.f, -5.f), glm::vec3(0.f));
      // mBillboardUbo.view = UIcamera.View();

      // UIcamera.setOrthographicProjection(-1, 1, -1, 1, .1f, 100.f);
      // mBillboardUbo.view = UIcamera.Projection();       // TODO combine this into one function

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



  void FcRenderer::initImgui()
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
    pipelineRenderingInfo.pColorAttachmentFormats = &mSwapchain.getFormat();

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
  void FcRenderer::createInstance(VkApplicationInfo& appInfo)
  {
    // First determine all the extensions needed for SDL to run Vulkan instance
    uint32_t sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(mWindow.SDLwindow(), &sdlExtensionCount, nullptr);
    std::vector<const char*> extensions(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(mWindow.SDLwindow(), &sdlExtensionCount, extensions.data());

    // Include debug utilities only when required
#ifndef NDEBUG
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    // Not working for now
    // if (enableValidationLayers)
    // {
    //   extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
    // }

    // Define a Create struct to initialize the vulkan instance
    VkInstanceCreateInfo instanceInfo{};

    // TODO change to platform dependent evaluation may not be needed with the above calls to SDL
#if defined (__APPLE__)
    // Only seems to be required for macOS implementation and only when validation layers added
    extensions.push_back("VK_KHR_get_physical_device_properties2");
    extensions.push_back("VK_KHR_portability_enumeration");
    instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    extensions.push_back("VK_KHR_get_physical_device_properties2");
    // TODO investigate why not available on linux
    /* extensions.push_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME); */

    // TODO LOG the required and found extensions
    FC_ASSERT(areFeaturesSupported(extensions, FeatureType::InstanceExtension));

    // Next, determine the what validation layers we need
    std::vector<const char *> validationLayers;

    if (enableValidationLayers)
    {
      validationLayers.push_back("VK_LAYER_KHRONOS_validation");

      // make sure our Vulkan drivers support these validation layers
      if (! areFeaturesSupported(validationLayers, FeatureType::ValidationLayer))
      {
        throw std::runtime_error("Validation layers requested but not available!");
      }

      // TODO may not be available on linux so should probably provide alternate path
      // enable the best practices layer extension to warn about possible efficiency mistakes
      std::array<VkValidationFeatureEnableEXT, 3> featureEnables = {
	VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
	// Massively slows things down ( TODO enable after implementing)
	VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT,
	VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT,
      };

      VkValidationFeaturesEXT features = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
	.enabledValidationFeatureCount = static_cast<uint32_t>(featureEnables.size()),
	.pEnabledValidationFeatures = featureEnables.data(),
      };


      // Sometimes we need to disable specific Vulkan validation checks for performance or known
      // bugs in the validation layers
      VkBool32 gpuav_descriptor_checks = VK_FALSE;
      VkBool32 gpuav_indirect_draws_buffers = VK_FALSE;
      VkBool32 gpuav_post_proces_descriptor_indexing = VK_FALSE;

#define LAYER_SETTINGS_BOOL32(name, var)        \
      VkLayerSettingEXT {                       \
        .pLayerName = validationLayers[0],          \
      .pSettingName = name,                            \
              .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT, \
        .valueCount = 1,                                \
           .pValues = var }

      const std::array<VkLayerSettingEXT, 3> settings = {
        LAYER_SETTINGS_BOOL32("gpuav_descriptor_checks",
                              &gpuav_descriptor_checks),
	LAYER_SETTINGS_BOOL32("gpuav_indirect_draws_buffers",
                              &gpuav_indirect_draws_buffers),
	LAYER_SETTINGS_BOOL32("gpuav_post_process_descriptor_indexing",
                              &gpuav_post_proces_descriptor_indexing)
      };

#undef LAYER_SETTINGS_BOOL32

      const VkLayerSettingsCreateInfoEXT layerSettingsCreate = {
        .sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT,
	.settingCount = static_cast<u32>(settings.size()),
	.pSettings = settings.data(),
	.pNext = &features
      };

      instanceInfo.pNext = &layerSettingsCreate;

      fcPrintEndl("INFO: Validation Layers added!");
    }

    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    instanceInfo.ppEnabledLayerNames = validationLayers.data();

    // Finally, call the vulkan function to create vulkan instance instance
    VK_ASSERT(vkCreateInstance(&instanceInfo, nullptr, &mInstance));

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
    mDrawExtent.height = std::min(mSwapchain.getSurfaceExtent().height, mDrawImage.Height());
    mDrawExtent.width = std::min(mSwapchain.getSurfaceExtent().width, mDrawImage.Width());

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
    // get indices of queue families from device
    QueueFamilyIndices queueFamilyIndices = mGpu.getQueueFamilies();
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolInfo.pNext = nullptr;
    commandPoolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;

    // Allocate the default command buffer that we will use for rendering
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    // To support mult-threading, we need to add multiple command pools
    for (FrameAssets &frame : mFrames)
    {
      VK_ASSERT(vkCreateCommandPool(pDevice, &commandPoolInfo, nullptr, &frame.commandPool));

      // allocate command buffer from pool
      allocInfo.commandPool = frame.commandPool;
      VK_ASSERT(vkAllocateCommandBuffers(pDevice, &allocInfo, &frame.commandBuffer));
    }

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GENERAL USE (IMMEDIATE)  -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // allocate command pool for general renderer use
    VK_ASSERT(vkCreateCommandPool(pDevice, &commandPoolInfo, nullptr, &mImmediateCommandPool));

    // TODO try to create multiple command buffers for specific tasks that we can reuse...
    // Allocate command buffer for immediate commands (ie. transitioning images/buffers to GPU)
    allocInfo.commandPool = mImmediateCommandPool;

    VK_ASSERT(vkAllocateCommandBuffers(pDevice, &allocInfo, &mImmediateCmdBuffer));

  } // --- FcRenderer::createCommandPools (_) --- (END)


  // TODO stage beginInfo, submit info, etc.
  // TODO SHOULD speed this up would by running on different queue than graphics queue so
  // we could overlap the execution from this with the main render loop
  VkCommandBuffer FcRenderer::beginCommandBuffer()
  {
    vkResetFences(pDevice, 1, &mImmediateFence);

    // information to be the command buffer record
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // becomes invalid after submit

    // begin recording transfer commands
    vkBeginCommandBuffer(mImmediateCmdBuffer, &beginInfo);

    // return commandBuffer;
    return mImmediateCmdBuffer;
  }


  void FcRenderer::submitCommandBuffer()
  {
    // End commands
    vkEndCommandBuffer(mImmediateCmdBuffer);

    VkCommandBufferSubmitInfo cmdBufferSubmitInfo = {};
    cmdBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdBufferSubmitInfo.commandBuffer = mImmediateCmdBuffer;

    // Queue submission information
    VkSubmitInfo2 submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &cmdBufferSubmitInfo;

    // Submit the command buffer to the queue
    VK_ASSERT(vkQueueSubmit2(mGpu.graphicsQueue(), 1, &submitInfo, mImmediateFence));

    vkWaitForFences(pDevice, 1, &mImmediateFence, true, U64_MAX);
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

    VkCommandBuffer cmd = currentFrame.commandBuffer;

    // transition draw image from undefined layout to best format we can draw to
    mDrawImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    //
    mDepthImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);
    //
    mShadowMap.generateMap(cmd, mDrawCollection);

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

    vkCmdBeginRendering(cmd, &renderInfo);

    vkCmdSetViewport(cmd, 0, 1, &mDynamicViewport);
    vkCmdSetScissor(cmd, 0, 1, &mDynamicScissors);

    // TODO implement without branches
    bool* shouldDrawShadowMap = CVarSystem::Get()->GetBoolCVar("shouldDrawShadowMap.bool");
    if (*shouldDrawShadowMap)
    {
      mShadowMap.drawDebugMap(cmd, currentFrame);
    }
    else
    {
      mSceneRenderer.draw(cmd, mDrawCollection, currentFrame, shouldDrawWireframe);

      // TODO condense this into an array of function pointers so that we can build
      // the specific 'pipeline' of function calls and avoid branches

      // Draw the bounding box around the object if enabled
      if (mDrawBoundingBoxes)
      {
        mBoundingBoxRenderer.draw(cmd, mDrawCollection, currentFrame, mBoundingBoxId);
      }

      if(mDrawNormalVectors)
      {
        mNormalRenderer.draw(cmd, mDrawCollection, currentFrame);
      }

      // TODO extrapolate functionality to frolic.cpp or cartridge
      // TODO have safegaurd for when the surface at index does not exist
      // only draw terrain when outside of building
      // const FcSurface& building = mDrawCollection.getSurfaceAtIndex(32);
      // // if (camera.isInside(building))...
      // if (building.isInBounds(mSceneData.eye))
      {
        mTerrain.draw(cmd, mSceneData, shouldDrawWireframe);
      }

      // Draw the skybox last so that we can skip pixels with ANY object in front of it
      mSkybox.draw(cmd, currentFrame);


      mBillboardRenderer.draw(cmd, mSceneData, currentFrame);
    }

    vkCmdEndRendering(cmd);

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
    renderInfo.renderArea = VkRect2D{ VkOffset2D{0, 0}, mSwapchain.getSurfaceExtent() };
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = nullptr;
    renderInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(cmd, &renderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
  }



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
      // create 2 semaphores (one tells us the image is ready to draw to and one tells us when we're done drawing)
      VK_ASSERT(vkCreateSemaphore(pDevice, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore));
      VK_ASSERT(vkCreateSemaphore(pDevice, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore));

      // create the fence that makes sure the draw commands of a a given frame is finished
      VK_ASSERT(vkCreateFence(pDevice, &fenceInfo, nullptr, &frame.renderFence));
    }

    // -*-*-*-*-*-*-*-*-*-*-*-*-   IMMEDIATE COMMAND SYNC   -*-*-*-*-*-*-*-*-*-*-*-*- //
    VK_ASSERT(vkCreateFence(pDevice, &fenceInfo, nullptr, &mImmediateFence));

  } // --- FcRenderer::createSynchronization (_) --- (END)



  uint32_t FcRenderer::beginFrame()
  {
    // call to update scene immediately (before waiting on fences)
    updateScene();

    // don't keep adding images to the queue or commands to the buffer until last draw has finished
    vkWaitForFences(pDevice, 1, &getCurrentFrame().renderFence, VK_TRUE, U64_MAX);

    // delete any per frame resources no longer needed now the that frame has finished rendering
    // ?? this seems to be the wrong location for this, just by observation: test

//    getCurrentFrame().janitor.flush();

    // 1. get the next available image to draw to and set to signal the semaphore when we're finished with it
    uint32_t swapchainImageIndex;
    VkResult result = vkAcquireNextImageKHR(pDevice, mSwapchain.vkSwapchain(), U64_MAX
                                            , getCurrentFrame().imageAvailableSemaphore
                                            , VK_NULL_HANDLE, &swapchainImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
      fcPrintEndl("ERROR out of date submit1");
      mShouldWindowResize = true;
      //handleWindowResize();
      return -1;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
      throw std::runtime_error("Failed to acquire Vulkan Swap Chain image!");
    }

    // manully un-signal (close) the fence ONLY when we are sure we're submitting work (result == VK_SUCESS)
    vkResetFences(pDevice, 1, &getCurrentFrame().renderFence);

    // ?? don't think we need this assert since we use semaphores and fences
    // assert(!mIsFrameStarted && "Can't call recordCommands() while frame is already in progress!");

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    // alias for brevity
    VkCommandBuffer commandBuffer = getCurrentFrame().commandBuffer;

    // TODO remove after extensive test if this is even needed or does vkBegin... carry implicit reset
    // since we know the commands finished executing (vkFence), reset command buffer to begin recording again
    // if (vkResetCommandBuffer(commandBuffer, 0) != VK_SUCCESS)
    // {
    //   throw std::runtime_error("Failed to reset command buffer!");
    // }

    // information about how to begin each command
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // let vulkan know that we intend to only use this buffer once
    bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    VK_ASSERT(vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo));

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROM OLD METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    // TODO would this make more sense to relocate to window resize?
    // ?? also, what are the costs associated with having dynamic states
    // make sure our dynamic viewport and scissors are set properly (if resizing the window etc.)
    // mDynamicViewport.width = static_cast<uint32_t>(mSwapchain.getSurfaceExtent().width);
// mDynamicViewport.height = static_cast<uint32_t>(mSwapchain.getSurfaceExtent().height);
    // mDynamicScissors.extent = mSwapchain.getSurfaceExtent();
    //  //
    // vkCmdSetViewport(commandBuffer, 0, 1, &mDynamicViewport);
    // vkCmdSetScissor(commandBuffer, 0, 1, &mDynamicScissors);

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   END FROM OLD METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*- //


    // VkImageSubresourceRange clearRange {};
    // clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // clearRange.baseMipLevel = 0;
    // clearRange.levelCount = VK_REMAINING_MIP_LEVELS;
    // clearRange.baseArrayLayer = 0;
    // clearRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    //  //make a clear-color from frame number. This will flash with a 120 frame period.

    //  VkClearDepthStencilValue clearValue;
    // //float flash = std::abs(std::sin(mFrameNumber / 60.f));
    //  clearValue = { 0.0f};

    //  vkCmdClearDepthStencilImage(commandBuffer, mDepthImage.Image(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
    /* vkCmdClearColorImage(commandBuffer, mDrawImage.Image(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange); */
    /* vkCmdClearDepthStencilImage(cmd, mSwapchain., VkImageLayout imageLayout, const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange *pRanges) */
    // bind the compute pipeline

    // ?? not sure this is what we want to return
    return swapchainImageIndex;
  } // _END_ beginFrame()

  // *-*-*-*-*-*-   OLD METHOD (VULKAN 1_1 WHEN SYNCH2 NOT AVAILABLE)   *-*-*-*-*-*- //

  // // information about how to begin each command
  //     VkCommandBufferBeginInfo bufferBeginInfo = {};
  //     bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  //      // information about how to begin a render pass (only needed for graphical applications)
  //     VkRenderPassBeginInfo renderPassBeginInfo = {};
  //     renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  //     renderPassBeginInfo.renderPass = mSwapchain.getRenderPass();
  //     renderPassBeginInfo.renderArea.offset = {0, 0};
  //     renderPassBeginInfo.renderArea.extent = mSwapchain.getSurfaceExtent();

  //     std::array<VkClearValue, 3> clearValues = {};
  //     clearValues[0].color = {0.6f, 0.65f, 0.4f, 1.0f};
  //     clearValues[1].color = {0.0f, 0.0f, 0.0f}; // ?? why are there 2 color attachmentes and not 1?
  //     clearValues[2].depthStencil.depth = 1.0f;

  //     renderPassBeginInfo.pClearValues = clearValues.data();
  //      // TODO since we know this ahead of time always--set simply
  //     renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

  //      // assign the framebuffer of the corresponding command buffer to assign to the render pass
  //     renderPassBeginInfo.framebuffer = mSwapchain.getFrameBuffer(nextSwapchainImage);

  //      // start recording commands to command buffer
  //     if (vkBeginCommandBuffer(mCommandBuffers[nextSwapchainImage], &bufferBeginInfo) != VK_SUCCESS)
  //     {
  //       throw std::runtime_error("Failed to start recording a Vulkan Command Buffer!");
  //     }

  //      // Begin render pass
  //     vkCmdBeginRenderPass(mCommandBuffers[nextSwapchainImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  //      // TODO would this make more sense to relocate to window resize?
  //      // ?? also, what are the costs associated with having dynamic states
  //      // make sure our dynamic viewport and scissors are set properly (if resizing the window etc.)
  //     mDynamicViewport.width = static_cast<uint32_t>(mSwapchain.getSurfaceExtent().width);
  //     mDynamicViewport.height = static_cast<uint32_t>(mSwapchain.getSurfaceExtent().height);
  //     mDynamicScissors.extent = mSwapchain.getSurfaceExtent();
  //      //
  //     vkCmdSetViewport(mCommandBuffers[nextSwapchainImage], 0, 1, &mDynamicViewport);
  //     vkCmdSetScissor(mCommandBuffers[nextSwapchainImage], 0, 1, &mDynamicScissors);

  //     return nextSwapchainImage;



  // TODO fix nomenclature of currentFrame vs nextImageIndex
  void FcRenderer::endFrame(uint32_t swapchainImageIndex)
  {
    // alias for brevity
    VkCommandBuffer commandBuffer = getCurrentFrame().commandBuffer;

    // now that the draw has been done to the draw image,
    // transition it into transfer source layout so we can copy to the swapchain after
    mDrawImage.transitionLayout(commandBuffer,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // transiton the swapchain so it can best accept an image being copied to it
    mSwapchain.transitionImage(commandBuffer, swapchainImageIndex,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // execute a copy from the draw image into the swapchain
    mSwapchain.getFcImage(swapchainImageIndex).copyFromImage(commandBuffer, &mDrawImage);

    // now transition swapchain image layout to attachment optimal so we can draw into it
    mSwapchain.transitionImage(commandBuffer, swapchainImageIndex
                               , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                               , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    //
    drawImGui(commandBuffer, mSwapchain.getFcImage(swapchainImageIndex).ImageView());

    // finally transition the swapchain image into presentable layout so we can present to surface
    mSwapchain.transitionImage(commandBuffer, swapchainImageIndex,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // stop recording to command buffer
    VK_ASSERT(vkEndCommandBuffer(commandBuffer));

    // prepare all the submit info for submiting commands to the queue
    VkCommandBufferSubmitInfo cmdBufferSubmitInfo = {};
    cmdBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    cmdBufferSubmitInfo.commandBuffer = commandBuffer;

    VkSemaphoreSubmitInfo waitSemaphorInfo = {};
    waitSemaphorInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    waitSemaphorInfo.semaphore = getCurrentFrame().imageAvailableSemaphore;
    waitSemaphorInfo.stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
    waitSemaphorInfo.value = 1;

    VkSemaphoreSubmitInfo signalSemaphorInfo = {};
    signalSemaphorInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    signalSemaphorInfo.semaphore = getCurrentFrame().renderFinishedSemaphore;
    signalSemaphorInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
    signalSemaphorInfo.value = 1;

    VkSubmitInfo2 submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    submitInfo.waitSemaphoreInfoCount = 1; // == nullptr ? 0 : 1 -> in build function
    submitInfo.pWaitSemaphoreInfos = &waitSemaphorInfo;
    submitInfo.signalSemaphoreInfoCount = 1; // == nullptr ? 0 : 1 -> in build function
    submitInfo.pSignalSemaphoreInfos = &signalSemaphorInfo;
    submitInfo.commandBufferInfoCount = 1;
    submitInfo.pCommandBufferInfos = &cmdBufferSubmitInfo;

    // Submit the command buffer to the queue
    VK_ASSERT(vkQueueSubmit2(mGpu.graphicsQueue(), 1, &submitInfo, getCurrentFrame().renderFence));

    // 3. present image to screen when it has signalled finished rendering
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;                                       // Number of semaphores to wait on
    presentInfo.pWaitSemaphores = &getCurrentFrame().renderFinishedSemaphore; // semaphore to wait on
    presentInfo.swapchainCount = 1;                                           // number of swapchains to present to
    presentInfo.pSwapchains = &mSwapchain.vkSwapchain();                      // swapchain to present images to
    presentInfo.pImageIndices = &swapchainImageIndex;                         //index of images in swapchains to present

    VkResult result = vkQueuePresentKHR(mGpu.presentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) // || mWindow.wasWindowResized())
    {
      fcPrintEndl("ERRROR OUT of date submit");
      // TODO  handle resize properly
      mShouldWindowResize = true;
      /* mWindow.resetWindowResizedFlag(); */
      /* handleWindowResize(); */
    }
    else if (result != VK_SUCCESS)
    {
      throw std::runtime_error("Faled to submit image to Vulkan Present Queue!");
    }

    // get next frame (use % MAX_FRAME_DRAWS to keep value below the number of frames we have in flight
    // increase the number of frames drawn
    mFrameNumber++;
    mDrawCollection.stats.frame = mFrameNumber;
  }



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

    // ?? don't think that the reference is needed here
    for (FrameAssets &frame : mFrames)
    {
      // TODO should this have an allocator for the command pool
      vkDestroyCommandPool(pDevice, frame.commandPool, nullptr);

      vkDestroySemaphore(pDevice, frame.imageAvailableSemaphore, nullptr);
      vkDestroySemaphore(pDevice, frame.renderFinishedSemaphore, nullptr);
      vkDestroyFence(pDevice, frame.renderFence, nullptr);
    }


    // -*-*-*-*-*-*-*-*-*-*-   IMMEDIATE COMMAND ARCHITECTURE   -*-*-*-*-*-*-*-*-*-*- //
    vkDestroyCommandPool(pDevice, mImmediateCommandPool, nullptr);
    vkDestroyFence(pDevice, mImmediateFence, nullptr);

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
    if (enableValidationLayers)
    {
      /* DestroyDebugUtilsMessengerExt(mInstance, debugMessenger, nullptr); */
    }
  }


} // END namespace fc
