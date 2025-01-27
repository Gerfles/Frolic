#include "fc_model_render_system.hpp"

 // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_descriptors.hpp"
#include "fc_gpu.hpp"
#include "fc_pipeline.hpp"
#include "fc_locator.hpp"
 // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
 // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  void FcModelRenderSystem::createPipeline(FcPipeline& pipeline)

  {
     // -*-*-*-*-*-*-*-*-*-*-*-*-   CREATE PIPELINE LAYOUT   -*-*-*-*-*-*-*-*-*-*-*-*- //
     // according to gpu vendors, their best use case is to send some indexes to the shader to be
     // used to access some bigger data buffers.
    FcPipelineConfig pipelineInfo{2};
    pipelineInfo.shaders[0].filename = "tri.vert.spv";
    pipelineInfo.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineInfo.shaders[1].filename = "tri.frag.spv";
    pipelineInfo.shaders[2].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPushConstantRange pushConstantRange{};
     // shader stage push constant will go to
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0; // offset into given data to pass to push constant
    pushConstantRange.size = sizeof(ModelPushConstantData); // size of data being passed

    pipelineInfo.addPushConstants(pushConstantRange);
    pipelineInfo.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);

    pipeline.create(pipelineInfo);
  }



  void FcModelRenderSystem::renderGameObjects()
  {
    // mLvePipeline->bind(frameInfo.commandBuffer);

    // vkCmdBindDescriptorSets(frameInfo.commandBuffer
    //                         , VK_PIPELINE_BIND_POINT_GRAPHICS
    //                         , mPipelineLayout
    //                         , 0, 1
    //                         , &frameInfo.globalDescriptorSet
    //                         , 0, nullptr);

    // for (auto& keyValuePair : frameInfo.gameObjects)
    // {
    //   LveGameObject& object = keyValuePair.second;
    //    // skip any object that doesnt have a model
    //   if (object.model == nullptr) continue;
    //    // render
    //   SimplePushConstantData push{};
    //    //auto modelMatrix = object.transform.mat4();
    //   push.modelMatrix = object.transform.mat4();
    //   push.normalMatrix = object.transform.normalMatrix();

    //   vkCmdPushConstants(
    //     frameInfo.commandBuffer,
    //     mPipelineLayout,
    //     VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    //     0,
    //     sizeof(SimplePushConstantData),
    //     &push);

    //   object.model->bind(frameInfo.commandBuffer);
    //   object.model->draw(frameInfo.commandBuffer);
    // }


    // mLvePipeline->bind(frameInfo.commandBuffer);

     // for (auto& obj : mGameObjects)
     // {
     //   obj.transform2d.rotation = glm::mod(obj.transform2d.rotation + 0.01f, glm::two_pi<float>());

     //   SimplePushConstantData pushData{};
     //   pushData.offsets = obj.transform2d.translation;
     //   pushData.color = obj.color;
     //   pushData.transform = obj.transform2d.mat2();

     //   vkCmdPushConstants(frameInfo.commandBuffer
     //                      , mPipelineLayout
     //                      , VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
     //                      , 0
     //                      , sizeof(SimplePushConstantData)
     //                      , &pushData);

     //   obj.model->bind(frameInfo.commandBuffer);
     //   obj.model->draw(frameInfo.commandBuffer);
     // }
  }





  FcModelRenderSystem::~FcModelRenderSystem()
  {
  }


} // namespace fc _END_
