#include "fc_billboard_render_system.hpp"


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_defaults.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_frame_assets.hpp"
#include "core/fc_gpu.hpp"
#include "core/fc_scene_renderer.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
#include <map>
#include <utility>
#include <vector>


namespace fc
{


FcBillboard::FcBillboard(float width, float height, glm::vec4 color)
  {
    mPush.width = width;
    mPush.height = height;
  }




  void FcBillboardRenderer::addBillboard(FcBillboard& billboard)
  {
    /* mBillboards.emplace_back(std::move(billboard)); */
    mBillboards.emplace_back(std::shared_ptr<FcBillboard>(&billboard));
  }




  void FcBillboard::loadTexture(std::filesystem::path &filename)
  {
    mTexture.loadStbi(filename, FcImageTypes::Texture);
    mPush.width = 0.5;
    mPush.height = 0.5;
    mPush.position = glm::vec4(0.f, 20.f, 0.f, 1.f);

    FcDescriptorBindInfo descriptorInfo;
    descriptorInfo.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                              VK_SHADER_STAGE_FRAGMENT_BIT);

    descriptorInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                               mTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               FcDefaults::Samplers.Linear);

    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

    // TODO create a single image descriptor layout for things like this
    VkDescriptorSetLayout layout = descClerk.createDescriptorSetLayout(descriptorInfo);
    mDescriptor = descClerk.createDescriptorSet(layout, descriptorInfo);
  }



  void FcBillboard::destroy()
  {
    mTexture.destroy();
  }


  // BUG uncomment emplace
  void FcBillboardRenderer::loadBillboard(std::filesystem::path& filename)
  {
    FcBillboard billboard;
    billboard.loadTexture(filename);
    /* mBillboards.emplace_back(std::move(billboard)); */
  }

  // void FcBillboardRenderer::addBillboard() {std::}
  // {

  // }


  void FcBillboardRenderer::buildPipelines()
  {
    FcPipelineConfig billboardConfig{2};
    billboardConfig.name = "Billboard";
    billboardConfig.shaders[0].filename = "billboard.vert.spv";
    billboardConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    billboardConfig.shaders[1].filename = "billboard.frag.spv";
    billboardConfig.shaders[1].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BillboardPushes);
    //
    billboardConfig.addPushConstants(pushConstantRange);

    // Setup pipeline parameters
    billboardConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    billboardConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    billboardConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);
    billboardConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    billboardConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    // TEST
    billboardConfig.enableDepthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    billboardConfig.enableBlendingAlpha();
    // MUST signal vulkan that we won't be reading any vertex data
    billboardConfig.disableVertexReading();

    // Create descriptor sets and layouts
    FcDescriptorBindInfo bindInfo{};

    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        VK_SHADER_STAGE_VERTEX_BIT);

    mUboBuffer.allocate(sizeof(BillboardUbo), FcBufferTypes::Uniform);

    bindInfo.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mUboBuffer, sizeof(BillboardUbo), 0);

    FcDescriptorClerk& deskClerk = FcLocator::DescriptorClerk();

    mUboDescriptorSetLayout = deskClerk.createDescriptorSetLayout(bindInfo);
    mUboDescriptorSet = deskClerk.createDescriptorSet(mUboDescriptorSetLayout, bindInfo);

    billboardConfig.addDescriptorSetLayout(mUboDescriptorSetLayout);
    // add descriptorset for the texture image
    billboardConfig.addSingleImageDescriptorSetLayout();

    // bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //                     VK_SHADER_STAGE_FRAGMENT_BIT);

    mPipeline.create(billboardConfig);
  }


  void FcBillboardRenderer::sortBillboardsByDistance(glm::vec3& cameraPosition)
  {
    // std::vector<FcBillboard* >& billboards = FcLocator::Billboards();

    // for (size_t i = 0; i < billboards.size(); ++i)
    // {


    // }

    // std::sort(std::begin(billboards), std::end(billboards)
    //           , [](FcBillboard& lhs, FcBillboard& rhs, glm::vec3& cameraPosition)
    //            {
    //              auto tempDist = glm::vec4(cameraPosition, 1.f) - lhs.Push().position;
    //              float distanceToLhs = glm::dot(tempDist, tempDist);

    //              tempDist

    //                }
    //           );
  }



  void FcBillboardRenderer::draw(VkCommandBuffer cmd, SceneDataUbo& sceneData, FrameAssets& currentFrame)
  {
    mUbo.view = sceneData.view;
    mUbo.projection = sceneData.projection;
    // ?? TEST with leaving the sizeof off
    mUboBuffer.write(&mUbo, sizeof(BillboardUbo));


    // sort the billboards by distance to the camera
    // TODO sort within update instead
    std::multimap<float, size_t> sortedIndices; // TODO?? uint32 or size_t
    for (size_t i = 0; i < mBillboards.size(); ++i)
    {
      // calculate distance
      auto distance = sceneData.eye - mBillboards[i]->PushComponent().position;
      float distanceSquared = glm::dot(distance, distance);
      sortedIndices.insert(std::pair(distanceSquared, i));
    }

    // bind pipeline to be used in render pass
    mPipeline.bind(cmd);

    // iterate through billboards in reverse order (to draw them back to front)
    for (auto index = sortedIndices.rbegin(); index != sortedIndices.rend(); ++index)
    {
      vkCmdPushConstants(cmd, mPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                         sizeof(BillboardPushes),
                         &mBillboards[index->second]->PushComponent());

      // VkDeviceSize offsets[] = { 0 };
      //  vkCmdBindVertexBuffers(mCommandBuffers[swapChainImageIndex], 0, 1,
      //  &font.VertexBuffer(), offsets);
      //  vkCmdBindIndexBuffer(mCommandBuffers[swapChainImageIndex],
      //  font.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

      FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

      // TODO  update the global Ubo only once per frame, not each draw call
      // descClerk.update(swapchainImageIndex, &ubo);

      std::array<VkDescriptorSet, 2> descriptorSets;
      descriptorSets[0] = mUboDescriptorSet;
      descriptorSets[1] = mBillboards[index->second]->getDescriptor();

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              mPipeline.Layout(), 0,
                              static_cast<uint32_t>(descriptorSets.size()),
                              descriptorSets.data(), 0, nullptr);

      //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
      vkCmdDraw(cmd, 6, 1, 0, 0);
    }
  }


  void FcBillboardRenderer::destroy()
  {
    mPipeline.destroy();

    for (std::shared_ptr<FcBillboard> billboard : mBillboards)
    {
      billboard->destroy();
    }

    mUboBuffer.destroy();

    VkDevice pDevice = FcLocator::Device();

    vkDestroyDescriptorSetLayout(pDevice, mUboDescriptorSetLayout, nullptr);
  }



} // _END_ namespace fc
