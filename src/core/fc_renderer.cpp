#include "fc_renderer.hpp"


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_game_object.hpp"
#include "core/fc_locator.hpp"
#include "core/platform.hpp"
#include "utilities.hpp"
#include "fc_debug.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_stdinc.h"
#include <SDL_events.h>
// #define GLM_FORCE_RADIANS
// #define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/packing.hpp>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include <SDL2/SDL_vulkan.h>
// ImGUI
#include "imgui.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <mutex>
#include <string>
#include <exception>
#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <cstdlib>
#include <unordered_set>
#include <iostream>

// TODO (note may no longer be relevant) All of the helper functions that submit commands so far
// have been set up to execute synchronously by waiting for the queue to become idle. For practical
// applications it is recommended to combine these operations in a single command buffer and execute
// them asynchronously for higher throughput, especially the transitions and copy in the
// createTextureImage function. Try to experiment with this by creating a setupCommandBuffer that
// the helper functions record commands into, and add a flushSetupCommands to execute the commands
// that have been recorded so far. It's best to do this after the texture mapping works to check if
// the texture resources are still set up correctly.

namespace fc
{
  // FcRenderer::FcRenderer()
  // {
  //    // TODO allow a passed struct that will initialize the window and app "stuff"
  //    // including a pointer to a VK_STRUCTURE type appinfo
  // }

  int FcRenderer::init(VkApplicationInfo& appInfo, VkExtent2D screenSize)
  {
    // TODO get rid of this perhaps
    try
    {
      // TODO get rid of this
      FcLocator::initialize();

      mWindow.initWindow(screenSize.width, screenSize.height);

      // now we need a vulkan instance to do anything else
      createInstance(appInfo);

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
      FcDescriptorClerk* descriptorClerk = new FcDescriptorClerk;
      // TODO understand the pool ratios better
      std::vector<PoolSizeRatio> poolRatios = { {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 6},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 6},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8}
      };

      //
      descriptorClerk->initDescriptorPools(1000, poolRatios);

      // register the descriptor with the locator
      FcLocator::provide(descriptorClerk);


      initDrawImage();

      // create the command pool for later allocating command from. Also create the command buffers
      createCommandPools();





      // create the graphics pipeline && create/attach descriptors
      // create the uniform buffers & initialize the descriptor sets that tell the pipeline about our uniform buffers
      // here we want to create a descriptor set for each swapchain image we have
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


