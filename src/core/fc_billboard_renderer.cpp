//>_ fc_billboard_renderer.cpp _<//
#include "fc_billboard_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_locator.hpp"
#include "fc_gpu.hpp"
#include "fc_scene.hpp"
#include "fc_math.hpp"
#include "vulkan/vulkan.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <map>

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  //
  void FcBillboardRenderer::addBillboard(FcBillboard& billboard)
  {
    mBillboards.emplace_back(std::shared_ptr<FcBillboard>(&billboard));
  }

  //
  //
  void FcBillboardRenderer::buildPipelines(std::vector<FrameAssets>& frames)
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

    // Create the uniform buffer object
    mUboBuffer.allocate(sizeof(BillboardUbo), FcBufferTypes::Uniform);

    // Create descriptor sets and layouts
    FcDescriptorBindInfo bindInfo{};
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        VK_SHADER_STAGE_VERTEX_BIT);
    bindInfo.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                          , mUboBuffer, sizeof(BillboardUbo), 0);
    //
    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
    //
    mUboDescriptorSetLayout = descClerk.createDescriptorSetLayout(bindInfo);
    mUboDescriptorSet = descClerk.createDescriptorSet(mUboDescriptorSetLayout, bindInfo);
    //
    billboardConfig.addDescriptorSetLayout(mUboDescriptorSetLayout);
    billboardConfig.addDescriptorSetLayout(descClerk.mBindlessDescriptorLayout);

    for (FrameAssets& frame : frames)
    {
      frame.billboardDescriptorSet = descClerk.createBindlessDescriptorSet();
    }

    // add descriptorset for the texture image
    /* billboardConfig.addSingleImageDescriptorSetLayout(); */

    // bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //                     VK_SHADER_STAGE_FRAGMENT_BIT);
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    mPipeline.create(billboardConfig);
  }


  //
  //
  void FcBillboardRenderer::sortBillboardsByDistance(glm::vec4& cameraPosition)
  {
    std::sort(mBillboards.begin(), mBillboards.end(),
              [cameraPosition](const std::shared_ptr<FcBillboard>& lhs,
                               const std::shared_ptr<FcBillboard>& rhs)
               {
                 glm::vec3 tempDist = vec3FromSubtractMixed(cameraPosition, lhs->Position());
                 float distanceToLhs = glm::dot(tempDist, tempDist);

                 tempDist = vec3FromSubtractMixed(cameraPosition, rhs->Position());
                 float distanceToRhs = glm::dot(tempDist, tempDist);

                 return distanceToLhs > distanceToRhs;
               });
  }

  //
  //
  void FcBillboardRenderer::draw(VkCommandBuffer cmd, SceneDataUbo& sceneData, FrameAssets& currentFrame)
  {
    mUbo.view = sceneData.view;
    mUbo.projection = sceneData.projection;
    mUboBuffer.write(&mUbo, sizeof(BillboardUbo));

    // TODO This is a good candidate for a compute shader
    sortBillboardsByDistance(sceneData.eye);

    // Alternate method
    // TODO profile to see which sorting method prevails (std::sort vs. indexed draws)
    // sort the billboards by distance to the camera

    // std::multimap<float, size_t> sortedIndices;
    // for (size_t i = 0; i < mBillboards.size(); ++i)
    // {
    //   // calculate distance
    //   glm::vec3 eyeLoc = glm::vec3(sceneData.eye.x, sceneData.eye.y, sceneData.eye.z);
    //   auto distance = eyeLoc - mBillboards[i]->PushComponent().position;
    //   float distanceSquared = glm::dot(distance, distance);
    //   sortedIndices.insert(std::pair(distanceSquared, i));
    // }

    // bind pipeline to be used in render pass
    mPipeline.bind(cmd);

    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

    // iterate through billboards in reverse order (to draw them back to front)
    for (auto& billboard : mBillboards)
    {
      // TODO make billboard's first components compatible with pushConstants...
      vkCmdPushConstants(cmd, mPipeline.Layout(),
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                         0, sizeof(BillboardPushes),
                         billboard.get());

      // TODO  update the global Ubo only once per frame, not each draw call
      std::array<VkDescriptorSet, 1> descriptorSets;
      descriptorSets[0] = mUboDescriptorSet;

      // TODO provided by vulkan 1_4: use this structure and keep a static version as a member
      // of billboard renderer so we don't need to repopulate every frame...
      // VkBindDescriptorSetsInfo descSetsInfo{};
      // descSetsInfo.sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
      // descSetsInfo.layout = mPipeline.Layout();
      // /* descSetsInfo.firstSet = 0; */
      // descSetsInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSets.size());
      // descSetsInfo.pDescriptorSets = descriptorSets.data();

      // vkCmdBindDescriptorSets2(cmd, &descSetsInfo);

      // TODO extrapolate like below or add below to array and bind once...
      vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              mPipeline.Layout(), 0,
                              static_cast<uint32_t>(descriptorSets.size()),
                              descriptorSets.data(), 0, nullptr);

      mPipeline.bindDescriptorSet(cmd, currentFrame.billboardDescriptorSet, 1);

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
