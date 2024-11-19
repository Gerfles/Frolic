#include "fc_renderer.hpp"

// - FROLIC ENGINE -
#include "SDL2/SDL_stdinc.h"
#include "core/fc_billboard_render_system.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_font.hpp"
#include "core/fc_game_object.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_model_render_system.hpp"
#include "core/fc_swapChain.hpp"
#include "core/fc_mesh.hpp"
#include "utilities.hpp"
#include "fc_debug.hpp"
#include "fc_camera.hpp"
#include <exception>
#include <mutex>
// - EXTERNAL LIBRARIES -
// TODO #define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "vulkan/vulkan_core.h"
#include <SDL2/SDL_vulkan.h>
// - STD LIBRARIES -
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

       // create the swapchain & renderpass & frambuffers & depth buffer
      mBufferCount = mSwapchain.init(mGpu);
      createCommandBuffers();

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



    }
    catch (const std::runtime_error& err) {
      printf("ERROR: %s\n", err.what());
      return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
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

     //TODO change to platform dependent evaluation
     // Only seems to be required for macOS implementation and only when validation layers added
    extensions.push_back("VK_KHR_get_physical_device_properties2");
    extensions.push_back("VK_KHR_portability_enumeration");
    instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

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



  void FcRenderer::createCommandBuffers()
  {
     // resize commmand buffer count to have one for each framebuffer
    mCommandBuffers.resize(mSwapchain.imageCount());

    VkCommandBufferAllocateInfo commandBufferAllocInfo{};
    commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferAllocInfo.commandPool = mGpu.commandPool();
    commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // PRIMARY allows this to be directly used by a queue as opposed to SECONDARY which allows it to be used only by another command buffer using vkExecuteCmdBuffer(secondary buffer)
    commandBufferAllocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

    if (vkAllocateCommandBuffers(mGpu.VkDevice(), &commandBufferAllocInfo, mCommandBuffers.data()))
    {
      throw std::runtime_error("Failed to allocate Vulkan Command Buffers!");
    }
  }


   //TODO change current frame to currentFrameBuffer
  void FcRenderer::drawModels(uint32_t swapchainImageIndex, GlobalUbo& ubo)
  {
     //
    auto gameObjects = FcLocator::GameObjects();

     // bind pipeline to be used in render pass
    mModelPipeline.bind(mCommandBuffers[swapchainImageIndex]);


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

        vkCmdPushConstants(mCommandBuffers[swapchainImageIndex], mModelPipeline.Layout()
                           , VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
                           , 0, sizeof(ModelPushConstantData), &push);

        for (size_t j = 0; j < currModel->MeshCount(); ++j)
        {
          FcMesh& currMesh = currModel->Mesh(j);

           //VkBuffer vertexBuffers[] = { currMesh.VertexBuffer() };// buffers to bind
          VkDeviceSize offsets[] = { 0 }; // offsets into buffers being bound

           // Bind all the necessary resources to our pipeline
           //SDL_Log("%d times ", currMesh.VertexCount());
          vkCmdBindVertexBuffers(mCommandBuffers[swapchainImageIndex], 0, 1, &currModel->Mesh(j).VertexBuffer(), offsets); // command to bind vertex buffer before drawing
          vkCmdBindIndexBuffer(mCommandBuffers[swapchainImageIndex], currMesh.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

           // dynamic offset ammount (SAVED FOR REFERENCE)
           // uint32_t dynamicOffset = static_cast<uint32_t>(mPipeline.ModelUniformAlignment()) * j;
           // vkCmdBindDescriptorSets(mCommandBuffers[currentFrame], VK_PIPELINE_BIND_POINT_GRAPHICS
           //                         , mPipeline.Layout(), 0, 1, mPipeline.DescriptorSet(currentFrame), 1, &dynamicOffset);

           // TODO don't update UBO unless changed (may still need to do just because of frame set)
          FcDescriptor& descriptorClerk = FcLocator::DescriptorClerk();

          descriptorClerk.update(swapchainImageIndex, &ubo);

           // Bind descriptor sets
          std::array<VkDescriptorSet, 2> descriptorSets = {
            descriptorClerk.UboDescriptorSet(swapchainImageIndex)
          , descriptorClerk.SamplerDescriptorSet(currMesh.DescriptorId()) };

          vkCmdBindDescriptorSets(mCommandBuffers[swapchainImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS
                                  , mModelPipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
                                  , descriptorSets.data() , 0, nullptr);

           // execute pipeline
          vkCmdDrawIndexed(mCommandBuffers[swapchainImageIndex], currMesh.IndexCount(), 1, 0, 0, 0);
        }
      }

    } // _END_ FOR draw models in model list
  }

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

    VkCommandBuffer currCommandBuffer = mCommandBuffers[swapchainImageIndex];

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
      FcDescriptor& descriptors = FcLocator::DescriptorClerk();


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
     // mBillboardPipeline.bind(mCommandBuffers[swapchainImageIndex]);
     //  // DRAW ALL UI COMPONENTS (LAST)
     //  // draw text box
     // vkCmdPushConstants(mCommandBuffers[swapchainImageIndex], mBillboardPipeline.Layout()
     //                    , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BillboardPushConstants), &font.Push());

     // VkDeviceSize offsets[] = { 0 };
     // // vkCmdBindVertexBuffers(mCommandBuffers[swapChainImageIndex], 0, 1, &font.VertexBuffer(), offsets);
     // // vkCmdBindIndexBuffer(mCommandBuffers[swapChainImageIndex], font.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

     //      // TODO don't update UBO unless changed
     // mDescriptorManager.update(swapchainImageIndex, &mBillboardUbo);

     // std::array<VkDescriptorSet, 2> descriptorSets = { mDescriptorManager.UboDescriptorSet(swapchainImageIndex)
     //                                                 , mDescriptorManager.SamplerDescriptorSet(font.TextureId()) };

     // vkCmdBindDescriptorSets(mCommandBuffers[swapchainImageIndex], VK_PIPELINE_BIND_POINT_GRAPHICS
     //                         , mBillboardPipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
     //                         , descriptorSets.data() , 0, nullptr);

     //  //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
     // vkCmdDraw(mCommandBuffers[swapchainImageIndex], 6, 1, 0, 0);



//    mBillboardRenderer.draw(cameraPosition, mCommandBuffers[swapchainImageIndex], swapchainImageIndex);
  }


  void FcRenderer::drawUI(std::vector<FcText>& UIelements, uint32_t swapchainImageIndex)
  {
     // mUIrenderer.draw(UIelements, mCommandBuffers[swapchainImageIndex], swapchainImageIndex);

    VkCommandBuffer currCommandBuffer = mCommandBuffers[swapchainImageIndex];
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


  void FcRenderer::createSynchronization()
  {
    mImageReadySemaphores.resize(MAX_FRAME_DRAWS);
    mRenderFinishedSemaphores.resize(MAX_FRAME_DRAWS);
    mDrawFences.resize(MAX_FRAME_DRAWS);

     // semaphore creation information
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

     // fence creation information
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // we want this fence to start off signaled (open) so that it can go through the first draw function.

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
       // create 2 semaphores (one tells us the image is ready to draw to and one tells us when we're done drawing)
      if (vkCreateSemaphore(mGpu.VkDevice(), &semaphoreInfo, nullptr, &mImageReadySemaphores[i]) != VK_SUCCESS
          || vkCreateSemaphore(mGpu.VkDevice(), &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS
          || vkCreateFence(mGpu.VkDevice(), &fenceInfo, nullptr, &mDrawFences[i]) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create a Vulkan sychronization object (Semaphore or Fence)!");
      }
    }
  }



  uint32_t FcRenderer::beginFrame()
  {
     //
    uint64_t maxWaitTime = std::numeric_limits<uint64_t>::max();

     // don't keep adding images to the queue or submitting commands to the buffer until this frame has signalled that it's ready (last draw has finished)
    vkWaitForFences(mGpu.VkDevice(), 1, &mDrawFences[mCurrentFrame], VK_TRUE, maxWaitTime);

     // 1. get the next available image to draw to and set to signal when we're finished with the image (a semaphore)
    uint32_t nextSwapchainImage;
    VkResult result = vkAcquireNextImageKHR(mGpu.VkDevice(), mSwapchain.vkSwapchain(), maxWaitTime
                                            , mImageReadySemaphores[mCurrentFrame], VK_NULL_HANDLE, &nextSwapchainImage);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
      handleWindowResize();
      return -1;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
      throw std::runtime_error("Failed to acquire Vulkan Swap Chain image!");
    }

     // manully un-signal (close) the fence ONLY when we are sure we're submitting work (result == VK_SUCESS)
    vkResetFences(mGpu.VkDevice(), 1, &mDrawFences[mCurrentFrame]);

     // ?? don't think we need this assert since we use semaphores and fences
     // assert(!mIsFrameStarted && "Can't call recordCommands() while frame is already in progress!");

     // information about how to begin each command
    VkCommandBufferBeginInfo bufferBeginInfo{};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

     // information about how to begin a render pass (only needed for graphical applications)
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = mSwapchain.getRenderPass();
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = mSwapchain.getSurfaceExtent();

    std::array<VkClearValue, 3> clearValues{};
    clearValues[0].color = {0.6f, 0.65f, 0.4f, 1.0f};
    clearValues[1].color = {0.0f, 0.0f, 0.0f}; // ?? why are there 2 color attachmentes and not 1?
    clearValues[2].depthStencil.depth = 1.0f;

    renderPassBeginInfo.pClearValues = clearValues.data();
     // TODO since we know this ahead of time always--set simply
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());

     // assign the framebuffer of the corresponding command buffer to assign to the render pass
    renderPassBeginInfo.framebuffer = mSwapchain.getFrameBuffer(nextSwapchainImage);

     // start recording commands to command buffer
    if (vkBeginCommandBuffer(mCommandBuffers[nextSwapchainImage], &bufferBeginInfo) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to start recording a Vulkan Command Buffer!");
    }

     // Begin render pass
    vkCmdBeginRenderPass(mCommandBuffers[nextSwapchainImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

     // TODO would this make more sense to relocate to window resize?
     // ?? also, what are the costs associated with having dynamic states
     // make sure our dynamic viewport and scissors are set properly (if resizing the window etc.)
    mDynamicViewport.width = static_cast<uint32_t>(mSwapchain.getSurfaceExtent().width);
    mDynamicViewport.height = static_cast<uint32_t>(mSwapchain.getSurfaceExtent().height);
    mDynamicScissors.extent = mSwapchain.getSurfaceExtent();
     //
    vkCmdSetViewport(mCommandBuffers[nextSwapchainImage], 0, 1, &mDynamicViewport);
    vkCmdSetScissor(mCommandBuffers[nextSwapchainImage], 0, 1, &mDynamicScissors);



    return nextSwapchainImage;
  } // _END_ beginFrame()



   // TODO fix nomenclature of currentFrame vs nextImageIndex
  void FcRenderer::endFrame(uint32_t swapChainImageIndex)
  {
     // end render pass
    vkCmdEndRenderPass(mCommandBuffers[swapChainImageIndex]);

     // stop recording to command buffer
    if (vkEndCommandBuffer(mCommandBuffers[swapChainImageIndex]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to stop recording a Vulkan Command Buffer!");
    }




     // 2. submit command buffer to queue for rendering, making sure it waits for the image to be signalled as
     // available before drawing and signals when it has finished rendering
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;                         // number of semaphores to wait on
    submitInfo.pWaitSemaphores = &mImageReadySemaphores[mCurrentFrame];             // list of semaphores to wait on
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.pWaitDstStageMask = waitStages;                 // stages to check semaphores at
    submitInfo.commandBufferCount = 1;                         // number of command buffers to submit
    submitInfo.pCommandBuffers = &mCommandBuffers[swapChainImageIndex]; // command buffer to submit
    submitInfo.signalSemaphoreCount = 1;                       // number of semaphores to signal
    submitInfo.pSignalSemaphores = &mRenderFinishedSemaphores[mCurrentFrame];           // semaphores to signal when command buffer finishes

     // Submit the command buffer to the queue
    if (vkQueueSubmit(mGpu.graphicsQueue(), 1, &submitInfo, mDrawFences[mCurrentFrame]) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to submit Vulkan Command Buffer to the queue!");
    }

     // 3. present image to screen when it has signalled finished rendering
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;                  // Number of semaphores to wait on
    presentInfo.pWaitSemaphores = &mRenderFinishedSemaphores[mCurrentFrame];      // semaphore to wait on
    presentInfo.swapchainCount = 1;                      // number of swapchains to present to
    presentInfo.pSwapchains = &mSwapchain.vkSwapchain(); // swapchain to present images to
    presentInfo.pImageIndices = &swapChainImageIndex;             //index of images in swapchains to present

    VkResult result = vkQueuePresentKHR(mGpu.presentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || mWindow.wasWindowResized())
    {
      mWindow.resetWindowResizedFlag();
      handleWindowResize();
    }
    else if (result != VK_SUCCESS)
    {
      throw std::runtime_error("Faled to submit image to Vulkan Present Queue!");
    }

     // get next frame (use % MAX_FRAME_DRAWS to keep value below the number of frames we have in flight
    mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAME_DRAWS;
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

    vkDeviceWaitIdle(mGpu.VkDevice());
    VkExtent2D winExtent{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

     //
    mSwapchain.reCreateSwapChain();
  }



  void FcRenderer::shutDown()
  {
    std::cout << "calling: FcRenderer::shutDown" << std::endl;
     // wait until no actions being run on device before destroying
    vkDeviceWaitIdle(mGpu.VkDevice());

     //mUiRenderer.destroy();

    FcLocator::DescriptorClerk().destroy();

    for (size_t i = 0; i < MAX_FRAME_DRAWS; ++i)
    {
      vkDestroySemaphore(mGpu.VkDevice(), mRenderFinishedSemaphores[i], nullptr);
      vkDestroySemaphore(mGpu.VkDevice(), mImageReadySemaphores[i], nullptr);
      vkDestroyFence(mGpu.VkDevice(), mDrawFences[i], nullptr);
    }

     // TODO conditionalize all elements that might not need destroying if outside
     // game is using engine and does NOT create all expected elements
    mModelPipeline.destroy();
    mBillboardPipeline.destroy();
    mUiPipeline.destroy();

    mSwapchain.destroy();

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
