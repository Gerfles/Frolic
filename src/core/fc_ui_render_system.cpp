#include "fc_ui_render_system.hpp"


// CORE
//#include "core/fc_billboard_render_system.hpp"
#include "core/fc_descriptors.hpp"
#include "core/fc_gpu.hpp"
#include "core/fc_font.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_pipeline.hpp"
// EXTERNAL
//#include "core/mesh.h"
#include "vulkan/vulkan_core.h"
#include <_types/_uint32_t.h>
#include <array>
#include <sys/_types/_size_t.h>


namespace fc
{

  void FcUIrenderSystem::createPipeline(FcPipeline& pipeline, VkRenderPass& renderPass)
  {
     // -- CREATE PIPELINE LAYOUT -- //
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(UIpushConstants);

    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { FcLocator::DescriptorClerk().UboSetLayout()
                                                                , FcLocator::DescriptorClerk().SamplerSetLayout() };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();

     // create pipeline layout
    if (vkCreatePipelineLayout(FcLocator::Device(), &pipelineLayoutInfo, nullptr, &pipeline.Layout()) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Pipeline Layout!");
    }

     // -- CREATE PIPELINE -- //
    assert(pipeline.Layout() != nullptr && "Cannot create pipeline before pipeline layout");
    
    PipelineConfigInfo pipelineConfig;                         // TODO try with = {}; just to make it more clear that this auto populates with defaults
    pipelineConfig.renderPass = renderPass;
     // ?? check that the below is okay and good practice
    pipelineConfig.pipelineLayout = pipeline.Layout();
//    pipelineConfig.rasterizationSamples = VK_SAMPLE_COUNT_8_BIT; //gpu.Properties().maxMsaaSamples;
    pipelineConfig.multiSamplingInfo.rasterizationSamples = FcLocator::Gpu().Properties().maxMsaaSamples;

     // make sure to clear these out since we won't be reading any vertex data and MUST signal vulkan as such
    pipelineConfig.bindingDescriptions.clear();
    pipelineConfig.attributeDescriptions.clear();
    
    pipeline.create("UI.vert.spv", "UI.frag.spv", pipelineConfig);
  }


  
   // TODO NOTE: can only draw boxes but eventually want to allow draw ob objects too
  void FcUIrenderSystem::draw(std::vector<FcText>& UIelements, VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex)
  {
     // bind pipeline to be used in render pass
    // mUIpipeline.bind(commandBuffer);
    //  // DRAW ALL UI COMPONENTS (LAST)
    //  // draw text box
    // for (size_t i = 0; i < UIelements.size(); i++)
    // {
    //   vkCmdPushConstants(commandBuffer, mUIpipeline.Layout()
    //                      , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BillboardPushComponent), &UIelements[i].Push());

    //   VkDeviceSize offsets[] = { 0 };
    //    // vkCmdBindVertexBuffers(mCommandBuffers[swapChainImageIndex], 0, 1, &font.VertexBuffer(), offsets);
    //    // vkCmdBindIndexBuffer(mCommandBuffers[swapChainImageIndex], font.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    //    // TODO don't update UBO unless changed
    //   FcDescriptor& descriptors = FcLocator::DescriptorClerk();
    
    //    //descriptors.update(swapchainImageIndex, &mBillboardUbo);
    
    //   std::array<VkDescriptorSet, 2> descriptorSets = { descriptors.UboDescriptorSet(swapchainImageIndex)
    //                                                   , descriptors.SamplerDescriptorSet(UIelements[i].TextureId()) };
          
    //   vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS
    //                           , mUIpipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
    //                           , descriptorSets.data() , 0, nullptr);
    
    //    //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
    //   vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    // }

  }



  void FcUIrenderSystem::destroy()
  {
//    mUIpipeline.destroy(); 
  }
  
} // _END_ namespace fc
