//>_ fc_billboard_renderer.cpp _<//
#include "fc_billboard_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_billboard.hpp"
#include "fc_descriptors.hpp"
#include "fc_frame_assets.hpp"
#include "fc_locator.hpp"
#include "fc_gpu.hpp"
#include "fc_scene.hpp"
#include "fc_math.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <map>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

//
namespace fc
{
  //
  //
  void FcBillboardRenderer::addBillboard(FcBillboard& billboard) noexcept
  {
    // TODO verify ownership and proper pointer selection
    mBillboards.emplace_back(std::shared_ptr<FcBillboard>(&billboard));
  }

  //
  //
  void FcBillboardRenderer::buildPipelines(std::vector<FrameAssets>& frames) noexcept
  {
    FcPipelineConfig billboardConfig;
    billboardConfig.name = "Billboard";
    billboardConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "bindless_billboard.vert.spv");
    billboardConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "bindless_billboard.frag.spv");

    // Setup pipeline parameters
    billboardConfig.enableDepthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    billboardConfig.enableBlendingAlpha();
    // MUST signal vulkan that we won't be reading any vertex data
    billboardConfig.disableVertexReading();

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BillboardPushConstants);
    //
    billboardConfig.addPushConstants(pushConstantRange);

    // Create the uniform buffer object
    mUboBuffer.allocate(sizeof(BillboardUbo), FcBufferTypes::Uniform);

    // Create descriptor sets and layouts
    FcDescriptorBindInfo bindInfo{};
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                        VK_SHADER_STAGE_VERTEX_BIT);
    bindInfo.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                          , mUboBuffer, sizeof(BillboardUbo), 0);
    bindInfo.enableBindlessTextures();

    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

    VkDescriptorSetLayout bindlessLayout = descClerk.createDescriptorSetLayout(bindInfo);

    for (FrameAssets& frame : frames)
    {
      frame.billboardDescriptorSet = descClerk.createDescriptorSet(bindlessLayout, bindInfo);
    }

    // TODO provied alternate path
    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NON-BINDLESS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // add descriptorset for the texture image
    /* billboardConfig.addSingleImageDescriptorSetLayout(); */

    // bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    //                     VK_SHADER_STAGE_FRAGMENT_BIT);
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    billboardConfig.addDescriptorSetLayout(bindlessLayout);

    mPipeline.create(billboardConfig);
  }

  //
  //
  void FcBillboardRenderer::sortBillboardsByDistance(glm::vec4& cameraPosition) noexcept
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
  void FcBillboardRenderer::draw(VkCommandBuffer cmd,
                                 SceneDataUbo& sceneData,
                                 FrameAssets& currentFrame) noexcept
  {
    mBillboardUbo.view = sceneData.view;
    mBillboardUbo.projection = sceneData.projection;
    mUboBuffer.write(&mBillboardUbo, sizeof(BillboardUbo));

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

    // bind the UBO and textures to pipeline
    mPipeline.bindDescriptorSet(cmd, currentFrame.billboardDescriptorSet, 0);

    // iterate through billboards to draw them back to front
    for (auto& billboard : mBillboards)
    {
      vkCmdPushConstants(cmd, mPipeline.Layout(),
                         VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                         0, sizeof(BillboardPushConstants),
                         billboard.get());

      vkCmdDraw(cmd, 6, 1, 0, 0);
    }
  }

  //
  //
  void FcBillboardRenderer::destroy() noexcept
  {
    mPipeline.destroy();

    mUboBuffer.destroy();
  }
}// --- namespace fc --- (END)
