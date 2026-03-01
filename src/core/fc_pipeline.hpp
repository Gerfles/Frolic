//>--- fc_pipeline.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <string>
#include <vector>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  //
  struct ShaderInfo
  {
     std::string filename;
     VkShaderStageFlagBits stageFlag;
     ShaderInfo(VkShaderStageFlagBits stageFlag, std::string filename) :
       filename(std::move(filename)), stageFlag(std::move(stageFlag)) {}
  };


  //
  //
  // Allow constructor with variable argument list (vk stage names)
  struct  FcPipelineConfig
  {
     std::vector<VkPushConstantRange> pushConstantsInfo;
     std::vector<VkDescriptorSetLayout> descriptorlayouts;
     const char* name; // ?? might want to remove but then may serve as good identifier (hashmap)

     // Only used when binding vertex buffer not when using GPU vertex buffer via address
     VkVertexInputBindingDescription bindingDescription{};
     std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};

     // TODO think about making vectors refs
     std::vector<ShaderInfo> shaders; // default size allocation of 1, declare with numStages to increase
     VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
     VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
     VkPipelineMultisampleStateCreateInfo multiSamplingInfo{};
     VkPipelineColorBlendAttachmentState colorBlendAttachment{};
     VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
     VkPipelineRenderingCreateInfo renderInfo{};
     VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
     VkPipelineViewportStateCreateInfo viewportInfo{};
     VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
     VkPipelineTessellationStateCreateInfo tessellationInfo{};
     VkFormat colorAttachmentFormat{};

     FcPipelineConfig() { initDefaultPipelineParameters(); }
     FcPipelineConfig(std::vector<ShaderInfo> shaderInfos);
     FcPipelineConfig(const FcPipelineConfig&) = delete;
     FcPipelineConfig& operator=(const FcPipelineConfig&) = delete;
     void initDefaultPipelineParameters();
     void addStage(VkShaderStageFlagBits stageFlag, std::string filename);
     void addPushConstants(VkPushConstantRange& pushConstant);
     void addDescriptorSetLayout(VkDescriptorSetLayout layout) { descriptorlayouts.push_back(layout);}
     VkDescriptorSetLayout addSingleImageDescriptorSetLayout();
     void setInputTopology(VkPrimitiveTopology topology);
     void setPolygonMode(VkPolygonMode mode);
     void setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
     void enableMultiSampling(VkSampleCountFlagBits sampleCount);
     void disableMultiSampling();
     void setColorAttachment(VkFormat format);
     void disableColorAttachment();
     void setDepthFormat(VkFormat format);

     void disableVertexReading();
     // TODO combine
     void setNonBufferVertexInputAttributes();
     void setVertexInputPositionOnly();
     void disableDepthtest();
     void enableDepthtest(bool depthWriteEnable, VkCompareOp op);
      // TODO combine
     void disableBlending();
     void enableBlendingAdditive();
     void enableBlendingAlpha();
     void setTessellationControlPoints(uint32_t patchControlPoints);
     //
     void reset();
  }; // ---   struct FcPipelineConfigInfo2 --- (END)



   // TODO on all the classes that are only instantiated once per game, initialize all the vulkan pointer to VK_NULL_HANDLE
  class FcPipeline
  {
   private:
      //TODO see if this can be eliminated and passed as init instead()
      //TODO if this device is necessary, should make all ref pointers const
     VkPipeline mPipeline{nullptr};
     VkPipelineLayout mPipelineLayout{nullptr};

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     VkPipelineBindPoint mBindPoint{VK_PIPELINE_BIND_POINT_GRAPHICS};
     const char* mName;
     uint32_t mNumDescriptorSets{0};
     uint32_t mSetIndex{0};
     // TODO determine if this can be deleted
     // VkDescriptorSetLayout mDescriptorSetLayout{nullptr};
     // std::vector<VkDescriptorSet> mLinkedDescriptorSets{};

     VkShaderModule createShaderModule(const std::vector<char>& code);
     void createRenderPass();

   public:
      // void create2(FcPipelineCreateInfo* pipelineInfo);
     void create(FcPipelineConfig& configInfo);
     //
     inline void bindDescriptorSet(VkCommandBuffer cmd, VkDescriptorSet descSet, uint32_t firstSet) const
      {
        // TODO make mBindPoint simpler if possible (separate compute pipeline)
        // TODO provided by vulkan 1_4: use this structure and keep a static version as a
        // member of billboard renderer so we don't need to repopulate every frame...
        // VkBindDescriptorSetsInfo descSetsInfo{}; descSetsInfo.sType =
        // VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO; descSetsInfo.layout =
        // mPipeline.Layout(); /* descSetsInfo.firstSet = 0; */
        // descSetsInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSets.size());
        // descSetsInfo.pDescriptorSets = descriptorSets.data();
        // vkCmdBindDescriptorSets2(cmd, &descSetsInfo);
        vkCmdBindDescriptorSets(cmd, mBindPoint, mPipelineLayout, firstSet, 1, &descSet, 0, nullptr);
      }
     //
     inline void bindDescriptorSets(VkCommandBuffer cmd
                                    , std::vector<VkDescriptorSet> sets, uint32_t firstSet) const
      {
        vkCmdBindDescriptorSets(cmd, mBindPoint, mPipelineLayout, firstSet, sets.size()
                                , sets.data(), 0, nullptr);
      }
     //
     inline void bind(VkCommandBuffer commandBuffer)
      {
        vkCmdBindPipeline(commandBuffer, mBindPoint, mPipeline);
      }

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NEW   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     FcPipeline() = default;
     ~FcPipeline() = default;
     FcPipeline(const FcPipeline&) = delete;
     FcPipeline& operator=(const FcPipeline&) = delete;
     //
     //void populatePipelineConfig(PipelineConfigInfo& configInfo);
     // void create(const std::string& vertShaderFilename,
     //             const std::string& fragShaderFilename,
     //             const PipelineConfigInfo& configInfo);
     // uint32_t updateTextureDescriptors(VkImageView textureImageView, VkSampler textureSampler)
     //  { return mDescriptor.createTextureDescriptor(textureImageView, textureSampler); }

     // GETTERS
     //size_t ModelUniformAlignment() { return mDescriptor.ModelUniformAlignment(); }
     const VkPipeline& getVkPipeline()  { return mPipeline; }
     VkPipelineLayout& Layout() { return mPipelineLayout; }
     const char* Name() { return mName; }
      //VkDescriptorSet DescriptorSet(int index) { return mDescriptor[index]; }
      //VkDescriptorSet TextureDescriptorSet(const FcFont& font) { return mDescriptor[font]; }
      //VkDescriptorSet TextureDescriptorSet(const FcMesh& mesh) { return mDescriptor[mesh]; }
     void destroy();
  };

}// --- namespace fc --- (END)
