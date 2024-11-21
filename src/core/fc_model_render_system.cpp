#include "fc_model_render_system.hpp"

// CORE
#include "core/fc_descriptors.hpp"
#include "core/fc_gpu.hpp"
#include "core/fc_pipeline.hpp"
// EXTERNAL
#include "vulkan/vulkan_core.h"
// STD


namespace fc
{
  void FcModelRenderSystem::createPipeline(FcGpu& gpu, FcPipeline& pipeline
                                           , FcDescriptor& descriptors, VkRenderPass& renderPass)
  {
     // -- CREATE PIPELINE LAYOUT -- //
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // shader stage push constant will go to
    pushConstantRange.offset = 0;                              // offset into given data to pass to push constant
    pushConstantRange.size = sizeof(ModelPushConstantData);              // size of data being passed

    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { descriptors.UboSetLayout()
                                                                , descriptors.SamplerSetLayout() };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

     // create pipeline layout
    if (vkCreatePipelineLayout(gpu.getVkDevice(), &pipelineLayoutInfo, nullptr, &pipeline.Layout()) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Pipeline Layout!");
    }

     // -- CREATE PIPELINE -- //

    assert(pipeline.Layout() != nullptr && "Cannot create pipeline before pipeline layout");

    PipelineConfigInfo pipelineConfig {};                         // TODO try with = {}; just to make it more clear that this auto populates with defaults
    pipelineConfig.renderPass = renderPass;
     // ?? check that the below is okay and good practice
    pipelineConfig.pipelineLayout = pipeline.Layout();

//    pipelineConfig.rasterizationSamples = VK_SAMPLE_COUNT_8_BIT; //gpu.Properties().maxMsaaSamples;
    pipelineConfig.multiSamplingInfo.rasterizationSamples = gpu.Properties().maxMsaaSamples;

    pipeline.create("tri.vert.spv", "tri.frag.spv", pipelineConfig);
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
