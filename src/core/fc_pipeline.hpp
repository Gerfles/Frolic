#pragma once


 // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include "fc_font.hpp"
//#include "core/fc_renderer.hpp"
//#include "fc_gpu.hpp"
//#include "fc_swapChain.hpp"
//#include "fc_descriptors.hpp"
//#include "mesh.h"
 // -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
#include <glm/vec4.hpp>
 // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD LIBRARIES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <string>
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
     VkShaderStageFlagBits stageFlag;
  };

   // Allow constructor with variable argument list (vk stage names)
  struct FcPipelineConfigInfo2
  {
     const char* name; // ?? might want to remove but then may serve as good identifier (hashmap)
     // std::vector<VkVertexInputBindingDescription> bindingDescriptions{};
     // std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
     std::vector<ShaderInfo> shaders; // default size allocation of 1, declare with numStages to increase
     std::vector<VkPushConstantRange> pushConstantsInfo;
     VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
     VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
     VkPipelineMultisampleStateCreateInfo multiSamplingInfo{};
     VkPipelineColorBlendAttachmentState colorBlendAttachment{};
     VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
     VkPipelineRenderingCreateInfo renderInfo{};
     VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
     VkPipelineViewportStateCreateInfo viewportInfo{};
     VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
     VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
     VkFormat colorAttachmentFormat;
      // std::vector<VkDynamicState> dynamicStateEnables{};
     // VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
     // VkSampleCountFlagBits rasterizationSamples{};


      // Initialize the number of stages with a list initializer
      // TODO CB = comment better
     FcPipelineConfigInfo2(int numStages);
     FcPipelineConfigInfo2(const PipelineConfigInfo&) = delete;
     FcPipelineConfigInfo2& operator=(const PipelineConfigInfo&) = delete;

     void addPushConstants(VkPushConstantRange pushConstant);
     void setInputTopology(VkPrimitiveTopology topology);
     void setPolygonMode(VkPolygonMode mode);
     void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
     void setMultiSampling(VkSampleCountFlagBits sampleCount);
     void setColorAttachment(VkFormat format);
     void setDepthFormat(VkFormat format);
     void disableDepthtest();
     void enableDepthtest(bool depthWriteEnable, VkCompareOp op);
      // TODO combine
     void disableBlending();
     void enableBlendingAdditive();
     void enableBlendingAlpha();
     void clear();
  }; // ---   struct FcPipelineConfigInfo2 --- (END)


  // class PipelineBuilder
  // {
  //  public:
  //    std::vector<VkPipelineShaderStageCreateInfo> mShaderStages;
  //    VkPipelineInputAssemblyStateCreateInfo mInputAssemblyInfo{};
  //    VkPipelineRasterizationStateCreateInfo mRasterizationInfo{};
  //    VkPipelineColorBlendAttachmentState mColorBlendAttachment{};
  //    VkPipelineMultisampleStateCreateInfo mMultiSamplingInfo{};
  //    VkPipelineLayout mPipelineLayout{VK_NULL_HANDLE};
  //    VkPipelineDepthStencilStateCreateInfo mDepthStencilInfo{};
  //    VkPipelineRenderingCreateInfo mRenderInfo{};
  //    VkFormat mColorAttachmentFormat{};
  //     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CTORS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //    PipelineBuilder() { clear(); }
  //     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   UTILS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //    VkPipeline buildPipeline(VkDevice device);
  //    void setShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
  //    void clear();
  // };

  struct ComputePushConstants
  {
     glm::vec4 data1;
     glm::vec4 data2;
     glm::vec4 data3;
     glm::vec4 data4;
  };


   // TODO might want to rename to something more logical
   // TODO Create a full default similar to vkStructs
  struct FcPipelineCreateInfo
  {
     const char* name;
      //ComputePushConstants data;
     std::vector<ShaderInfo> shaders;//{1};

      // Initialize the number of stages with a list initializer
      // TODO CB = comment better
     FcPipelineCreateInfo(int numStages = 0) : shaders(numStages) {};
  };



   // TODO on all the classes that are only instantiated once per game, initialize all the vulkan pointer to VK_NULL_HANDLE
  class FcPipeline
  {
   private:
      //TODO see if this can be eliminated and passed as init instead()
      //TODO if this device is necessary, should make all ref pointers const
     VkPipeline mPipeline = nullptr;
     VkPipelineLayout mPipelineLayout = nullptr;
     VkPipelineBindPoint mBindPoint{VK_PIPELINE_BIND_POINT_GRAPHICS};
     const char* mName;


     VkShaderModule createShaderModule(const std::vector<char>& code);
     void createRenderPass();

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      // class PipelineBuilder

   public:
     void create2(FcPipelineCreateInfo* pipelineInfo);
     void create3(FcPipelineConfigInfo2& configInfo);
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
     const char* Name() { return mName; }
      //VkDescriptorSet DescriptorSet(int index) { return mDescriptor[index]; }
      //VkDescriptorSet TextureDescriptorSet(const FcFont& font) { return mDescriptor[font]; }
      //VkDescriptorSet TextureDescriptorSet(const FcMesh& mesh) { return mDescriptor[mesh]; }

     void destroy();
  };




}
