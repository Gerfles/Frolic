#include "fc_materials.hpp"
#include "fc_renderer.hpp"
#include <vulkan/vulkan_core.h>
#include <vector>
#include "core/fc_descriptors.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_pipeline.hpp"




namespace fc
{
  void GLTFMetallicRoughness::buildPipelines(FcRenderer *renderer)
  {
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   OPAQUE PIPELINE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

    fcLog("Building materials pipeline");

     // TODO addshader() func
    FcPipelineConfig pipelineConfig{2};
    pipelineConfig.name = "Opaque Pipeline";
    pipelineConfig.shaders[0].filename = "mesh.vert.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineConfig.shaders[1].filename = "brdf.frag.spv";
    pipelineConfig.shaders[1].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    // add push constants
    VkPushConstantRange matrixRange;
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    matrixRange.offset = 0;
    matrixRange.size = sizeof(DrawPushConstants);

    pipelineConfig.addPushConstants(matrixRange);

    FcDescriptorBindInfo bindInfo{};
    VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stages);
    bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);
    bindInfo.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);
    bindInfo.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);
    bindInfo.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);
    bindInfo.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);

     // create the descriptor set layout for the material
    // TODO check to see if we even need a member variable for the below?? could it be temporary
    mMaterialDescriptorLayout = FcLocator::DescriptorClerk().createDescriptorSetLayout(bindInfo);

    // place the scene descriptor layout in the first slot (0), and material next (1)
    pipelineConfig.addDescriptorSetLayout(renderer->getSceneDescriptorLayout());
    pipelineConfig.addDescriptorSetLayout(mMaterialDescriptorLayout);


     // TODO find a way to do this systematically with the format of the draw/depth image
     // ... probably by adding a pipeline builder to renderer and calling from frolic
    pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineConfig.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);
    // TODO prefer config via:
    //pipelineConfig.enableMultiSampling(VK_SAMPLE_COUNT_1_BIT);
    //pipelineConfig.disableMultiSampling();
    //pipelineConfig.disableBlending();
    //pipelineConfig.enableBlendingAlpha();
    //pipelineConfig.enableBlendingAdditive();
    pipelineConfig.enableDepthtest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // TODO make pipeline and config and descriptors friend classes
    mOpaquePipeline.create(pipelineConfig);

    // *-*-*-*-*-*-*-*-*-*-*-*-*-   TRANSPARENTE PIPELINE   *-*-*-*-*-*-*-*-*-*-*-*-*- //
    // using the same pipeline config, alter slightly for transparent pipeline
    pipelineConfig.name = "Transparent Pipeline";
    pipelineConfig.enableBlendingAdditive();
    pipelineConfig.enableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

     mTransparentPipeline.create(pipelineConfig);
  }


  MaterialInstance GLTFMetallicRoughness::writeMaterial(VkDevice device, MaterialPass pass
                                                        , const MaterialResources &resources)
  {
    MaterialInstance matData;
    matData.passType = pass;

    if (pass == MaterialPass::Transparent)
    {
      fcLog("TRANSPARENT MATERIAl");
      matData.pPipeline = &mTransparentPipeline;
    }
    else
    {
      matData.pPipeline = &mOpaquePipeline;
    }


    FcDescriptorBindInfo bindInfo{};
    VkShaderStageFlags stages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, stages);
    bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);
    bindInfo.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);
    bindInfo.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);
    bindInfo.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);
    bindInfo.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, stages);

     // create the descriptor set layout for the material
    mMaterialDescriptorLayout = FcLocator::DescriptorClerk().createDescriptorSetLayout(bindInfo);

    //FcDescriptorBindInfo bindInfo{};
    bindInfo.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, resources.dataBuffer
                          , sizeof(MaterialConstants), resources.dataBufferOffset);
    bindInfo.attachImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resources.colorImage
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, resources.colorSampler);
    bindInfo.attachImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resources.metalRoughImage
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, resources.metalRoughSampler);
    bindInfo.attachImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resources.normalTexture
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, resources.normalSampler);
    bindInfo.attachImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resources.occlusionTexture
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, resources.occlusionSampler);
    bindInfo.attachImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, resources.emissiveTexture
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, resources.emissiveSampler);


    matData.materialSet = FcLocator::DescriptorClerk().createDescriptorSet(mMaterialDescriptorLayout, bindInfo);

    return matData;
  }

  void GLTFMetallicRoughness::clearResources(VkDevice device)
  {
    mOpaquePipeline.destroy();
    mTransparentPipeline.destroy();
    vkDestroyDescriptorSetLayout(device, mMaterialDescriptorLayout, nullptr);
  }
}
