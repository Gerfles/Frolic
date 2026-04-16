//>--- fc_billboard_renderer.cpp ---<//
#include "fc_billboard_renderer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/log.hpp"
#include "fc_types.hpp"
#include "fc_billboard.hpp"
#include "fc_descriptors.hpp"
#include "fc_frame_assets.hpp"
#include "fc_locator.hpp"
#include "fc_math.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <algorithm>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  void FcBillboardRenderer::init(FcDescriptorCollection& descCollection) noexcept
  {
    FcPipelineConfig pipelineConfig;
    pipelineConfig.name = "Billboard";
    pipelineConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "bindless_billboard.vert.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "bindless_billboard.frag.spv");

    // Setup pipeline parameters
    pipelineConfig.enableDepthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.enableBlendingAlpha();
    // MUST signal vulkan that we won't be reading any vertex data
    pipelineConfig.disableVertexReading();

    // Add push constants to pipeline
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BillboardPushConstants);
    pipelineConfig.addPushConstants(pushConstantRange);

    // Create the uniform buffer object
    mUboBuffer.allocate(sizeof(BillboardUbo), FcBufferTypes::Uniform);
    mUboBuffer.setName("Billboard Uniform buffer");


    // Create the descriptor sets used to build pipeline
    pipelineConfig.attachUniformBuffer(0, 0, mUboBuffer, sizeof(BillboardUbo), 0, VK_SHADER_STAGE_VERTEX_BIT);
    pipelineConfig.attachBindlessDescriptors(0);

    // Create and register our billboard descriptor set with the passed in descriptor collection
    mDescriptorSet = pipelineConfig.createDescriptorSet(0);
    descCollection.billboardDescriptorSet = &mDescriptorSet;

    //
    mPipeline.create(pipelineConfig);
  }


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
  void FcBillboardRenderer::update(VkCommandBuffer cmd, SceneData& sceneData) noexcept
  {
    mBillboardUbo.view = sceneData.view;
    mBillboardUbo.projection = sceneData.projection;

    mUboBuffer.write(false, cmd, &mBillboardUbo, sizeof(BillboardUbo));

    // TODO This is a good candidate for a compute shader
    sortBillboardsByDistance(sceneData.eye);
  }


  //
  void FcBillboardRenderer::draw(VkCommandBuffer cmd, SceneData& sceneData) noexcept
  {
    /* update(cmd, sceneData); */
    // ?? TEST would this be better to just send scenedata already packaged and just use what's necessary
    // mBillboardUbo.view = sceneData.view;
    // mBillboardUbo.projection = sceneData.projection;

    // mUboBuffer.write(true, cmd, &mBillboardUbo, sizeof(BillboardUbo));

    // // TODO This is a good candidate for a compute shader
    // sortBillboardsByDistance(sceneData.eye);

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
    mPipeline.bindDescriptorSet(cmd, mDescriptorSet, 0);

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
  void FcBillboardRenderer::destroy() noexcept
  {
    FC_DEBUG_LOG("Destroying billboard renderer...");

    mPipeline.destroy();
    mUboBuffer.immediateDestroy();
  }
}// --- namespace fc --- (END)
