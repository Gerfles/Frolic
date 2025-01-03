#include "fc_renderer.hpp"


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "SDL2/SDL_stdinc.h"
#include "core/fc_billboard_render_system.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_font.hpp"
#include "core/fc_game_object.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_model_render_system.hpp"
#include "core/fc_pipeline.hpp"
#include "core/fc_swapChain.hpp"
#include "core/fc_mesh.hpp"
#include "core/platform.hpp"
#include "utilities.hpp"
#include "fc_debug.hpp"
#include "fc_camera.hpp"
#include <cstdint>
#include <exception>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <mutex>
#include <string>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <SDL_events.h>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
#include "vulkan/vulkan_core.h"
#include <SDL2/SDL_vulkan.h>
// ImGUI
#include "imgui.h"
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <cstdlib>
#include <stdexcept>
#include <unordered_set>
#include <vector>
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
      std::cout << "window dimensions88: " << mWindow.ScreenSize().width
                << " x " << mWindow.ScreenSize().height << std::endl;

       // retrieve the physical device then create the logical device to interface with GPU & command pool
      if ( !mGpu.init(mInstance, mWindow))
      {
        throw std::runtime_error("Failed to initialize GPU device!");
      }

      FcLocator::provide(&mGpu);
      FcLocator::provide(this);

      pDevice = FcLocator::Gpu().getVkDevice();

       // create the swapchain & renderpass & frambuffers & depth buffer
      mBufferCount = mSwapchain.init(mGpu, mWindow.ScreenSize());

      initDrawImage();

       // create the command pool for later allocating command from. Also create the command buffers
      createCommandPools();

       // initialze the dynamic viewport and scissors
      mDynamicViewport.x = 0.0f;
      mDynamicViewport.y = 0.0f;
      mDynamicViewport.minDepth = 0.0f;
      mDynamicViewport.maxDepth = 1.0f;
       //
      mDynamicScissors.offset = {0, 0};

       // create the graphics pipeline && create/attach descriptors
       // create the uniform buffers & initialize the descriptor sets that tell the pipeline about our uniform buffers
       // here we want to create a descriptor set for each swapchain image we have
       // TODO rename from create to init maybe
       // TODO determine is descriptorclerk should be a local variable or heap variable as is
      FcDescriptor* descriptorClerk = new FcDescriptor;
      descriptorClerk->create(&mGpu, mBufferCount);
       // register the descriptor with the locator
      FcLocator::provide(descriptorClerk);

       // create all of the standard pipelines that the renderer relies on
       // TODO not sure if we actually need a member modedlRenderer or if we can delete after creation
       // TODO standardize all pipelines and draw calls
      mModelRenderer.createPipeline(mGpu, mModelPipeline, *descriptorClerk, mSwapchain.getRenderPass());
      mBillboardRenderer.createPipeline(mBillboardPipeline, mSwapchain.getRenderPass());
      mUiRenderer.createPipeline(mUiPipeline, mSwapchain.getRenderPass());

       // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

      std::vector<PoolSizeRatio> sizes = { {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1} };
      descriptorClerk->createDescriptorPool2(10, sizes);

      descriptorClerk->addBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT);
      descriptorClerk->createDescriptorSetLayout2();
      descriptorClerk->createDescriptorSets2(mDrawImage);



       // // Make sure to initialize the FcPipelineCreateInfo with the number of stages we want
       // FcPipelineCreateInfo gradient{1};
       // gradient.name = "gradient";
       // gradient.data = {};
       // gradient.shaders[0].filename = "gradient_color.comp.spv";
       // gradient.shaders[0].stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;

       // mGradientPipeline.create2(&gradient);

       // FcPipelineCreateInfo sky{1};
       // sky.name = "sky";
       // sky.data.data1 = glm::vec4{0.1, 0.2, 0.4, 0.97};
       // sky.shaders[0].filename = "sky.comp.spv";
       // sky.shaders[0].stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;

       // mSkyPipeline.create2(&sky);

       // // TODO delete
       // mPipelines.push_back(&mGradientPipeline);
       // mPipelines.push_back(&mSkyPipeline);



       // computeShader.stage = VK_SHADER_STAGE_COMPUTE_BIT;
       // computeShader.filename = "gradient_color.comp.spv";

       // load the only stage into a vector since this requires that TODO allow compute shaders to be created differntly
       //std::vector<ShaderInfo> computeShaderInfos {computeShader};



      initImgui();
       // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

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


       // FcModel model;
       // model.createModel("models/smooth_vase.obj", mPipeline, mGpu);
       // mModelList.push_back(model);

      initDefaults();


    }
    catch (const std::runtime_error& err) {
      printf("ERROR: %s\n", err.what());
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
  }



  void FcRenderer::initDefaults()
  {
     //testModel.loadGltfMeshes("..\\..\\models\\basicmesh.glb");
    testModel.loadGltfMeshes("..//models//basicmesh.glb");
  }



  void FcRenderer::initImgui()
  {
     // Create the descriptor pool for IMGUI
     // MASSIVELY oversized, doesn't seem to be in any imgui demo that was mentioned
     // TODO /TRY try and follow this up by recreating the example from ImgGUI site
    VkDescriptorPoolSize poolSizes[] = {
       // {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
       // {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
       // {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
       // {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1},
      // {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      // {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      // {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      // {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      // {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      // {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000}
		};

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
    imGuiInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&imGuiInfo))
			{
				throw std::runtime_error("Failed to initialize ImGui within Vulkan");
			}

    if (!ImGui_ImplVulkan_CreateFontsTexture())
			{
				throw std::runtime_error("Failed to create ImGui Fonts");
			}
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
     // TODO create a constructor from VkExtent2D to VkExtent3D
    VkExtent3D drawImgExtent = {mWindow.ScreenSize().width, mWindow.ScreenSize().height, 1};

    VkImageUsageFlags imgUse{};
     // We plan on copying into but also from the image
    imgUse = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imgUse |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
     // Storage bit allows computer shader to write to image
    imgUse |= VK_IMAGE_USAGE_STORAGE_BIT;
     // Color attachment allows graphics pipelines to draw geometry into it
    imgUse |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

     // hardcoding the draw format to 32 bit float
    mDrawImage.create(drawImgExtent
                      , VK_FORMAT_R16G16B16A16_SFLOAT
                      , VK_SAMPLE_COUNT_1_BIT
                      , imgUse
                      , VK_IMAGE_ASPECT_COLOR_BIT);

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE DEPTH IMAGE   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    imgUse = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    mDepthImage.create(drawImgExtent, VK_FORMAT_D32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, imgUse, VK_IMAGE_ASPECT_DEPTH_BIT);
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
	// TODO SHOULD speed this up would be to run it on a different queue than the graphics queue so
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

     // Queue submission information
     // VkSubmitInfo submitInfo{};
     // submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
     // submitInfo.commandBufferCount = 1;
     // submitInfo.pCommandBuffers = &commandBuffer;

     // Submit the command buffer to the queue
     // TODO assert, don't check like this
    if (vkQueueSubmit2(mGpu.graphicsQueue(), 1, &submitInfo, mImmediateFence) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to submit Vulkan Command Buffer to the queue!");
    }

    vkWaitForFences(pDevice, 1, &mImmediateFence, true, u64_max);
  }


	//TODO change current frame to currentFrameBuffer
  void FcRenderer::drawModels(uint32_t swapchainImageIndex, GlobalUbo& ubo)
  {
     //
    auto gameObjects = FcLocator::GameObjects();

     // bind pipeline to be used in render pass
    mModelPipeline.bind(getCurrentFrame().commandBuffer);

     // now make sure to record commands for all of our objects (meshes)
    for (size_t i = 0; i < gameObjects.size(); ++i)
    {
      auto currModel = gameObjects[i]->pModel;

       // draw only the game objects that have an associated model
      if (gameObjects[i]->pModel != nullptr)
      {
        ModelPushConstantData push{};
        push.modelMatrix = gameObjects[i]->transform.mat4();
        push.normalMatrix = gameObjects[i]->transform.normalMatrix();

        vkCmdPushConstants(getCurrentFrame().commandBuffer, mModelPipeline.Layout()
                           , VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
                           , 0, sizeof(ModelPushConstantData), &push);

        for (size_t j = 0; j < currModel->MeshCount(); ++j)
        {
          FcMesh& currMesh = currModel->Mesh(j);

           //VkBuffer vertexBuffers[] = { currMesh.VertexBuffer() };// buffers to bind
          VkDeviceSize offsets[] = { 0 }; // offsets into buffers being bound

           // Bind all the necessary resources to our pipeline
           //SDL_Log("%d times ", currMesh.VertexCount());
           // bind vertex buffer before drawing
          vkCmdBindVertexBuffers(getCurrentFrame().commandBuffer, 0, 1, &currModel->Mesh(j).VertexBuffer(), offsets);
          vkCmdBindIndexBuffer(getCurrentFrame().commandBuffer, currMesh.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

          // dynamic offset ammount (SAVED FOR REFERENCE)
           // uint32_t dynamicOffset = static_cast<uint32_t>(mPipeline.ModelUniformAlignment()) * j;
           // vkCmdBindDescriptorSets(mCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS
           // , mPipeline.Layout(), 0, 1, mPipeline.DescriptorSet(currentFrame), 1, &dynamicOffset);

           // TODO don't update UBO unless changed (may still need to do just because of frame set)
          FcDescriptorClerk& descriptorClerk = FcLocator::DescriptorClerk();

          descriptorClerk.update(swapchainImageIndex, &ubo);

           // Bind descriptor sets
          std::array<VkDescriptorSet, 2> descriptorSets = { descriptorClerk.UboDescriptorSet(swapchainImageIndex)
                                                          , descriptorClerk.SamplerDescriptorSet(currMesh.DescriptorId()) };

          vkCmdBindDescriptorSets(getCurrentFrame().commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS
                                  , mModelPipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
                                  , descriptorSets.data() , 0, nullptr);

           // execute pipeline
          vkCmdDrawIndexed(getCurrentFrame().commandBuffer, currMesh.IndexCount(), 1, 0, 0, 0);
        }
      }

    } // _END_ FOR draw models in model list
  } // --- FcRenderer::drawModels (_) --- (END)


	// ?? TODO do we need camera position
  void FcRenderer::drawBillboards(glm::vec3 cameraPosition, uint32_t swapchainImageIndex, GlobalUbo& ubo)
  {
    std::vector<FcBillboard* >& billboards = FcLocator::Billboards();

     // std::cout << "billboards size: " << billboards.size() << std::endl;

     // sort the billboards by distance to the camera
     // TODO sort within update instead
    std::multimap<float, size_t> sorted; // TODO?? uint32 or size_t
    for (size_t i = 0; i < billboards.size(); ++i)
    {
       // calculate distance
       // TODO check that distance is computed correctly with the w component being 1
      auto distance = glm::vec4(cameraPosition, 1.f) - billboards[i]->PushComponent().position;
      float distanceSquared = glm::dot(distance, distance);
      sorted.insert(std::pair(distanceSquared, i));
       //billboards[i]->Push().color =
    }

    VkCommandBuffer currCommandBuffer = getCurrentFrame().commandBuffer;

     // bind pipeline to be used in render pass
    mBillboardPipeline.bind(currCommandBuffer);
     // DRAW ALL UI COMPONENTS (LAST)
     // draw text box

     // iterate through billboards in reverse order (to draw them back to front)
     //    std::cout << "sorted size : " << sorted.size() << std::endl;
    for (auto it = sorted.rbegin(); it != sorted.rend(); ++it )
    {
       // glm::vec4 col = billboards[it->second]->Push().color;
       // printVec(col, "color");


      vkCmdPushConstants(currCommandBuffer, mBillboardPipeline.Layout()
                         , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BillboardPushComponent)
                         , &billboards[it->second]->PushComponent());

      VkDeviceSize offsets[] = { 0 };
       // vkCmdBindVertexBuffers(mCommandBuffers[swapChainImageIndex], 0, 1, &font.VertexBuffer(), offsets);
       // vkCmdBindIndexBuffer(mCommandBuffers[swapChainImageIndex], font.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

       // TODO don't update UBO unless changed
      FcDescriptorClerk& descriptors = FcLocator::DescriptorClerk();


       // TODO  update the global Ubo only once per frame, not each draw call
      descriptors.update(swapchainImageIndex, &ubo);

      std::array<VkDescriptorSet, 2> descriptorSets = { descriptors.UboDescriptorSet(swapchainImageIndex)
                                                      , descriptors.SamplerDescriptorSet(billboards[it->second]->TextureId()) };
       //, descriptors.SamplerDescriptorSet(loopCount) };

      vkCmdBindDescriptorSets(currCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS
                              , mBillboardPipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
                              , descriptorSets.data() , 0, nullptr);

       //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
       //TODO find out if there's a better way to dispatch all draws simultaneously
      vkCmdDraw(currCommandBuffer, 6, 1, 0, 0);
    }




     // bind pipeline to be used in render pass
     // mBillboardPipeline.bind(getCurrentFrame().commandBuffer);
     //  // DRAW ALL UI COMPONENTS (LAST)
     //  // draw text box
     // vkCmdPushConstants(getCurrentFrame().commandBuffer, mBillboardPipeline.Layout()
     //                    , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BillboardPushConstants), &font.Push());

     // VkDeviceSize offsets[] = { 0 };
     // // vkCmdBindVertexBuffers(mCommandBuffers[swapChainImageIndex], 0, 1, &font.VertexBuffer(), offsets);
     // // vkCmdBindIndexBuffer(mCommandBuffers[swapChainImageIndex], font.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

     //      // TODO don't update UBO unless changed
     // mDescriptorManager.update(swapchainImageIndex, &mBillboardUbo);

     // std::array<VkDescriptorSet, 2> descriptorSets = { mDescriptorManager.UboDescriptorSet(swapchainImageIndex)
     //                                                 , mDescriptorManager.SamplerDescriptorSet(font.TextureId()) };

     // vkCmdBindDescriptorSets(getCurrentFrame().commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS
     //                         , mBillboardPipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
     //                         , descriptorSets.data() , 0, nullptr);

     //  //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
     // vkCmdDraw(getCurrentFrame().commandBuffer, 6, 1, 0, 0);



     //    mBillboardRenderer.draw(cameraPosition, getCurrentFrame().commandBuffer, swapchainImageIndex);
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
      FcDescriptor& descriptors = FcLocator::DescriptorClerk();

       //descriptors.update(swapchainImageIndex, &mBillboardUbo);

      std::array<VkDescriptorSet, 2> descriptorSets = { descriptors.UboDescriptorSet(swapchainImageIndex)
                                                      , descriptors.SamplerDescriptorSet(UIelements[i].TextureId()) };

      vkCmdBindDescriptorSets(currCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS
                              , mUiPipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
                              , descriptorSets.data() , 0, nullptr);

       //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
      vkCmdDraw(currCommandBuffer, 6, 1, 0, 0);
    }
  }

  void FcRenderer::attachPipeline(FcPipeline *pipeline)
  {
    pDrawPipeline = pipeline->getVkPipeline();
    pDrawPipelineLayout = pipeline->Layout();
  }


  void FcRenderer::drawSimple(ComputePushConstants& pushConstants)
  {

    VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

     // transition our main draw image into general layout so we can write into it
    mDrawImage.transitionImage(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

    // VkImageSubresourceRange clearRange {};
    // clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    // clearRange.baseMipLevel = 0;
    // clearRange.levelCount = VK_REMAINING_MIP_LEVELS;
    // clearRange.baseArrayLayer = 0;
    // clearRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    //  //make a clear-color from frame number. This will flash with a 120 frame period.
    // VkClearColorValue clearValue;
    // float flash = std::abs(std::sin(mFrameNumber / 60.f));
    // clearValue = { { 0.0f, 0.0f, flash, 1.0f } };

    //  //clear image
    // // vkCmdClearColorImage(cmd, mSwapchain.vkImage(swapchainImgIndex)
    // //                      , VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);
    // vkCmdClearColorImage(cmd, mDrawImage.Image(), VK_IMAGE_LAYOUT_GENERAL, &clearValue, 1, &clearRange);

    // bind the compute pipeline
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, pDrawPipeline);

		// bind the descriptor set containing the draw image for the compute pipeline
    FcDescriptor descriptor = FcLocator::DescriptorClerk();

		//		drawImGui(cmd, mSwapchain.getFcImage(swapchainImgIndex).ImageView());

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE
                            , pDrawPipelineLayout, 0, 1, descriptor.vkDescriptor(), 0, nullptr);

		// inject the push constants
		// ComputePushConstants pc;
		// pc.data1 = glm::vec4(1,0,0,1);
		// pc.data2 = glm::vec4(0,0,1,1);

    vkCmdPushConstants(cmd, pDrawPipelineLayout
                       , VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &pushConstants);

		// execute the compute pipeline dispatch.
    vkCmdDispatch(cmd, std::ceil(mDrawImage.size().width / 16.0)
                  , std::ceil(mDrawImage.size().height / 16.0), 1);
  }

  void FcRenderer::drawGeometry(FcPipeline& pipeline)
  {
    VkCommandBuffer cmd = getCurrentFrame().commandBuffer;

    mDrawExtent.height = std::min(mSwapchain.getSurfaceExtent().height, mDrawImage.getExtent().height) * renderScale;
    mDrawExtent.width = std::min(mSwapchain.getSurfaceExtent().width, mDrawImage.getExtent().width) * renderScale;

// transition draw image from computer shader write optimal to best format for graphics pipeline writeable
    mDrawImage.transitionImage(cmd,  VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
//
    mDepthImage.transitionImage(cmd, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

     // begin a render pass connected to our draw image
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.imageView = mDrawImage.ImageView();
    colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
     //		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
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
    renderInfo.renderArea = VkRect2D{ VkOffset2D{0, 0}, mDrawExtent};
    renderInfo.layerCount = 1;
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachments = &colorAttachment;
    renderInfo.pDepthAttachment = &depthAttachment;
    renderInfo.pStencilAttachment = nullptr;

    vkCmdBeginRendering(cmd, &renderInfo);

     // bind the descriptor set containing the draw image for the compute pipeline
//    FcDescriptor descriptor = FcLocator::DescriptorClerk();

     //		drawImGui(cmd, mSwapchain.getFcImage(swapchainImgIndex).ImageView());
    // vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS
    //                         , pipeline.Layout(), 0, 1, descriptor.vkDescriptor(), 0, nullptr);

     // set dynamic viewport and scissors
    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = mDrawExtent.width;
    viewport.height = mDrawExtent.height;
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissors = {};
    scissors.offset.x = 0;
    scissors.offset.y = 0;
    scissors.extent.width = mDrawExtent.width;
    scissors.extent.height = mDrawExtent.height;

    vkCmdSetScissor(cmd, 0, 1, &scissors);

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   DRAW MONKEY HEAD   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    pipeline.bind(cmd);

     // translate from camera
    glm::mat4 view = glm::translate(glm::vec3{0, 0, -5});
     // camera projeciton
    glm::mat4 projection = glm::perspective(glm::radians(70.0f)
                                            , static_cast<float>(mDrawExtent.width)
                                            / static_cast<float>(mDrawExtent.height)
                                            , 0.0f, 100.f);

     // invert the y direction on projection matrix so that openGL images match Vulkan axis
     projection[1][1] *= -1;

     drawPushConstants pushConstants;
     pushConstants.vertexBuffer = testModel.Mesh(2).VertexBufferAddress();
     pushConstants.worldMatrix = projection * view;

    vkCmdPushConstants(cmd, pipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT, 0
                       , sizeof(drawPushConstants), &pushConstants);

    vkCmdBindIndexBuffer(cmd, testModel.Mesh(2).IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(cmd, testModel.Mesh(2).IndexCount(), 1, testModel.Mesh(2).getStartIndex(), 0, 0);

    vkCmdEndRendering(cmd);
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
    getCurrentFrame().janitor.flush();

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
    mDrawImage.transitionImage(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

     // transiton the swapchain so it can best accept an image being copied to it
    mSwapchain.transitionImage(commandBuffer, swapchainImageIndex
                               , VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

     // execute a copy from the draw image into the swapchain
    mSwapchain.getFcImage(swapchainImageIndex).copyFromImage(commandBuffer, &mDrawImage, mDrawImage.size()
                                                             , mDrawImage.size());

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
     // TODO delete the following line (shouldn't need)
     // waitSemaphorInfo.deviceIndex = 0;

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




	// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   OLD METHOD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
	// void FcRenderer::endFrame(uint32_t swapchainImageIndex)
	//   {

	//     VkCommandBuffer commandBuffer = getCurrentFrame().commandBuffer;

	//      // transition the swapchain image into writeable mode before rendering
	//     mSwapchain.transitionImage(commandBuffer, swapchainImageIndex
	//                                , VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	//      // end render pass
	//     vkCmdEndRenderPass(commandBuffer);

	//      // stop recording to command buffer
	//     if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS)
	//     {
	//       throw std::runtime_error("Failed to stop recording a Vulkan Command Buffer!");
	//     }

	//      // 2. submit command buffer to queue for rendering, making sure it waits for the image to be signalled as
	//      // available before drawing and signals when it has finished rendering
	//     VkSubmitInfo submitInfo{};
	//     submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	//     submitInfo.waitSemaphoreCount = 1;                         // number of semaphores to wait on
	//     submitInfo.pWaitSemaphores = &mImageReadySemaphores[mCurrentFrame];             // list of semaphores to wait on
	//     VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	//     submitInfo.pWaitDstStageMask = waitStages;                 // stages to check semaphores at
	//     submitInfo.commandBufferCount = 1;                         // number of command buffers to submit
	//     submitInfo.pCommandBuffers = &mCommandBuffers[swapChainImageIndex]; // command buffer to submit
	//     submitInfo.signalSemaphoreCount = 1;                       // number of semaphores to signal
	//     submitInfo.pSignalSemaphores = &mRenderFinishedSemaphores[mCurrentFrame];           // semaphores to signal when command buffer finishes

	//      // Submit the command buffer to the queue
	//     if (vkQueueSubmit(mGpu.graphicsQueue(), 1, &submitInfo, mDrawFences[mCurrentFrame]) != VK_SUCCESS)
	//     {
	//       throw std::runtime_error("Failed to submit Vulkan Command Buffer to the queue!");
	//     }

	//      // 3. present image to screen when it has signalled finished rendering
	//     VkPresentInfoKHR presentInfo{};
	//     presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	//     presentInfo.waitSemaphoreCount = 1;                  // Number of semaphores to wait on
	//     presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[mCurrentFrame];      // semaphore to wait on
	//     presentInfo.swapchainCount = 1;                      // number of swapchains to present to
	//     presentInfo.pSwapchains = &mSwapchain.vkSwapchain(); // swapchain to present images to
	//     presentInfo.pImageIndices = &swapChainImageIndex;             //index of images in swapchains to present

	//     VkResult result = vkQueuePresentKHR(mGpu.presentQueue(), &presentInfo);

	//     if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mWindow.wasWindowResized())
	//     {
	//       mWindow.resetWindowResizedFlag();
	//       handleWindowResize();
	//     }
	//     else if (result != VK_SUCCESS)
	//     {
	//       throw std::runtime_error("Faled to submit image to Vulkan Present Queue!");
	//     }

	//      // get next frame (use % MAX_FRAME_DRAWS to keep value below the number of frames we have in flight
	//     mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAME_DRAWS;
	//   }


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

     //mUiRenderer.destroy();

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
