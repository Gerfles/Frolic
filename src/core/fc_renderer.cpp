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
/* #include <SDL_events.h> */
#include <SDL2/SDL_vulkan.h>
#include <SDL2/SDL_log.h>
#include <SDL_version.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // FcRenderer::FcRenderer()
  // {
  //    // TODO allow a passed struct that will initialize the window and app "stuff"
  //    // including a pointer to a VK_STRUCTURE type appinfo
  // }

  int FcRenderer::init(FcConfig& config, SceneData** pSceneData)
  {
    // TODO make this (try) more robust and add try catch blocks to all non-rendering functions
    try
    {
      // retrieve the physical device then create the logical device to interface with GPU & command pool
      if ( !mGpu.init(FcLocator::vkInstance(), config))
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

      // Used to match our draw images to the window extent
      VkExtent2D screenSize = config.getWindowPtr()->ScreenSize();

      // Initialize our draw image
      mDrawImage.createImage(screenSize.width,
                             screenSize.height, FcImageTypes::ScreenBuffer);

      // Initialize our depth image (z buffer)
      mDepthImage.createImage(screenSize.width, screenSize.height
                              ,FcImageTypes::DepthBuffer);

      // create the graphics pipeline && create/attach descriptors create the uniform
      // buffers & initialize the descriptor sets that tell the pipeline about our uniform
      // buffers here we want to create a descriptor set for each swapchain image we have
      // TODO rename from create to init maybe
      // TODO determine is descriptorclerk should be a local variable or heap variable as is

      //
      initImgui(swapchainImageFormat, config);

      // create timeline semaphore
      mTimelineSemaphore = createTimelineSemaphore(pDevice,
                                                   mSwapchain.imageCount() - 1,
                                                   "Semaphore: mTimelineSemaphore");

      //
      mImmediateCommands.init(pDevice, mGpu.getQueues().graphicsFamily, "SYNC");

      //
      FcLocator::provide(&mJanitor);

      // Initialize all the default textures/samplers/etc. to use in places where they are not imported
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


  //
  // TODO could pass pScene data here but probably better to just include this in mRenderer.init()
  void FcRenderer::initDefaults()//FcBuffer& sceneDataBuffer, SceneDataUbo* sceneData)
  {
    // TODO place in function along with other init detailed stuff
    mColorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    mColorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // using LOAD_OP_DONT_CARE here since we know the entire image will be written over
    mColorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    mColorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Depth image attachment info for drawing into the z-buffer
    mDepthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    mDepthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
    mDepthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    mDepthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    mDepthAttachment.clearValue.depthStencil.depth = 0.f;
    mDepthAttachment.imageView = mDepthImage.ImageView();

    // TODO create a function for allowing viewport and scisors to change
    // TODO have swapchain dims available locally (and different from window dims in some cases)
    // TODO provide for these to change if VK_ERROR_OUT_OF_DATE_KHR, etc.
    VkExtent2D drawExtent;
    drawExtent.height = std::min(FcLocator::ScreenDims().height, mDrawImage.Height());
    drawExtent.width = std::min(FcLocator::ScreenDims().width, mDrawImage.Width());

    //
    mRenderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    mRenderInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, drawExtent};
    mRenderInfo.layerCount = 1;
    mRenderInfo.colorAttachmentCount = 1;
    mRenderInfo.pColorAttachments = &mColorAttachment;

    // initialze the dynamic Viewport
    mDynamicViewport.x = 0;
    mDynamicViewport.y = 0;
    mDynamicViewport.width = drawExtent.width;
    mDynamicViewport.height = drawExtent.height;
    mDynamicViewport.minDepth = 0.0f;
    mDynamicViewport.maxDepth = 1.0f;

    // initialze the dynamic scissors
    mDynamicScissors.offset = {0, 0};
    mDynamicScissors.extent.width = mDynamicViewport.width;
    mDynamicScissors.extent.height = mDynamicViewport.height;

    //
    mShadowRenderer.init(mDescriptorCollection);
    mTerrain.init("..//maps/terrain_heightmap_r16.ktx2");

    // FIXME should be able to load any kind of terrain map
    /* mTerrain.init("..//maps/iceland_heightmap.png"); */;

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

    // TODO get rid of scenedescriptorLayout (I believe)
    mSceneRenderer.init(mSceneData.viewProj, mDescriptorCollection, mShadowRenderer.Image());

    //
    const FcBuffer& sceneDataBuffer = mSceneRenderer.getSceneDataBuffer();

    mSkybox.init(sceneDataBuffer);
    //

    mSceneData.projection = pActiveCamera->Projection();


    // TODO get rid of build pipeline calls and just do that within init
    // TODO see if we can decouple the scene descriptor layout
    mBoundingBoxRenderer.init(sceneDataBuffer);


    mNormalRenderer.init(sceneDataBuffer);
    mBillboardRenderer.init(mDescriptorCollection);
    // // TODO think about destroying layout here

    mSceneData.sunlightDirection = glm::vec4(43.1f, 25.f, 23.f, 1.f);
    glm::vec3 target{mSceneData.sunlightDirection.x, 0.0, mSceneData.sunlightDirection.z};

    mShadowRenderer.updateLightSource(mSceneData.sunlightDirection, target);
  }


  //
  void FcRenderer::initImgui(VkFormat swapchainFormat, FcConfig& config)
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
    if (!ImGui_ImplSDL2_InitForVulkan(config.getWindowPtr()->SDLwindow()))
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
    imGuiInfo.Instance = FcLocator::vkInstance();
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



  //
  // TODO try to create multiple command buffers for specific tasks that we can reuse...
  const CommandBufferWrapper& FcRenderer::beginCommandBuffer()
  {
    /* fcPrintEndl("Acquiring command buffer"); */
    // begin recording transfer commands
    const CommandBufferWrapper& cmdBuffer = mImmediateCommands.acquire();
    return cmdBuffer;
  }


  // FIXME
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

    mSceneData.lighSpaceTransform = clip * mShadowRenderer.LightSpaceMatrix();

    mSceneRenderer.updateSceneData(mSceneData);

    // // TODO could flag draw collection items with nodes that "know" they need to be updated
    // // then draw collection could do all the updating in one go.

    // TODO
    // could update frustum by sending camera in and then could in turn be sent to
    // various rendering methods
    mFrustum.update(mSceneData.viewProj);
    mFrustum.normalize();
    mTerrain.update(mFrustum);

    // Update sceneUpdate time in ms
    mDrawCollection.stats.sceneUpdateTime = mTimer.elapsedTime() * 1000;
  }


  //
  void FcRenderer::beginFrame()
  {
    // call to update scene immediately (before waiting on fences)
    // TODO remove from drawframe and place in frolic
    updateScene();

    // reset counters and frame clock
    mDrawCollection.stats.objectsRendered = 0;
    mDrawCollection.stats.triangleCount = 0;
    mTimer.start();

    // TODO fix command buffer stuff and remove this kind of initiallization
    mCurrentCommandBuffer = CommandBuffer(this);
    VkCommandBuffer cmd = mCurrentCommandBuffer.getVkCommandBuffer();

    // transition draw image from undefined layout to best format we can draw to
    mDrawImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    //
    mDepthImage.transitionLayout(cmd, VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

    mSwapchain.acquireCurrentFrame();

    // load the color attachment and depth attachment to our draw image and depth image
    mColorAttachment.imageView = mDrawImage.ImageView();
    // Attach depthImage to the previously null render info
    mRenderInfo.pDepthAttachment = &mDepthAttachment;

    // TODO try to rellocate or do separately (need to address memory barrier issues first)
    mShadowRenderer.generateMap(cmd, mDrawCollection);

    vkCmdBeginRendering(cmd, &mRenderInfo);

    vkCmdSetViewport(cmd, 0, 1, &mDynamicViewport);
    vkCmdSetScissor(cmd, 0, 1, &mDynamicScissors);
  }


  //
  void FcRenderer::drawFrame()
  {
    VkCommandBuffer cmd = mCurrentCommandBuffer.getVkCommandBuffer();

    // TODO implement without branches
    bool* shouldDrawShadowMap = CVarSystem::Get()->GetBoolCVar("shouldDrawShadowMap.bool");
    if (*shouldDrawShadowMap)
    {
      mShadowRenderer.drawDebugMap(cmd, mDescriptorCollection);
    }
    else
    {
      mSceneRenderer.draw(cmd, mDrawCollection, shouldDrawWireframe);

      // TODO condense this into an array of function pointers so that we can build
      // the specific 'pipeline' of function calls and avoid branches

      // Draw the bounding box around the object if enabled
      if (mDrawBoundingBoxes)
      {
        mBoundingBoxRenderer.draw(cmd, mDrawCollection, mDescriptorCollection, mBoundingBoxId);
      }

      if(mDrawNormalVectors)
      {
        mNormalRenderer.draw(cmd, mDrawCollection, mDescriptorCollection);
      }

      // TODO extrapolate functionality to frolic.cpp or cartridge
      // TODO have safegaurd for when the surface at index does not exist
      // only draw terrain when outside of building
      const FcSubmesh& building = mDrawCollection.getSurfaceAtIndex(32);
      // if (camera.isInside(building))...
      if (building.isInBounds(mSceneData.eye))
      {
        mTerrain.draw(cmd, mSceneData, shouldDrawWireframe);
      }

      // Draw the skybox last so that we can skip pixels with ANY object in front of it
      mSkybox.draw(cmd, mDescriptorCollection);


      mBillboardRenderer.draw(cmd, mSceneData);
    }

    vkCmdEndRendering(cmd);

    // TODO this may be necessary for some synchronization to take place in the future (multi-threading) but
    // for now is disabled because the framerate suffer pretty substantially while enabled.
    // const bool shouldPresent = true;
    // if (shouldPresent)
    // {
    //   // FIXME this should signal our timeline semaphore
    //   const u64 signalValue = mSwapchain.syncTimelineSignalValue();

    //   // Tell swapchain to wait for this value next time we want to acquire the swapchain image
    //   mImmediateCommands.signalSemaphore(mTimelineSemaphore, signalValue);
    // }


    // now that we have finished drawing to our internal draw image, copy to swapchain and present
    mSwapchain.present(mDrawImage, cmd);

    // Convert elapsed time to milliseconds
    mDrawCollection.stats.meshDrawTime = mTimer.elapsedTime() * 1000;
  }


  //
  void FcRenderer::drawImGui()
  {
    mColorAttachment.imageView = mSwapchain.getFrameTexture().ImageView();

    // Remove the previously connected depth attachment from the render info (since ImGui doesn't need it)
    mRenderInfo.pDepthAttachment = nullptr;

    VkCommandBuffer cmd = mCurrentCommandBuffer.mWrapper->cmdBuffer;

    // Start rendering for GUI
    vkCmdBeginRendering(cmd, &mRenderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
  }


  //
  void FcRenderer::endFrame()
  {
    mCurrentCommandBuffer.mLastSubmitHandle = mImmediateCommands.submit(*mCurrentCommandBuffer.mWrapper);

    // Try and delete some of the buffers we created along the way (staging buffers, etc.)
    mJanitor.flushBuffers();

    // TODO call to resource manager Update(). Do any texture uploads along with deletions...
    // ?? For now update bindless textures here, after the rendering with textures has ended
    // TRY to relocate to a more descriptive/intuitive location if possible
    if (mDrawCollection.bindlessTextureUpdates.size())
    {
      updateBindlessDescriptors();
    }

    // TODO eliminate mFrameNumber from renderer -> swapchain already contains this
    // get next frame (use % MAX_FRAME_DRAWS to keep value below the number of frames we have in flight
    // increase the number of frames drawn
    // mFrameNumber++;
    // mDrawCollection.stats.frame = mFrameNumber;
  }


  // ?? would this be better placed in descriptor class
  void FcRenderer::updateBindlessDescriptors()
  {
    // Handle deferred writes to bindless textures
    // TODO probably should cache these as vectors of size bindlessTextureUpdates.size()

    // TODO find out how many/often we update DSs
    if (mDrawCollection.bindlessTextureUpdates.size())
    {
      VkWriteDescriptorSet bindlessDescriptorWrites[MAX_BINDLESS_RESOURCES];
      VkDescriptorImageInfo bindlessImageInfos[MAX_BINDLESS_RESOURCES];

      u32 currentWriteIndex = 0;

      for (i32 i = mDrawCollection.bindlessTextureUpdates.size() - 1; i >= 0; --i)
      {
        ResourceUpdate& textureToUpdate = mDrawCollection.bindlessTextureUpdates[i];

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
          /* descriptorWrite.dstSet = *mDescriptorCollection.sceneBindlessTextureSet; */
          // DELETE in favor of above ??
          descriptorWrite.dstSet = mSceneRenderer.getSceneDescSet();
          texture = mDrawCollection.mTextures.get(textureToUpdate.handle);
        }
        else if (textureToUpdate.type == ResourceDeletionType::Billboard)
        {
          descriptorWrite.dstSet = *mDescriptorCollection.billboardDescriptorSet;
          texture = mDrawCollection.mBillboards.get(textureToUpdate.handle);
        }
        else
        {
          fcPrintEndl("Failed to Update Bindless Resource: ");
          /* std::cout << " << textureToUpdate.handle << std::endl; */
          descriptorWrite.dstSet = *mDescriptorCollection.billboardDescriptorSet;
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

        textureToUpdate.currentFrame = U32_MAX;
        uint32_t size = mDrawCollection.bindlessTextureUpdates.size();
        mDrawCollection.bindlessTextureUpdates[i] = mDrawCollection.bindlessTextureUpdates[size - 1];
        mDrawCollection.bindlessTextureUpdates.pop_back();
      }

      // BUG tries to update destroyed descriptor sets
      vkUpdateDescriptorSets(pDevice, currentWriteIndex, bindlessDescriptorWrites, 0, nullptr);
    }

    mJanitor.flushDescLayouts();
  }


  //
  void FcRenderer::shutDown()
  {
    // wait until no actions being run on device before destroying
    mJanitor.flushBuffers();

    vkDeviceWaitIdle(pDevice);

    mDrawCollection.destroy();

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULTS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    FcDefaults::destroy();

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

    // TODO conditionalize all elements that might not need destroying if outside
    // game is using engine and does NOT create all expected elements
    mSwapchain.destroy();
    mDrawImage.destroy();
    mDepthImage.destroy();
    //
    mGpu.release(FcLocator::vkInstance());

    // if (enableValidationLayers)
    // {
    //   /* DestroyDebugUtilsMessengerExt(mInstance, debugMessenger, nullptr); */
    // }
  }


} // END namespace fc