      // FcModel model;
      // model.createModel("models/smooth_vase.obj", mPipeline, mGpu);
      // mModelList.push_back(model);
    }
    catch (const std::runtime_error& err) {
      printf("ERROR: %s\n", err.what());
      return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
  }



  void FcRenderer::initDefaults(FcBuffer& sceneDataBuffer, SceneData* sceneData)
  {
    pSceneData = sceneData;
    // -*-*-*-*-   3 DEFAULT TEXTURES--WHITE, GREY, BLACK AND CHECKERBOARD   -*-*-*-*- //
    uint32_t white = glm::packUnorm4x8(glm::vec4(1.f, 1.f, 1.f, 1.f));
    mWhiteTexture.createTexture(1, 1, static_cast<void*>(&white)
                                , sizeof(white));

    uint32_t grey = glm::packUnorm4x8(glm::vec4(0.36f, 0.36f, 0.36f, 1.f));
    mGreyTexture.createTexture(1, 1, static_cast<void*>(&grey)
                               , sizeof(grey));

    uint32_t black = glm::packUnorm4x8(glm::vec4(0.f, 0.f, 0.f, 1.f));
    mBlackTexture.createTexture(1, 1, static_cast<void*>(&black)
                                , sizeof(black));

    // checkerboard image
    uint32_t checkerColor = glm::packUnorm4x8(glm::vec4(1.f, 0.f, 1.f, 1.f));
    std::array<uint32_t, 16 * 16> pixels;
    for (int x = 0; x < 16; x++)
    {
      for (int y = 0; y < 16; y++)
      {
        pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? checkerColor : black;
      }
    }
    mCheckerboardTexture.createTexture(16, 16, static_cast<void*>(&pixels)
                                       , pixels.size() * sizeof(pixels[0]));

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    vkCreateSampler(pDevice, &samplerInfo, nullptr, &mDefaultSamplerNearest);

    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(pDevice, &samplerInfo, nullptr, &mDefaultSamplerLinear);

    // TODO take advantage of the fact that Descriptor params can be reset once it spits out the SET
    // TODO probably delete from here
    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

    FcDescriptorBindInfo descriptorBindInfo{};
    descriptorBindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                                  , VK_SHADER_STAGE_FRAGMENT_BIT);

    descriptorBindInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mGreyTexture
                                   , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mDefaultSamplerNearest);


    // *-*-*-*-*-*-*-*-*-*-*-*-   FRAME DATA INITIALIZATION   *-*-*-*-*-*-*-*-*-*-*-*- //
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

    sceneDescriptorBinding.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, sceneDataBuffer
                                        , sizeof(SceneData), 0);

    // create descriptorSet for sceneData
    mSceneDataDescriptorLayout = descClerk.createDescriptorSetLayout(sceneDescriptorBinding);


    // Allocate a descriptorSet to each frame buffer
    for (FrameData& frame : mFrames)
    {
      frame.sceneDataDescriptorSet = descClerk.createDescriptorSet(
        mSceneDataDescriptorLayout, sceneDescriptorBinding);
      // frame.sceneDataDescriptorSet = descClerk.createDescriptorSet(mSingleImageDescriptorLayout
      //                                                              , descriptorBindInfo);
    }

    // TODO these should all be a part of frolic or game class, not the renderer
    mSkybox.loadTextures("..//models//skybox", ".jpg");
    // TODO should be more descriptive in name to show this has to happen after loadTextures
    mSkybox.init(mSceneDataDescriptorLayout);
    //
    mShadowMap.init(this);

    //
    //mTerrain.init(this, "..//maps/simple.png");
    /* mTerrain.init(this, "..//maps/metalplate01_rgba.ktx"); */
    mTerrain.init(this, "..//maps/terrain_heightmap_r16.ktx2");
    //mTerrain.init(this, "..//maps/terrain_heightmap_r16.ktx");
    // BUG may need render fence or semaphor
    // mTerrain.init(this, "..//maps/iceland_heightmap.png");
    // vkDeviceWaitIdle(pDevice);

    // TODO Organize initialization

    // set the uniform buffer for the material data
    materialConstants.allocate(sizeof(MaterialConstants), FcBufferTypes::Uniform);

    // write the buffer
    MaterialConstants* materialUniformData =
      (MaterialConstants*)materialConstants.getAddres();

    materialUniformData->colorFactors = glm::vec4{1,1,1,1};
    materialUniformData->metalRoughFactors = glm::vec4{1, 0.5, 0, 0};

    GLTFMetallicRoughness::MaterialResources materialResources;
    // default the material textures
    materialResources.dataBuffer = materialConstants;
    materialResources.colorImage = mWhiteTexture;
    materialResources.colorSampler = mDefaultSamplerLinear;
    materialResources.metalRoughImage = mWhiteTexture;
    materialResources.metalRoughSampler = mDefaultSamplerLinear;
    materialResources.normalTexture = mWhiteTexture;
    materialResources.normalSampler = mDefaultSamplerLinear;
    materialResources.occlusionTexture = mWhiteTexture;
    materialResources.occlusionSampler = mDefaultSamplerLinear;
    materialResources.emissiveTexture = mBlackTexture;
    materialResources.emissiveSampler = mDefaultSamplerLinear;
    materialResources.dataBufferOffset = 0;

    // // TODO think about destroying layout here
    mMetalRoughMaterial.buildPipelines(this);

    defaultMaterialData = mMetalRoughMaterial.writeMaterial(pDevice, MaterialPass::MainColor
                                                          , materialResources);

    // TODO implement with std::optional
    // TODO move to frolic.cpp
    //structure.loadGltf(this, "..//models//MosquitoInAmber.glb");
    //structure.loadGltf(this, "..//models//MaterialsVariantsShoe.glb");
    structure.loadGltf(this, "..//models//helmet//DamagedHelmet.gltf");
    //structure2.loadGltf(this, "..//models//helmet//DamagedHelmet.gltf");
    //structure.loadGltf(this, "..//models//Box.gltf");
    //structure.loadGltf(this, "..//models//GlassHurricaneCandleHolder.glb");
    //structure.loadGltf(this, "..//models//ToyCar.glb");
    //structure.loadGltf(this, "..//models//structure_mat.glb");

    structure2.loadGltf(this, "..//models//sponza//Sponza.gltf");



    // // NOTE: This moves the object not the camera/lights/etc...
    // glm::mat4 drawMat{1.f};
    // glm::vec3 translate{30.f, -00.f, 85.f};
    // drawMat = glm::translate(drawMat, translate);
    // structure.update(drawMat);

    // BUG investigate why this file doesn't load
    //structure.loadGltf(this, "..//models//monkey.glb");

    // FIXME requires enabling one or more extensions in fastgltf
    //structure.loadGltf(this, "..//models//SheenWoodLeatherSofa.glb");




    initNormalDrawPipeline(sceneDataBuffer);
    initBoundingBoxPipeline(sceneDataBuffer);
    // TODO remove at some point but prefer to leave in while debugging
    vkDeviceWaitIdle(pDevice);
  }



  void FcRenderer::initImgui()
  {
    // Create the descriptor pool for IMGUI
    // MASSIVELY oversized, doesn't seem to be in any imgui demo that was mentioned
    // TODO /TRY try and follow this up by recreating the example from ImgGUI site
    VkDescriptorPoolSize poolSizes[] = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };
    // {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    // {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
    // {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
    // {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
    // {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
    // {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
    // {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
    // {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
    // {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
    // {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000}


    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = poolSizes;


    if (vkCreateDescriptorPool(pDevice, &poolInfo, nullptr, &mImgGuiDescriptorPool) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to allocate Descriptor Pool for ImGUI");
    }

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



  void FcRenderer::updateUseFlags(MaterialFeatures featureToUpdate, bool enable)
  {
    // Get the address for the buffer of material constant data being refd by the shader
    MaterialConstants* changed
      = static_cast<MaterialConstants*>(structure.mMaterialDataBuffer.getAddres());

    // TODO need to this for all "structures"
    int numMaterial = structure.mMaterials.size();
    std::cout << "numMaterials = " << numMaterial << std::endl;
    if (enable)
    {
      for (size_t i = 0; i < numMaterial; i++)
      {
        changed->flags |= featureToUpdate;
        changed++;
      }
    }
    else
    {
      for (size_t i = 0; i < numMaterial; i++)
      {
        changed->flags &= ~featureToUpdate;
        changed++;
      }
    }
  }

  void FcRenderer::setColorTextureUse(bool enable)
  {
    updateUseFlags(MaterialFeatures::HasColorTexture, enable);
  }

  void FcRenderer::setRoughMetalUse(bool enable)
  {
    updateUseFlags(MaterialFeatures::HasRoughMetalTexture, enable);
  }

  void FcRenderer::setAmbientOcclussionUse(bool enable)
  {
    updateUseFlags(MaterialFeatures::HasOcclusionTexture, enable);
  }

  void FcRenderer::setNormalMapUse(bool enable)
  {
    updateUseFlags(MaterialFeatures::HasNormalTexture, enable);
  }

  void FcRenderer::setEmissiveTextureUse(bool enable)
  {
    updateUseFlags(MaterialFeatures::HasEmissiveTexture, enable);
  }



  void FcRenderer::createInstance(VkApplicationInfo& appInfo)
  {
    // First determine all the extensions needed for SDL to run Vulkan instance
    uint32_t sdlExtensionCount = 0;
    SDL_Vulkan_GetInstanceExtensions(mWindow.SDLwindow(), &sdlExtensionCount, nullptr);
    std::vector<const char*> extensions(sdlExtensionCount);
    SDL_Vulkan_GetInstanceExtensions(mWindow.SDLwindow(), &sdlExtensionCount, extensions.data());

    // finally, define a Create struct to initialize the vulkan instance
    VkInstanceCreateInfo instanceInfo{};

    // TODO change to platform dependent evaluation
    // Only seems to be required for macOS implementation and only when validation layers added
    // extensions.push_back("VK_KHR_get_physical_device_properties2");
    // extensions.push_back("VK_KHR_portability_enumeration");
    // instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

    // TODO LOG the required and found extensions
    if (!areInstanceExtensionsSupported(extensions))
    {
      throw std::runtime_error("Missing required SDL extension");
    }

    // Next, determine the what validation layers we need
    std::vector<const char *> validationLayers;

    if (enableValidationLayers)
    {
      validationLayers.push_back("VK_LAYER_KHRONOS_validation");
      // check that Vulkan drivers support these validation layers
      if (!areValidationLayersSupported(validationLayers))
      {
        throw std::runtime_error("Validation layers requested but not available!");
      }

      // enable the best practices layer extension to warn about possible efficiency mistakes
      std::array<VkValidationFeatureEnableEXT, 1> featureEnables = {VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT};
      VkValidationFeaturesEXT features = {};
      features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
      features.enabledValidationFeatureCount = static_cast<uint32_t>(featureEnables.size());
      features.pEnabledValidationFeatures = featureEnables.data();
      instanceInfo.pNext = &features;
      SDL_Log("Validation Layers added!");
    }

    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    instanceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    instanceInfo.ppEnabledExtensionNames = extensions.data();
    instanceInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    // ?? Make sure this still works when validationlayers is empty!
    instanceInfo.ppEnabledLayerNames = validationLayers.data();

    // now just call the vulkan function to create an instance
    VkResult result = vkCreateInstance(&instanceInfo, nullptr, &mInstance);

    if (result != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Vulkan Instance!");
    }


  }  // END void FcRenderer::createInstance(...)



  bool FcRenderer::areInstanceExtensionsSupported(const std::vector<const char*>& instanceExtensions)
  {
    // populate list of all available vulkan instance extensions
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

    // add all the required extentions to an unordered set for easy search
    std::unordered_set<std::string> requiredExtensions(instanceExtensions.begin(), instanceExtensions.end());

    // now go through and delete all the extensions that we know are available from the required list
    for (const auto& extension : availableExtensions)
    {
      //std::cout << extension.extensionName << std::endl;
      requiredExtensions.erase(extension.extensionName);
    }

    // return true if all the all the passed instance extensions were found in all available extensions
    return requiredExtensions.empty();
  }


  void FcRenderer::initDrawImage()
  {
    // match our draw image to the window extent
  mDrawImage.create(mWindow.ScreenSize().width, mWindow.ScreenSize().height
                    , FcImageTypes::ScreenBuffer);

    // *-*-*-*-*-*-*-*-*-*-*-   CREATE DRAW IMAGE DESCRIPTOR   *-*-*-*-*-*-*-*-*-*-*- //
    // TODO some redundancy that might be able to be eliminated
    FcDescriptorBindInfo bindingInfo;
    bindingInfo.addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
    bindingInfo.attachImage(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
                            , mDrawImage, VK_IMAGE_LAYOUT_GENERAL, nullptr);

    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
    mBackgroundDescriptorlayout = descClerk.createDescriptorSetLayout(bindingInfo);
    mDrawImageDescriptor = descClerk.createDescriptorSet(mBackgroundDescriptorlayout, bindingInfo);

    // since we don't need the DSLayout, get rid of it
    //vkDestroyDescriptorSetLayout(pDevice, mBackgroundDescriptorlayout, nullptr);

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE DEPTH IMAGE   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    mDepthImage.create(mWindow.ScreenSize().width, mWindow.ScreenSize().height
                       ,FcImageTypes::DepthBuffer);


    // TODO provide for these to change if VK_ERROR_OUT_OF_DATE_KHR, etc.
    mDrawExtent.height = std::min(mSwapchain.getSurfaceExtent().height
                                  , mDrawImage.Height());// * renderScale;
    mDrawExtent.width = std::min(mSwapchain.getSurfaceExtent().width
                                 , mDrawImage.Width());// * renderScale;

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
  // TODO make two seperate functions, one for pool creation, one for command buffer allocation
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
    for (FrameData &frame : mFrames)
    {
      if (vkCreateCommandPool(pDevice, &commandPoolInfo, nullptr, &frame.commandPool) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create a Vulkan Command Pool!");
      }

      allocInfo.commandPool = frame.commandPool;

      // allocate command buffer from pool
      if (vkAllocateCommandBuffers(pDevice, &allocInfo, &frame.commandBuffer) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to allocate a Vulkan Command Buffer!");
      }
    }

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GENERAL USE (IMMEDIATE)  -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // allocate command pool for general renderer use
    if (vkCreateCommandPool(pDevice, &commandPoolInfo, nullptr, &mImmediateCommandPool) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Command Pool!");
    }

    // TODO try to create multiple command buffers for specific tasks that we can reuse...
    // Allocate command buffer for immediate commands (ie. transitioning images/buffers to GPU)
    allocInfo.commandPool = mImmediateCommandPool;

    if (vkAllocateCommandBuffers(pDevice, &allocInfo, &mImmediateCmdBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to allocate a Vulkan Command Buffer!");
    }
  } // --- FcRenderer::createCommandPools (_) --- (END)


  // TODO stage beginInfo, submit info, etc.
  // TODO SHOULD speed this up would by running on different queue than graphics queue so
  // we could overlap the execution from this with the main render loop
  VkCommandBuffer FcRenderer::beginCommandBuffer()
  {
    // command buffer to hold transfer commands
    //VkCommandBuffer commandBuffer;
    vkResetFences(pDevice, 1, &mImmediateFence);

    // ?? shouldn't be necessary if recording new commands
    //vkResetCommandBuffer(mImmediateCmdBuffer, 0);

    // TODO DELETE (buffer already allocated)
    // command buffer details
    // VkCommandBufferAllocateInfo allocInfo{};
    // allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // allocInfo.commandPool = mImmediateCommandPool;
    // allocInfo.commandBufferCount = 1;

    //  // allocate command buffer from pool
    // vkAllocateCommandBuffers(FcLocator::Device(), &allocInfo, &commandBuffer);

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
    // TODO assert, don't check like this
    if (vkQueueSubmit2(mGpu.graphicsQueue(), 1, &submitInfo, mImmediateFence) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to submit Vulkan Command Buffer to the queue!");
    }

    vkWaitForFences(pDevice, 1, &mImmediateFence, true, u64_max);
  }


  // TODO should pass in variables from frolic.cpp here
  void FcRenderer::updateScene()
  {
    mTimer.start();
    // TODO this is calling the destructor for all objects in draw, should flatten more
    // to just the necessary AND changing parameters...
    mainDrawContext.opaqueSurfaces.clear();
    mainDrawContext.transparentSurfaces.clear();

    rotationMatrix = glm::rotate(rotationMatrix
                                 , rotationSpeed * .0001f * glm::pi<float>(), {0.f, -1.f, 0.f});

    // TODO alter the independence of these two separate calls
    // Might be better to combine them but also a waste when considering that many objects are
    // stationary and therefore do not require an update. Probably better to have static & dynamic
    // objects be "drawn differently" also hate that this functio is called draw just because
    // it adds the meshNodes to the drawContext...
    glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 3.0f, 0.0f));
    structure.update(translationMat * rotationMatrix);
    structure.draw(mainDrawContext);
    // glm::mat4 translationMat = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 18.0f));
    // structure2.update(translationMat);
     structure2.draw(mainDrawContext);

     // TODO
     // could update frustum by sending camera in and then could in turn be sent to
     // various rendering methods
     mFrustum.update(pSceneData->projection * pSceneData->view);
     // TEST if needed
     mFrustum.normalize();
     mTerrain.update(mFrustum);

    // ?? elapsed time should already be in ms
    stats.sceneUpdateTime = mTimer.elapsedTime();
  }


  // ?? TODO do we need camera position
  void FcRenderer::drawBillboards(glm::vec3 cameraPosition, uint32_t swapchainImageIndex, SceneData& ubo)
  {
    std::vector<FcBillboard* >& billboards = FcLocator::Billboards();

    // sort the billboards by distance to the camera
    // TODO sort within update instead
    std::multimap<float, size_t> sortedIndices; // TODO?? uint32 or size_t
    for (size_t i = 0; i < billboards.size(); ++i)
    {
      // calculate distance
      auto distance = ubo.eye - billboards[i]->PushComponent().position;
      float distanceSquared = glm::dot(distance, distance);
      sortedIndices.insert(std::pair(distanceSquared, i));
    }

    VkCommandBuffer currCommandBuffer = getCurrentFrame().commandBuffer;

    // bind pipeline to be used in render pass
    mBillboardPipeline.bind(currCommandBuffer);
    // DRAW ALL UI COMPONENTS (LAST)
    // draw text box

    // iterate through billboards in reverse order (to draw them back to front)
    for (auto index = sortedIndices.rbegin(); index != sortedIndices.rend(); ++index )
    {
      vkCmdPushConstants(currCommandBuffer, mBillboardPipeline.Layout()
                         , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BillboardPushComponent)
                         , &billboards[index->second]->PushComponent());

      //VkDeviceSize offsets[] = { 0 };
      // vkCmdBindVertexBuffers(mCommandBuffers[swapChainImageIndex], 0, 1, &font.VertexBuffer(), offsets);
      // vkCmdBindIndexBuffer(mCommandBuffers[swapChainImageIndex], font.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

      FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

      // TODO  update the global Ubo only once per frame, not each draw call
      // descClerk.update(swapchainImageIndex, &ubo);

      std::array<VkDescriptorSet, 2> descriptorSets;
      descriptorSets[0] =  mFrames[swapchainImageIndex].sceneDataDescriptorSet;
      descriptorSets[1] = billboards[index->second]->getDescriptor();

      vkCmdBindDescriptorSets(currCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS
                              , mBillboardPipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
                              , descriptorSets.data() , 0, nullptr);

      //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
      vkCmdDraw(currCommandBuffer, 6, 1, 0, 0);
    }
  }


  void FcRenderer::drawUI(std::vector<FcText>& UIelements, uint32_t swapchainImageIndex)
  {
    // mUIrenderer.draw(UIelements, getCurrentFrame().commandBuffer, swapchainImageIndex);

    VkCommandBuffer currCommandBuffer = getCurrentFrame().commandBuffer;
    // bind pipeline to be used in render pass
    mUiPipeline.bind(currCommandBuffer);
    // DRAW ALL UI COMPONENTS (LAST)
    // draw text box
    for (size_t i = 0; i < UIelements.size(); i++)
    {
      vkCmdPushConstants(currCommandBuffer, mUiPipeline.Layout()
                         , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BillboardPushComponent), &UIelements[i].Push());

      VkDeviceSize offsets[] = { 0 };
      // vkCmdBindVertexBuffers(mCommandBuffers[swapChainImageIndex], 0, 1, &font.VertexBuffer(), offsets);
      // vkCmdBindIndexBuffer(mCommandBuffers[swapChainImageIndex], font.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

      // TODO don't update UBO unless changed
      FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

      //descriptors.update(swapchainImageIndex, &mBillboardUbo);

      std::array<VkDescriptorSet, 2> descriptorSets;
      descriptorSets[0] = mFrames[swapchainImageIndex].sceneDataDescriptorSet;
      descriptorSets[1] = UIelements[i].getDescriptor();


      vkCmdBindDescriptorSets(currCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS
                              , mUiPipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
                              , descriptorSets.data() , 0, nullptr);

      //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
      vkCmdDraw(currCommandBuffer, 6, 1, 0, 0);
    }
  }



  //
  void FcRenderer::drawBackground(ComputePushConstants& pushConstants)
  {
    // VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

    // // transition our main draw image into general layout so we can write into it
    // mDrawImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // // TODO pDrawPipeline deleted for now, encapsolate this functionality
    // //vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pDrawPipeline);

    // // bind the descriptorClerk set containing the draw image for the compute
    // // pipeline
    // FcDescriptorClerk& descriptorClerk = FcLocator::DescriptorClerk();

    // //		drawImGui(cmd,
    // //mSwapchain.getFcImage(swapchainImgIndex).ImageView());
    // std::array<VkDescriptorSet, 1> ds{mDrawImageDescriptor};

    // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE,
    //                         pDrawPipelineLayout, 0, 1, ds.data(), 0, nullptr);

    // vkCmdPushConstants(cmd, pDrawPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0,
    //                    sizeof(ComputePushConstants), &pushConstants);

    // // execute the compute pipeline dispatch.
    // vkCmdDispatch(cmd, std::ceil(mDrawImage.Width() / 16.0),
    //               std::ceil(mDrawImage.Height() / 16.0), 1);
  }


  void FcRenderer::initNormalDrawPipeline(FcBuffer& sceneDataBuffer)
  {
    FcPipelineConfig pipelineConfig{3};
    pipelineConfig.name = "Normal Draw Pipeline";
    pipelineConfig.shaders[0].filename = "normal_display.vert.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineConfig.shaders[1].filename = "normal_display.geom.spv";
    pipelineConfig.shaders[1].stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
    pipelineConfig.shaders[2].filename = "normal_display.frag.spv";
    pipelineConfig.shaders[2].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    // add push constants
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(DrawPushConstants);

    pipelineConfig.addPushConstants(matrixRange);

    pipelineConfig.addDescriptorSetLayout(mSceneDataDescriptorLayout);

    pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    // TODO front face
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_AND_BACK, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);
    // TODO prefer config via:
    //pipelineConfig.enableMultiSampling(VK_SAMPLE_COUNT_1_BIT);
    //pipelineConfig.disableMultiSampling();

    pipelineConfig.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    mNormalDrawPipeline.create(pipelineConfig);
  }



    void FcRenderer::initBoundingBoxPipeline(FcBuffer& sceneDataBuffer)
  {
    FcPipelineConfig pipelineConfig{3};
    pipelineConfig.name = "Bounding Box Draw";
    pipelineConfig.shaders[0].filename = "bounding_box.vert.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineConfig.shaders[1].filename = "bounding_box.geom.spv";
    pipelineConfig.shaders[1].stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
    pipelineConfig.shaders[2].filename = "bounding_box.frag.spv";
    pipelineConfig.shaders[2].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    // add push constants
    VkPushConstantRange pushConstantRange;
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BoundingBoxPushConstants);

    pipelineConfig.addPushConstants(pushConstantRange);

    pipelineConfig.addDescriptorSetLayout(mSceneDataDescriptorLayout);

    pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    // ?? Would be better to implement with line primitives but not sure if all implementations
    // can use lines... triangles are pretty much guaranteed
    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineConfig.setCullMode(VK_CULL_MODE_FRONT_AND_BACK, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);
    pipelineConfig.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.disableBlending();

    mBoundingBoxPipeline.create(pipelineConfig);
  }


  void FcRenderer::drawGeometry()
  {
    // TODO should consider sorting outside the drawGeometry perhaps, unless something changes
    // or perhaps just inserting objects into draw via a hashmap. One thing to consider though
    // is that we also perform visibility checks before we sort
    // TODO should also make sure to sort using more than one thread
    // Sort rendered objects according to material type and if the same sorted by indexBuffer
    // A lot of big game engines do this to reduce the number of pipeline/descriptor set binds
    std::vector<uint32_t> sortedOpaqueIndices;
    sortedOpaqueIndices.reserve(mainDrawContext.opaqueSurfaces.size());

    // Only place the meshes whose bounding box is within the view frustrum
    for (uint32_t i = 0; i < mainDrawContext.opaqueSurfaces.size(); i++)
    {
      // BUG the bounding boxes are excluding visible objects for some reason
      // May only be on the sponza gltf...
      // BUG also when no objects are rendered, causes an error in seting the scissors and viewport
      // TODO implement normal arrows and bounding boxes
      // if (mainDrawContext.opaqueSurfaces[i].isVisible(pSceneData->viewProj))
      // {
      //   sortedOpaqueIndices.push_back(i);
      // }
      sortedOpaqueIndices.push_back(i);

    }

    // ?? couldn't we sort drawn meshes into a set of vectors that're already sorted by material
    // and keep drawn object in linked list every iteration (unless removed manually) instead
    // of clearing the draw list every update...

    // TODO sort algorithm could be improved by calculating a sort key, and then our sortedOpaqueIndices
    // would be something like 20bits draw index and 44 bits for sort key/hash
    // sort the opaque surfaces by material and mesh
    std::sort(sortedOpaqueIndices.begin(), sortedOpaqueIndices.end(), [&](const auto& iA, const auto& iB)
     {
       const RenderObject& A = mainDrawContext.opaqueSurfaces[iA];
       const RenderObject& B = mainDrawContext.opaqueSurfaces[iB];
       if (A.material == B.material)
       {
         return A.indexBuffer < B.indexBuffer;
       }
       else
       {
         return A.material < B.material;
       }
     });


    // reset counters
    stats.objectsRendered = 0;
    stats.triangleCount = 0;
    // begin clock
    mTimer.start();

    VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

    //std::cout << "drawExtent: " << mDrawExtent.width << " x " << mDrawExtent.height << std::endl;
    // transition draw image from compute shader write optimal to best format
    // for graphics pipeline writeable
    mDrawImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    //
    mDepthImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

    // TODO extract into builder...
    // begin a render pass connected to our draw image
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = mDrawImage.ImageView();
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR :
    //VK_ATTACHMENT_LOAD_OP_LOAD;

    // TODO Should use LOAD_OP_DONT_CARE here if we know the entire image will be written over
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
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

    // Reset the previously used draw instruments for the new draw call
    // defined outside of the draw function, this is the state we will try to skip
    lastUsedPipeline = nullptr;
    lastUsedMaterial = nullptr;
    lastUsedIndexBuffer = VK_NULL_HANDLE;

    // First draw the opaque objects using the captured Lambda

    for (uint32_t surfaceIndex : sortedOpaqueIndices)
    {
      // if (surfaceIndex == 12)
      // {
      drawSurface(cmd, mainDrawContext.opaqueSurfaces[surfaceIndex]);
      // }
    }


    // Afterwards, we can draw the transparent ones using the captured Lambda
    for (RenderObject& surface : mainDrawContext.transparentSurfaces)
    {
      drawSurface(cmd, surface);
    }

    // Draw the bounding box around the object if enabled
    if (mDrawBoundingBoxes)
    {
      // Draw all bounding boxes if signaled (default value = -1)
      if (mBoundingBoxId < 0)
      {
        for (RenderObject& surface : mainDrawContext.opaqueSurfaces)
        {
          drawBoundingBox(cmd, surface);
        }
      }
      else // otherwise, just draw the object that we are told to
      {
        // make sure we don't try and draw a bounding box that doesn't exist
        if (mBoundingBoxId >= mainDrawContext.opaqueSurfaces.size())
        {
          mBoundingBoxId = -1;
        }
        else
        {
          drawBoundingBox(cmd, mainDrawContext.opaqueSurfaces[mBoundingBoxId]);
        }
      }
    }

    // // Finally draw the Normals for the opaque objects
    if (mDrawNormalVectors)
    {
      for (uint32_t& surfaceIndex : sortedOpaqueIndices)
      {
        drawNormals(cmd, mainDrawContext.opaqueSurfaces[surfaceIndex]);
      }
    }

    mSkybox.draw(cmd, &getCurrentFrame().sceneDataDescriptorSet);

    mTerrain.draw(cmd, pSceneData, drawWireframe);

    vkCmdEndRendering(cmd);

    // ?? elapsed time should already be in ms
    stats.meshDrawTime = mTimer.elapsedTime();
  }


  void FcRenderer::drawShadowMap(bool drawDebug)
  {
    VkCommandBuffer cmd = getCurrentFrame().commandBuffer;
    //loadedScenes["structure"]->draw(glm::mat4{1.f}, mainDrawContext);
    mShadowMap.generateMap(cmd, mainDrawContext);

    //std::cout << "drawExtent: " << mDrawExtent.width << " x " << mDrawExtent.height << std::endl;
    // transition draw image from compute shader write optimal to best format
    // for graphics pipeline writeable
    mDrawImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    //
    // mDepthImage.transitionImage(cmd, VK_IMAGE_LAYOUT_UNDEFINED,
    //                             VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

    // TODO extract into builder...
    // begin a render pass connected to our draw image
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = mDrawImage.ImageView();
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR :
    //VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;


    //
    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, mDrawExtent};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pStencilAttachment = nullptr;

   vkCmdBeginRendering(cmd, &renderInfo);

    vkCmdSetViewport(cmd, 0, 1, &mDynamicViewport);
    vkCmdSetScissor(cmd, 0, 1, &mDynamicScissors);

    // draw the quad that we'll map the shadow map to
    if (drawDebug)
    {
      mShadowMap.drawDebugMap(cmd);
      vkCmdEndRendering(cmd);
    }
    else
    {
      vkCmdEndRendering(cmd);
      drawGeometry();
    }
  }



  void FcRenderer::drawSurface(VkCommandBuffer cmd, const RenderObject& surface)
  {

    if (surface.material != lastUsedMaterial)
    {
      lastUsedMaterial = surface.material;

      // Only rebind pipeline and material descriptors if the material changed
      // TODO have each object track state of its own descriptorSets
      if (surface.material->pPipeline != lastUsedPipeline)
      {
        lastUsedPipeline = surface.material->pPipeline;

        surface.bindPipeline(cmd);
        surface.bindDescriptorSet(cmd, getCurrentFrame().sceneDataDescriptorSet, 0);
        surface.bindDescriptorSet(cmd, mSkybox.Descriptor(), 1);
        surface.bindDescriptorSet(cmd, mShadowMap.Descriptor(), 2);
      }

      surface.bindDescriptorSet(cmd, surface.material->materialSet, 3);
    }

    // Only bind index buffer if it has changed
    if (surface.indexBuffer != lastUsedIndexBuffer)
    {
      lastUsedIndexBuffer = surface.indexBuffer;
      surface.bindIndexBuffer(cmd);
    }

    // Calculate final mesh matrix
    DrawPushConstants pushConstants;
    pushConstants.vertexBuffer = surface.vertexBufferAddress;
    pushConstants.worldMatrix = surface.transform;
    pushConstants.normalTransform = surface.invModelMatrix;

    //
    vkCmdPushConstants(cmd, surface.material->pPipeline->Layout()
                       , VK_SHADER_STAGE_VERTEX_BIT
                       , 0, sizeof(DrawPushConstants), &pushConstants);
    // Note here that we have to offset from the initially pushed data since we
    // are really just filling a range alloted to us in total...
    vkCmdPushConstants(cmd, surface.material->pPipeline->Layout()
                       , VK_SHADER_STAGE_GEOMETRY_BIT
                       , sizeof(DrawPushConstants), sizeof(float), &expansionFactor);

    vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);

    // add counters for triangles and draws calls
    stats.objectsRendered++;
    stats.triangleCount += surface.indexCount / 3;
  }



  void FcRenderer::drawNormals(VkCommandBuffer cmd, const RenderObject& surface)
  {

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mNormalDrawPipeline.getVkPipeline());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mNormalDrawPipeline.Layout()
                            , 0, 1, &getCurrentFrame().sceneDataDescriptorSet, 0, nullptr);

    surface.bindIndexBuffer(cmd);

    // Calculate final mesh matrix
    DrawPushConstants pushConstants;
    pushConstants.vertexBuffer = surface.vertexBufferAddress;
    pushConstants.worldMatrix = surface.transform;
    pushConstants.normalTransform = surface.invModelMatrix;

    vkCmdPushConstants(cmd, mNormalDrawPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                       , 0, sizeof(DrawPushConstants), &pushConstants);

    vkCmdDrawIndexed(cmd, surface.indexCount, 1, surface.firstIndex, 0, 0);
  }



  void FcRenderer::drawBoundingBox(VkCommandBuffer cmd, const RenderObject& surface)
  {
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mBoundingBoxPipeline.getVkPipeline());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mBoundingBoxPipeline.Layout()
                            , 0, 1, &getCurrentFrame().sceneDataDescriptorSet, 0, nullptr);

    // Send the bounding box to the shaders
    BoundingBoxPushConstants pushConstants;
    pushConstants.modelMatrix = surface.transform;
    pushConstants.origin = glm::vec4(surface.bounds.origin, 1.f);
    pushConstants.extents = glm::vec4(surface.bounds.extents, 0.f);

    vkCmdPushConstants(cmd, mBoundingBoxPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                       , 0, sizeof(BoundingBoxPushConstants), &pushConstants);

    // TODO update to utilize sascha method for quads
    vkCmdDraw(cmd, 36, 1, 0, 0);
  }


  void FcRenderer::drawImGui(VkCommandBuffer cmd, VkImageView targetImageView)
  {
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = targetImageView;
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    //		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // if (clear)
    // 	{
    // 		colorAttachment.clearValue = *clear;
    // 	}

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

    for (FrameData &frame : mFrames)
    {
      // create 2 semaphores (one tells us the image is ready to draw to and one tells us when we're done drawing)
      if (vkCreateSemaphore(pDevice, &semaphoreInfo, nullptr, &frame.imageAvailableSemaphore) != VK_SUCCESS
          || vkCreateSemaphore(pDevice, &semaphoreInfo, nullptr, &frame.renderFinishedSemaphore) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create a Vulkan sychronization Semaphore!");
      }
      // create the fence that makes sure the draw commands of a a given frame is finished

      if (vkCreateFence(pDevice, &fenceInfo, nullptr, &frame.renderFence) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create a Vulkan sychronization Fence!");
      }
    }

    // -*-*-*-*-*-*-*-*-*-*-*-*-   IMMEDIATE COMMAND SYNC   -*-*-*-*-*-*-*-*-*-*-*-*- //
    if (vkCreateFence(pDevice, &fenceInfo, nullptr, &mImmediateFence) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan sychronization Fence!");
    }

  } // --- FcRenderer::createSynchronization (_) --- (END)



  uint32_t FcRenderer::beginFrame()
  {
    // call to update scene immediately (before waiting on fences)
    updateScene();

    // TODO put all ImGui stuff into renderer and maybe its own class
    // update ImGui
    // ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // TODO define in constants
    uint64_t maxWaitTime = std::numeric_limits<uint64_t>::max();


    // don't keep adding images to the queue or commands to the buffer until last draw has finished
    vkWaitForFences(pDevice, 1, &getCurrentFrame().renderFence, VK_TRUE, maxWaitTime);

    // delete any per frame resources no longer needed now the that frame has finished rendering
    // ?? this seems to be the wrong location for this, just by observation: test

//    getCurrentFrame().janitor.flush();

    // 1. get the next available image to draw to and set to signal the semaphore when we're finished with it
    uint32_t swapchainImageIndex;
    VkResult result = vkAcquireNextImageKHR(pDevice, mSwapchain.vkSwapchain(), maxWaitTime
                                            , getCurrentFrame().imageAvailableSemaphore
                                            , VK_NULL_HANDLE, &swapchainImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
      fcLog("ERRROR OUT of date submit1");
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

    if (vkBeginCommandBuffer(commandBuffer, &bufferBeginInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to start recording a Vulkan Command Buffer!");
    }


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
    mDrawImage.transitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    // transiton the swapchain so it can best accept an image being copied to it
    mSwapchain.transitionImage(commandBuffer, swapchainImageIndex
                               , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // execute a copy from the draw image into the swapchain
    mSwapchain.getFcImage(swapchainImageIndex).copyFromImage(commandBuffer, &mDrawImage);


    // TODO ?? Note that drawImGui can also go here so should find out what they do in the examples

    // now transition swapchain image layout to attachment optimal so we can draw into it
    mSwapchain.transitionImage(commandBuffer, swapchainImageIndex
                               , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
                               , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    //
    drawImGui(commandBuffer, mSwapchain.getFcImage(swapchainImageIndex).ImageView());

    // finally transition the swapchain image into presentable layout so we can present to surface
    mSwapchain.transitionImage(commandBuffer, swapchainImageIndex
                               , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // stop recording to command buffer
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to stop recording a Vulkan Command Buffer!");
    }

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
    if (vkQueueSubmit2(mGpu.graphicsQueue(), 1, &submitInfo, getCurrentFrame().renderFence) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to submit Vulkan Command Buffer to the queue!");
    }

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
      fcLog("ERRROR OUT of date submit");
      mShouldWindowResize = true;
      //mWindow.resetWindowResizedFlag();
      //handleWindowResize();
    }
    else if (result != VK_SUCCESS)
    {
      throw std::runtime_error("Faled to submit image to Vulkan Present Queue!");
    }

    // get next frame (use % MAX_FRAME_DRAWS to keep value below the number of frames we have in flight
    // increase the number of frames drawn
    mFrameNumber++;
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
    std::cout << "calling: FcRenderer::shutDown" << std::endl;
    // wait until no actions being run on device before destroying
    vkDeviceWaitIdle(pDevice);


    //loadedScenes.clear();
    structure.clearAll();

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULTS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    vkDestroySampler(pDevice, mDefaultSamplerLinear, nullptr);
    vkDestroySampler(pDevice, mDefaultSamplerNearest, nullptr);

    mWhiteTexture.destroy();
    mGreyTexture.destroy();
    mBlackTexture.destroy();
    mCheckerboardTexture.destroy();

//mUiRenderer.destroy();

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   SCENE DATA   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    mMetalRoughMaterial.clearResources(pDevice);
    vkDestroyDescriptorSetLayout(pDevice, mSceneDataDescriptorLayout, nullptr);
    vkDestroyDescriptorSetLayout(pDevice, mBackgroundDescriptorlayout, nullptr);

    // TODO should think about locating mImgGui into Descriptor Clerk
    FcLocator::DescriptorClerk().destroy();

    // close imGui
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(pDevice, mImgGuiDescriptorPool, nullptr);

    // ?? don't think that the reference is needed here
    for (FrameData &frame : mFrames)
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
    mModelPipeline.destroy();
    mBillboardPipeline.destroy();
    mUiPipeline.destroy();

    mSwapchain.destroy();

    mDrawImage.destroy();
    mDepthImage.destroy();

    mTestMeshes.destroy();

    mGpu.release(mInstance);

    mWindow.close(mInstance);

    if (mInstance != nullptr)
    {
      vkDestroyInstance(mInstance, nullptr);
    }
    if (enableValidationLayers)
    {
      DestroyDebugUtilsMessengerExt(mInstance, debugMessenger, nullptr);
    }
  }


} // END namespace fc
