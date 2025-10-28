//>_ fc_billboard_renderer.cpp _<//
#include "fc_billboard_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_locator.hpp"
#include "fc_gpu.hpp"
#include "fc_scene.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <map>

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  void FcBillboardRenderer::addBillboard(FcBillboard& billboard)
  {
    mBillboards.emplace_back(std::shared_ptr<FcBillboard>(&billboard));
  }

  //
  //
  void FcBillboardRenderer::buildPipelines()
  {
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BillboardPushes);

    FcPipelineConfig billboardConfig{2};
    billboardConfig.name = "Billboard";
    // TODO provide non-bindless path
    billboardConfig.shaders[0].filename = "bindless_billboard.vert.spv";
    billboardConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    billboardConfig.shaders[1].filename = "bindless_billboard.frag.spv";
    billboardConfig.shaders[1].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
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
    //
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        VK_SHADER_STAGE_VERTEX_BIT);
    //
    mUboBuffer.allocate(sizeof(BillboardUbo), FcBufferTypes::Uniform);
    //
    bindInfo.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                          , mUboBuffer, sizeof(BillboardUbo), 0);
    //
    FcDescriptorClerk& deskClerk = FcLocator::DescriptorClerk();
    //
    mUboDescriptorSetLayout = deskClerk.createDescriptorSetLayout(bindInfo);
    mUboDescriptorSet = deskClerk.createDescriptorSet(mUboDescriptorSetLayout, bindInfo);

    billboardConfig.addDescriptorSetLayout(mUboDescriptorSetLayout);
    billboardConfig.addDescriptorSetLayout(deskClerk.mBindlessDescriptorLayout);

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NON-BINDLESS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // add descriptorset for the texture image
    /* billboardConfig.addSingleImageDescriptorSetLayout(); */

    // bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //                     VK_SHADER_STAGE_FRAGMENT_BIT);
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    mPipeline.create(billboardConfig);
  }


  //
  //
  void FcBillboardRenderer::sortBillboardsByDistance(glm::vec3& cameraPosition)
  {
    std::sort(mBillboards.begin(), mBillboards.end(),
              [cameraPosition](const std::shared_ptr<FcBillboard>& lhs,
                               const std::shared_ptr<FcBillboard>& rhs)
               {
                 auto tempDist = cameraPosition - lhs->Position();
                 float distanceToLhs = glm::dot(tempDist, tempDist);

                 tempDist = cameraPosition - rhs->Position();
                 float distanceToRhs = glm::dot(tempDist, tempDist);

                 return distanceToLhs > distanceToRhs;
               });
  }


  void FcBillboardRenderer::draw(VkCommandBuffer cmd, SceneDataUbo& sceneData, FrameAssets& currentFrame)
  {
    mUbo.view = sceneData.view;
    mUbo.projection = sceneData.projection;
    mUboBuffer.write(&mUbo, sizeof(BillboardUbo));

    // sort the billboards by distance to the camera
    // TODO This is a good candidate for a compute shader
    // std::multimap<float, size_t> sortedIndices;
    // for (size_t i = 0; i < mBillboards.size(); ++i)
    // {
    //   // calculate distance
    //   // TODO write vector functions to subtract 3-component from 4-component
    //   // complete with error avoidance overrides like static_cast<T>
    //   glm::vec3 eyeLoc = glm::vec3(sceneData.eye.x, sceneData.eye.y, sceneData.eye.z);
    //   auto distance = eyeLoc - mBillboards[i]->PushComponent().position;
    //   float distanceSquared = glm::dot(distance, distance);
    //   sortedIndices.insert(std::pair(distanceSquared, i));
    // }

    glm::vec3 cameraPos{sceneData.eye.x, sceneData.eye.y, sceneData.eye.z};
    sortBillboardsByDistance(cameraPos);

    // bind pipeline to be used in render pass
    mPipeline.bind(cmd);

    // TODO profile to see which sorting method prevails (std::sort vs. indexed draws)

    // iterate through billboards in reverse order (to draw them back to front)
    for (auto& billboard : mBillboards)
      /* for (auto index = sortedIndices.rbegin(); index != sortedIndices.rend(); ++index) */
    {
      vkCmdPushConstants(cmd, mPipeline.Layout(),
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                         0, sizeof(BillboardPushes),
                         /* &mBillboards[index->second]->PushComponent()); */
                         &billboard->PushComponent());

      FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

      // TODO  update the global Ubo only once per frame, not each draw call
      // descClerk.update(swapchainImageIndex, &ubo);
      std::array<VkDescriptorSet, 1> descriptorSets;
      descriptorSets[0] = mUboDescriptorSet;
      /* descriptorSets[1] = descClerk.mBindlessDescriptorSet; */

      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              mPipeline.Layout(), 0,
                              static_cast<uint32_t>(descriptorSets.size()),
                              descriptorSets.data(), 0, nullptr);

      mPipeline.bindDescriptorSet(cmd, descClerk.mBindlessDescriptorSet, 1);

      //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
      // TODO accomplish all billboard draws within one draw/bind
      vkCmdDraw(cmd, 6, 1, 0, 0);
    }
  }

  //
  //
  void FcBillboardRenderer::destroy()
  {
    mPipeline.destroy();

    mUboBuffer.destroy();

    vkDestroyDescriptorSetLayout(FcLocator::Device(), mUboDescriptorSetLayout, nullptr);
  }

}// --- namespace fc --- (END)
