//>--- fc_renderer.cpp ---<//
#include "fc_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/log.hpp"
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
#include<cmath>
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
    fcPrintEndl("Initializing Main Renderer... ");
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




      //
      mImmediateCommands.init(pDevice, mGpu.getQueues().graphicsFamily, "SYNC");



      // create the swapchain & renderpass & frambuffers & depth buffer
      VkFormat swapchainImageFormat = mSwapchain.init(mGpu, config);


      // create timeline semaphore
      mTimelineSemaphore = createTimelineSemaphore(pDevice,
                                                   mSwapchain.imageCount() - 1,
                                                   "Semaphore: mTimelineSemaphore");

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


      //
      FcLocator::provide(&mJanitor);

      // Initialize all the default textures/samplers/etc. to use in places where they are not imported
      fcPrint("Initializing default samplers/textures/etc... ");
      FcDefaults::init(pDevice);
      fcPrintEndl("DONE");

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
    fcPrint("Initializing Main Renderer Defaults... ");
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
    mTerrainRenderer.init("..//maps/terrain_heightmap_r16.ktx2");

    // FIXME should be able to load any kind of terrain map
    /* mTerrainRenderer.init("..//maps/iceland_heightmap.png"); */;

    // TODO Organize initialization

    // set the uniform buffer for the material data
    materialConstants.allocate(sizeof(MaterialConstants), FcBufferTypes::Uniform);
    materialConstants.setName("Material Constants");

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

    // Build memory barrier to transition draw image to be written to
    populateImageMemoryBarrier(VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                               mDrawImage.getVkImage(),
                               mDrawImgColorAttachmentBarrier);

    // Build memory barrier to transition depth buffer to be written to
    populateImageMemoryBarrier(VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
                               mDepthImage.getVkImage(),
                               mDepthImgAttachmentOptimalBarrier);

    // Build memory barrier to transition draw image to copy itself into swapchain buffer
    populateImageMemoryBarrier(VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                               VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                               mDrawImage.getVkImage(),
                               mDrawImgWriteAccessBarrier);

    fcPrintEndl("DONE");
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
  // TODO try to create multiple command buffers for specific tasks that we can reuse... perhaps
  // putting some premade command buffer statically within the image class etc.
  // TODO get rid of this functionality within renderer
  FcCommandBuffer& FcRenderer::beginCommandBuffer()
  {
    /* fcPrintEndl("Acquiring command buffer"); */
    // begin recording transfer commands
    // CommandBuffer& cmdBuffer = mImmediateCommands.acquire();
    // return cmdBuffer;

    return mImmediateCommands.acquire();
  }


  // FIXME
  void FcRenderer::submitCmdBuffer(FcCommandBuffer& cmdBuffer)
  {
    // const bool shouldPresent = hasSwapChain() && present;

    // if shouldPresent
    // {
    //   const u64
    // }
    // End commands
    mImmediateCommands.submit(cmdBuffer);

    /* vkWaitForFences(pDevice, 1, &mImmediateFence, true, U64_MAX); */
  }



  void FcRenderer::submitNonRenderCmdBuffer(FcCommandBuffer& cmdBuffer)
  {
    // const bool shouldPresent = hasSwapChain() && present;

    // if shouldPresent
    // {
    //   const u64
    // }
    // End commands
    mImmediateCommands.submitSingleUseCmdBuffer(cmdBuffer);

    /* vkWaitForFences(pDevice, 1, &mImmediateFence, true, U64_MAX); */
  }


  //
  // TODO should pass in variables from frolic.cpp here
  void FcRenderer::updateScene()
  {
    mTimer.start();

    mCurrentCommandBuffer = &mImmediateCommands.acquire();
    VkCommandBuffer cmd = mCurrentCommandBuffer->getVkCmdBuffer();

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

    mSceneRenderer.updateSceneData(cmd, mSceneData);

    // // TODO could flag draw collection items with nodes that "know" they need to be updated
    // // then draw collection could do all the updating in one go.

    // TODO
    // could update frustum by sending camera in and then could in turn be sent to
    // various rendering methods
    mFrustum.update(mSceneData.viewProj);
    mFrustum.normalize();

    mTerrainRenderer.update(cmd, mFrustum);
    mBillboardRenderer.update(cmd, mSceneData);

    // Update sceneUpdate time in ms
    mDrawCollection.stats.sceneUpdateTime = mTimer.elapsedTime() * 1000;
  }


  //
  void FcRenderer::beginFrame()
  {
    // reset counters and frame clock
    mTimer.start();
    mDrawCollection.stats.objectsRendered = 0;
    mDrawCollection.stats.triangleCount = 0;

    VkCommandBuffer cmd = mCurrentCommandBuffer->getVkCmdBuffer();

    // transition draw image from undefined layout to the optimal format for drawing into
    mDrawImage.transitionLayoutCached(cmd, mDrawImgColorAttachmentBarrier);

    // transition depth image into depth attachment for sampling
    mDepthImage.transitionLayoutCached(cmd, mDepthImgAttachmentOptimalBarrier);

    mSwapchain.acquireCurrentFrame();

    // load the color attachment and depth attachment to our draw image and depth image
    mColorAttachment.imageView = mDrawImage.ImageView();
    // Attach depthImage to the previously null render info
    mRenderInfo.pDepthAttachment = &mDepthAttachment;

    // TODO try to rellocate or do separately (need to address memory barrier issues first)
    mShadowRenderer.generateMap(mDrawCollection);

    vkCmdBeginRendering(cmd, &mRenderInfo);

    vkCmdSetViewport(cmd, 0, 1, &mDynamicViewport);
    vkCmdSetScissor(cmd, 0, 1, &mDynamicScissors);
  }


  //
  void FcRenderer::drawFrame()
  {
    VkCommandBuffer cmd = mCurrentCommandBuffer->getVkCmdBuffer();

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
      if (mShouldDrawBoundingBoxes)
      {
        mBoundingBoxRenderer.draw(cmd, mDrawCollection, mDescriptorCollection, mBoundingBoxId);
      }

      if(mShouldDrawNormalVectors)
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
        mTerrainRenderer.draw(cmd, mSceneData, shouldDrawWireframe);
      }

      mBillboardRenderer.draw(cmd, mSceneData);

      // Draw the skybox nearly last so that we can skip pixels with ANY object in front of it
      mSkybox.draw(cmd, mDescriptorCollection);
    }

    vkCmdEndRendering(cmd);

    // // TODO this may be necessary for some synchronization to take place in the future (multi-threading) but
    // // for now is disabled because the framerate suffer pretty substantially while enabled.
    // const bool shouldPresent = true;
    // if (shouldPresent)
    // {
    //   // FIXME this should signal our timeline semaphore
    //   const u64 signalValue = mSwapchain.syncTimelineSignalValue();

    //   // Tell swapchain to wait for this value next time we want to acquire the swapchain image
    //   mImmediateCommands.signalSemaphore(mTimelineSemaphore, signalValue);
    // }

    // First transition draw image into transfer source layout so we can copy to the swapchain image
    mDrawImage.transitionLayoutCached(cmd, mDrawImgWriteAccessBarrier);

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

    VkCommandBuffer cmd = mCurrentCommandBuffer->getVkCmdBuffer();

    // Start rendering for GUI
    vkCmdBeginRendering(cmd, &mRenderInfo);

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

    vkCmdEndRendering(cmd);
  }


  //
  void FcRenderer::endFrame()
  {

    mImmediateCommands.submit(*mCurrentCommandBuffer);
    /* mCurrentCommandBuffer.mLastSubmitHandle = mImmediateCommands.submit(*mCurrentCommandBuffer.mWrapper); */

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
      // TODO utilize pre-allocated vectors instead
      VkWriteDescriptorSet bindlessDescriptorWrites[MAX_BINDLESS_RESOURCES];
      VkDescriptorImageInfo bindlessImageInfos[MAX_BINDLESS_RESOURCES];

      u32 currentWriteIndex = 0;

      // BUG??
      // using signed integer here so
      for (size_t i = mDrawCollection.bindlessTextureUpdates.size(); i > 0; --i)
      {
        // update the bindless descriptor sets in reverse order
        ResourceUpdate& update = mDrawCollection.bindlessTextureUpdates[i - 1];

        // TRY only doing under the following circumstance
        /* if (textureToUpdate.current_frame == current_frame) */
        VkWriteDescriptorSet& descriptorWrite = bindlessDescriptorWrites[currentWriteIndex];
        descriptorWrite = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.dstBinding = BINDLESS_DESCRIPTOR_SLOT;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.dstArrayElement = update.textureHandle.index();

        FcImage* texture;
        texture = mDrawCollection.mTextures.getElement(update.textureHandle);

        if (update.type == ResourceDeletionType::Texture)
        {
          descriptorWrite.dstSet = mSceneRenderer.getSceneDescSet();
        }
        else if (update.type == ResourceDeletionType::Billboard)
        {
          descriptorWrite.dstSet = mBillboardRenderer.getBillboardDescSet();
          // FcHandle<FcImage> tempHandle = mDrawCollection.mTextures.getHandle<FcImage>(71);
          // texture = mDrawCollection.mTextures.getElement(tempHandle);
          /* descriptorWrite.dstSet = mSceneRenderer.getSceneDescSet(); */
        }
        else
        {
          FC_DEBUG_LOG_FORMAT("FAILED: to update bindless resource: %u", update.textureHandle);
          /* descriptorWrite.dstSet = *mDescriptorCollection.billboardDescriptorSet; */
          descriptorWrite.dstSet = mBillboardRenderer.getBillboardDescSet();
          texture = &FcDefaults::Textures.checkerboard;
        }

        VkDescriptorImageInfo& imageInfo = bindlessImageInfos[currentWriteIndex];
        imageInfo.imageView = texture->ImageView();
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.sampler = texture->hasSampler() ? texture->Sampler() : FcDefaults::Samplers.Linear;

        descriptorWrite.pImageInfo = &imageInfo;

        ++currentWriteIndex;

        update.currentFrame = U32_MAX;

        // ?? not sure what this is trying to accomplish
        /* uint32_t size = mDrawCollection.bindlessTextureUpdates.size(); */
        /* mDrawCollection.bindlessTextureUpdates[i - 1] = mDrawCollection.bindlessTextureUpdates[size - 1]; */

        mDrawCollection.bindlessTextureUpdates.pop_back();
      }

      vkUpdateDescriptorSets(pDevice, currentWriteIndex, bindlessDescriptorWrites, 0, nullptr);
    }

    // Finally get rid of any descriptor set layouts we've had laying around waiting for descriptor sets
    // to update. NOTE: even though it seems that the descriptor set layouts are no longer needed once
    // the descriptor sets are created, vulkan has chosen to tightly couple the two and lacking good documentation
    mJanitor.flushDescLayouts();
  }


  //
  void FcRenderer::shutDown()
  {
    // wait until no actions being run on device before destroying
    materialConstants.immediateDestroy();
    mDrawCollection.destroy();

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEFAULTS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    FcDefaults::destroy();

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   SUB-RENDERERS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    mSceneRenderer.destroy();
    mShadowRenderer.destroy();
    mBoundingBoxRenderer.destroy();
    mNormalRenderer.destroy();
    mBillboardRenderer.destroy();
    mTerrainRenderer.destroy();
    mSkybox.destroy();

    // TODO may need placed elsewhere
    mJanitor.flushBuffers();

    FcLocator::DescriptorClerk().destroy();

    // close imGui
    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(pDevice, mImgGuiDescriptorPool, nullptr);

    // TODO conditionalize all elements that might not need destroying if outside
    // game is using engine and does NOT create all expected elements
    mSwapchain.destroy();
    mDrawImage.destroy();
    mDepthImage.destroy();


    vkDestroySemaphore(pDevice, mTimelineSemaphore, nullptr);

    //
    mImmediateCommands.destroy();
    //
    mGpu.release();

    // if (enableValidationLayers)
    // {
    //   /* DestroyDebugUtilsMessengerExt(mInstance, debugMessenger, nullptr); */
    // }
  }


} // END namespace fc
