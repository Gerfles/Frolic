#pragma once


// - FROLIC ENGINE -
//#include "fc_font.hpp"
#include "fc_gpu.hpp"
#include "fc_swapChain.hpp"
//#include "fc_descriptors.hpp"
//#include "mesh.h"
// - EXTERNAL LIBRARIES -
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <vector>



namespace fc
{
  struct PipelineConfigInfo
  {
     PipelineConfigInfo();
     PipelineConfigInfo(const PipelineConfigInfo&) = delete;
     PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

     std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
     std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
     VkPipelineViewportStateCreateInfo viewportInfo{};
     VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
     VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
     VkPipelineMultisampleStateCreateInfo multiSamplingInfo{};
     VkPipelineColorBlendAttachmentState colorBlendAttachment{};
     VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
     VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
     std::vector<VkDynamicState> dynamicStateEnables{};
     VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
//     VkSampleCountFlagBits rasterizationSamples{};
     VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
     VkRenderPass renderPass = VK_NULL_HANDLE;
     uint32_t subpass = 0;
  };

  struct ShaderInfo
  {
     std::string filename;
     VkShaderStageFlagBits stage;
  };


  // TODO on all the classes that are only instantiated once per game, initialize all the vulkan pointer to VK_NULL_HANDLE
    class FcPipeline
    {
     private:
        //TODO see if this can be eliminated and passed as init instead()
        //TODO if this device is necessary, should make all ref pointers const
       VkPipeline mPipeline = nullptr;
       VkPipelineLayout mPipelineLayout = nullptr;
       VkShaderModule createShaderModule(const std::vector<char>& code);
       void createRenderPass();

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


     public:
       void create2(std::vector<ShaderInfo> shaderInfos);
       void bind2(VkCommandBuffer commandBuffer);

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

       FcPipeline() = default;
       ~FcPipeline() = default;
       FcPipeline(const FcPipeline&) = delete;
       FcPipeline& operator=(const FcPipeline&) = delete;
        //
       void populatePipelineConfig(PipelineConfigInfo& configInfo);
       void create(const std::string& vertShaderFilename,
                   const std::string& fragShaderFilename,
                   const PipelineConfigInfo& configInfo);

       void bind(VkCommandBuffer commandBuffer);
        // uint32_t updateTextureDescriptors(VkImageView textureImageView, VkSampler textureSampler)
        //  { return mDescriptor.createTextureDescriptor(textureImageView, textureSampler); }
        // GETTERS
        //size_t ModelUniformAlignment() { return mDescriptor.ModelUniformAlignment(); }
       const VkPipeline& getVkPipeline() const { return mPipeline; }
       VkPipelineLayout& Layout() { return mPipelineLayout; }
        //VkDescriptorSet DescriptorSet(int index) { return mDescriptor[index]; }
        //VkDescriptorSet TextureDescriptorSet(const FcFont& font) { return mDescriptor[font]; }
        //VkDescriptorSet TextureDescriptorSet(const FcMesh& mesh) { return mDescriptor[mesh]; }

       void destroy();
    };




}
