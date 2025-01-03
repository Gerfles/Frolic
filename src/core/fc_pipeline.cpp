#include "fc_pipeline.hpp"

// - FROLIC ENGINE -
#include "core/fc_descriptors.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_renderer.hpp"
#include "fc_swapChain.hpp"
#include "utilities.hpp"
// - EXTERNAL LIBRARIES -
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <cstdint>
#include <stdexcept>
#include <vector>
// #include <cstddef>
//#include <type_traits>

namespace fc
{
  void FcPipelineConfigInfo2::clear()
  {
     // clear all of the structs we need back to 0 with their correct stype
     // ?? Uses CPP20 initializers so perhaps a different solution would be preferred for compatibility
    inputAssemblyInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

    rasterizationInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};

    multiSamplingInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};

    depthStencilInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};

    renderInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

    colorBlendAttachment = {};

    pipelineLayout = {};

     // mShaderStages.clear();
  }


  FcPipelineConfigInfo2::FcPipelineConfigInfo2(int numStages) : shaders(numStages)
  {
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   VIEWPORT   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // make viewport state from our stored viewport and scissors
     // TODO add support for multiple viewports or scissors
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = nullptr;//&viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = nullptr;//&scissor;


     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   COLOR BLENDING   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // TODO may need tweaking here with defaults
     // Blend attachment state (how blending is handled)
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendInfo.logicOpEnable = VK_FALSE; // alternative to calculations is to use logical operations
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
     // colorBlendInfo.blendConstants[0] = 0.0f;
     // colorBlendInfo.blendConstants[1] = 0.0f;
     // colorBlendInfo.blendConstants[2] = 0.0f;
     // colorBlendInfo.blendConstants[3] = 0.0f;


     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   VERTEX INPUT   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    // vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data(); //(data spacing / stride info)
    // vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    // vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   INPUT ASSEMBLY   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
     //mInputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
     // // really only useful when using "strip" topology
     //mInputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   RASTERIZATION   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   MULTISAMPLING   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    multiSamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DEPTH STENCIL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   DYNAMIC RENDERING   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

  }// --------------------------------------------------------- PipelineBuilder::buildPipeline (_) --- (END)//


   // NOTE: this will copy each push constant range
   // TODO sub for more efficient emplace
  void FcPipelineConfigInfo2::addPushConstants(VkPushConstantRange pushConstant)
  {
    pushConstantsInfo.push_back(pushConstant);
  }


  void FcPipelineConfigInfo2::setInputTopology(VkPrimitiveTopology topology)
  {
    inputAssemblyInfo.topology = topology;
     // we wont be using primitive restart here so leave it false
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
  }


  void FcPipelineConfigInfo2::setPolygonMode(VkPolygonMode mode)
  {
    rasterizationInfo.polygonMode = mode;
    rasterizationInfo.lineWidth = 1.f;
  }


  void FcPipelineConfigInfo2::setCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
  {
    rasterizationInfo.cullMode = cullMode;
    rasterizationInfo.frontFace = frontFace;
  }


  void FcPipelineConfigInfo2::setMultiSampling(VkSampleCountFlagBits sampleCount)
  {
    multiSamplingInfo.sampleShadingEnable = VK_FALSE;
     //
    multiSamplingInfo.rasterizationSamples = sampleCount;
    multiSamplingInfo.minSampleShading = 1.0f;
    multiSamplingInfo.pSampleMask = nullptr;
     // no alpha to coverage either
    multiSamplingInfo.alphaToCoverageEnable = VK_FALSE;
    multiSamplingInfo.alphaToOneEnable = VK_FALSE;

  }


  void FcPipelineConfigInfo2::setColorAttachment(VkFormat format)
  {
    colorAttachmentFormat = format;
     // connect the format to the renderInfo structure
    renderInfo.colorAttachmentCount = 1;
    renderInfo.pColorAttachmentFormats = &colorAttachmentFormat;
  }


  void FcPipelineConfigInfo2::setDepthFormat(VkFormat format)
  {
    renderInfo.depthAttachmentFormat = format;
  }


   // TODO Should just make this the default
  void FcPipelineConfigInfo2::disableDepthtest()
  {
    depthStencilInfo.depthTestEnable = VK_FALSE;
    depthStencilInfo.depthWriteEnable = VK_FALSE;
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_NEVER;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.stencilTestEnable = VK_FALSE;
    depthStencilInfo.front = {};
    depthStencilInfo.back = {};
    depthStencilInfo.minDepthBounds = 0.f;
    depthStencilInfo.maxDepthBounds = 1.f;
  }

  void FcPipelineConfigInfo2::enableDepthtest(bool depthWriteEnable, VkCompareOp op)
  {
    depthStencilInfo.depthTestEnable = VK_TRUE;
    depthStencilInfo.depthWriteEnable = depthWriteEnable;
    depthStencilInfo.depthCompareOp = op;
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilInfo.stencilTestEnable = VK_FALSE;
    depthStencilInfo.front = {};
    depthStencilInfo.back = {};
    depthStencilInfo.minDepthBounds = 0.f;
    depthStencilInfo.maxDepthBounds = 1.f;
  }


  void FcPipelineConfigInfo2::disableBlending()
  {
     // default write mask
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
     // no blending
    colorBlendAttachment.blendEnable = VK_FALSE;
  }

  void FcPipelineConfigInfo2::enableBlendingAdditive()
  {
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
  }

  void FcPipelineConfigInfo2::enableBlendingAlpha()
  {
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
  }


// DEFAULT configuration info for creating a pipeline
  PipelineConfigInfo::PipelineConfigInfo()
  {
     // INPUT ASSEMBLY
    inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyInfo.primitiveRestartEnable = VK_FALSE; // really only useful when using "strip" topology

     // VIEWPORT
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = nullptr;//&viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = nullptr;//&scissor;

     // -- RASTERIZER --
    rasterizationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizationInfo.depthClampEnable = VK_FALSE; // change if fragment beyond near/far planes are clipped (default) or clamp to far plane
    rasterizationInfo.rasterizerDiscardEnable = VK_FALSE; // whether to discard data and skip rasterizer. never creates fragments--only suitable for pipeline without framebuffer output
    rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL; // how to handle filling the points between vertices
    rasterizationInfo.lineWidth = 1.0f; // how thick lines should be when drawn
    rasterizationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizationInfo.depthBiasEnable = VK_FALSE; // whether to add depth bias to fragments (to reduce shadow acne)
    rasterizationInfo.depthBiasConstantFactor = 0.0f;
    rasterizationInfo.depthBiasClamp = 0.0f;
    rasterizationInfo.depthBiasSlopeFactor = 0.0f;

     // -- MULTISAMPLING --

     // multisampling is not for textures - it really only works on the edges of shapes
    multiSamplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSamplingInfo.sampleShadingEnable = VK_FALSE;
    multiSamplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // number of samples to use per fragment
    multiSamplingInfo.minSampleShading = 1.0f;
    multiSamplingInfo.pSampleMask = nullptr;
    multiSamplingInfo.alphaToCoverageEnable = VK_FALSE;
    multiSamplingInfo.alphaToOneEnable = VK_FALSE;




     // -- COLOR BLENDING --
     // TODO may need tweaking here with defaults
     // Blend attachment state (how blending is handled)
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                                          | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT; // colors to apply blending to
    colorBlendAttachment.blendEnable = VK_TRUE; // enable blending
     // blending uses equation: (srcColorBlendFactor * newColor) colorBlendOp (dstColorBlendFacto * oldColor)
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

     // blending decides how to blend a new color being written to a fragment, with the old value
    colorBlendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
     //configInfo.colorBlendInfo.logicOp = VK_LOGIC_OP_NAND; // not used since we will be using math instead of logic
    colorBlendInfo.logicOpEnable = VK_FALSE; // alternative to calculations is to use logical operations
    colorBlendInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendInfo.attachmentCount = 1;
    colorBlendInfo.pAttachments = &colorBlendAttachment;
    colorBlendInfo.blendConstants[0] = 0.0f;
    colorBlendInfo.blendConstants[1] = 0.0f;
    colorBlendInfo.blendConstants[2] = 0.0f;
    colorBlendInfo.blendConstants[3] = 0.0f;

     // -- DEPTH STENCIL TESTING -- //
     //
    depthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilInfo.depthTestEnable = VK_TRUE; // enable checking depth to determine fragment write
    depthStencilInfo.depthWriteEnable = VK_TRUE; // enable writing to depth buffer to replace old values
    depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS; // comparison operation that allows an overwrite (is in front)
    depthStencilInfo.depthBoundsTestEnable = VK_FALSE; // Depth Bounds Test: Does the depth value exist between the two user definable bounds
    depthStencilInfo.minDepthBounds = 0.0f;
    depthStencilInfo.maxDepthBounds = 1.0f;
    depthStencilInfo.stencilTestEnable = VK_FALSE;
    depthStencilInfo.front = {};
    depthStencilInfo.back = {};

     // -- DYNAMIC STATES --

     // allow for window resizing on the fly (without recreating the pipeline)
     //dynamic viewport : can resize in command buffer with vkCmdSetViewPort // dynamic scissor : can resize in command buffer with vkCmdSetScissor
    dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
     //
    dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    dynamicStateInfo.pDynamicStates = dynamicStateEnables.data();
    dynamicStateInfo.flags = 0;

     // TODO create a non-dynamic viewport and scissors in the case that the window cannot be resized
     //  // create  a viewport info structure
     // VkViewport viewport{};
     // viewport.x = 0.0f;
     // viewport.y = 0.0f;

     // VkExtent2D surfaceExtent = swapchain.getSurfaceExtent();

     // viewport.width = static_cast<float>(surfaceExtent.width);
     // viewport.height = static_cast<float>(surfaceExtent.height);
     // viewport.minDepth = 0.0f;
     // viewport.maxDepth = 1.0f;

     //  // create a scissor info struct
     // VkRect2D scissor{};
     // scissor.offset = {0,0};           // offset to use region from
     // scissor.extent = surfaceExtent; // extent to describe region to use, starting at offset

     //-- VERTEX INPUT --

     // how the data for a single vertex (including info such as position, color, texture coords, normals, etc) is as a whole
    bindingDescriptions.resize(1);
    bindingDescriptions[0].binding = 0;                             // can bind multiple streams of data, this defines which one
    bindingDescriptions[0].stride = sizeof(Vertex);                 // size of a single vertex object
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // how to move between data after each vertex
                                                                // VK_VERTEX_INPUT_RATE_VERTEX : move on to the next vertex
                                                                // VK_VERTEX_INPUT_RATE_INSTANCE : move to a vertex for the next instance

     // How the data for an attribute is defined within a vertex
    attributeDescriptions.resize(4);

     // position attribute
     // which binding the data is at (should be the same as above unless you have multiple streams of data)
    attributeDescriptions[0].binding = 0;
     // location in shader where data will be read from
    attributeDescriptions[0].location = 0;
    // format the data will take (also helps define the size of the data)
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    // where this attribute is defined in the data for a single vertex
    attributeDescriptions[0].offset = offsetof(Vertex, position);

     // color attribute
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);

     // normals attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, normal);

     // texture attribute
    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(Vertex, texCoord);
  }


  void FcPipeline::bind(VkCommandBuffer commandBuffer)
  {
    vkCmdBindPipeline(commandBuffer, mBindPoint, mPipeline);
  }

  void FcPipeline::create2(FcPipelineCreateInfo* pipelineInfo)
  {
     // -*-*-*-*-*-*-*-*-*-*-*-*-   CREATE PIPELINE LAYOUT   -*-*-*-*-*-*-*-*-*-*-*-*- //

    // std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { FcLocator::DescriptorClerk().UboSetLayout()
    //                                                             , FcLocator::DescriptorClerk().SamplerSetLayout() };

    mName = pipelineInfo->name;

    VkPushConstantRange pushConstants{};
    pushConstants.offset = 0;
    pushConstants.size = sizeof(ComputePushConstants);
    pushConstants.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstants;

    pipelineLayoutInfo.setLayoutCount = 1;
     //pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
     //pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutInfo.pSetLayouts = FcLocator::DescriptorClerk().vkDescriptorLayout();

     // create the pipeline layout
    if (vkCreatePipelineLayout(FcLocator::Device(), &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Pipeline Layout!");
    }

// std::string& vertShaderFilename, const std::string& fragShaderFilename, const PipelineConfigInfo& pipelineInfo)
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE SHADERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     // read in SPIR-V code of shaders
     // TODO add ability to specify filename as relative path and load the absolute path or
     // perhaps a small loader program that finds file paths for everything

     // allocate and initialize ( = {}) enough shader state create infos for each stage
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(pipelineInfo->shaders.size()
                                                              , VkPipelineShaderStageCreateInfo{});

    std::vector<VkShaderModule> shaderModules(shaderStages.size());

    for (uint32_t i = 0; i < shaderStages.size(); i++)
    {
       // create the shader modules from our SPIR-V code
      auto shaderCode = readFile("shaders/" + pipelineInfo->shaders[i].filename);
      shaderModules[i] = createShaderModule(shaderCode);

       // populate the shader stage create info
      shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStages[i].stage = pipelineInfo->shaders[i].stageFlag;
      shaderStages[i].module = shaderModules[i];
      shaderStages[i].pName = "main";
    }

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE PIPELINE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    assert(mPipelineLayout != nullptr && "Cannot create pipeline before pipeline layout");

    VkComputePipelineCreateInfo computePipelineInfo = {};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.layout = mPipelineLayout;
     // TODO integrate graphics pipeline creation with compute, probably best to just create separate call for compute
    computePipelineInfo.stage = shaderStages[0];

    if (vkCreateComputePipelines(FcLocator::Device(), VK_NULL_HANDLE, 1,
                                  &computePipelineInfo, nullptr, &mPipeline) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Vulkan Graphics Pipeline!");
    }

     // Destroy shader modules, no longer needed after pipeline created
    for (VkShaderModule& shaderModule : shaderModules)
    {
      vkDestroyShaderModule(FcLocator::Device(), shaderModule, nullptr);
    }
  }

   // * TODO * Preferred method! Destroy all other methods and configs
  void FcPipeline::create3(FcPipelineConfigInfo2& pipelineConfig)
  {
    fcLog("FcPipeline::create3() called");
     // -*-*-*-*-*-*-*-*-*-*-*-*-   CREATE PIPELINE LAYOUT   -*-*-*-*-*-*-*-*-*-*-*-*- //

     // std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts =
     //   { FcLocator::DescriptorClerk().UboSetLayout()
     //   , FcLocator::DescriptorClerk().SamplerSetLayout() };

     // save a pointer to the device instance
    VkDevice pDevice = FcLocator::Device();

    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pipelineConfig.pushConstantsInfo.size());
    layoutInfo.pPushConstantRanges = pipelineConfig.pushConstantsInfo.data();

     // TODO make consistent and delete variable size
    layoutInfo.setLayoutCount = 1;
     //layoutInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
     //layoutInfo.pSetLayouts = descriptorSetLayouts.data();
    layoutInfo.pSetLayouts = FcLocator::DescriptorClerk().vkDescriptorLayout();

     // create the pipeline layout
    if (vkCreatePipelineLayout(pDevice, &layoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Pipeline Layout!");
    }

    mName = pipelineConfig.name;

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE SHADERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     // read in SPIR-V code of shaders
     // TODO add ability to specify filename as relative path and load the absolute path or
     // perhaps a small loader program that finds file paths for everything
     // allocate and initialize ( = {}) enough shader state create infos for each stage
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages(pipelineConfig.shaders.size()
                                                              , VkPipelineShaderStageCreateInfo{});

    std::vector<VkShaderModule> shaderModules(shaderStages.size());

     // TODO provide the ability to add any stages and figure out systematically
    for (uint32_t i = 0; i < shaderStages.size(); i++)
    {
       // create the shader modules from our SPIR-V code
      auto shaderCode = readFile("shaders/" + pipelineConfig.shaders[i].filename);
      shaderModules[i] = createShaderModule(shaderCode);

       // populate the shader stage create info
      shaderStages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStages[i].stage = pipelineConfig.shaders[i].stageFlag;
      shaderStages[i].module = shaderModules[i];
      shaderStages[i].pName = "main";
    }

     // TODO check to see if this is necessary to accomplish that an empty binding/attribute vector uses null for vertexInputInfo
     // auto& bindingDescriptions = pipelineConfig.bindingDescriptions;
     // auto& attributeDescriptions = pipelineConfig.attributeDescriptions;

     // TODO might be best to just create separate call for compute pipeline
     // Determine if we should build a graphics pipeline or a compute
    if (pipelineConfig.shaders[0].stageFlag == VK_SHADER_STAGE_COMPUTE_BIT)
    {
    fcLog("Creating Compute Pipeline");
      VkComputePipelineCreateInfo computePipelineInfo = {};
      computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
      computePipelineInfo.layout = mPipelineLayout;
      computePipelineInfo.stage = shaderStages[0];

      if (vkCreateComputePipelines(FcLocator::Device(), VK_NULL_HANDLE, 1,
                                   &computePipelineInfo, nullptr, &mPipeline) != VK_SUCCESS)
      {
        throw std::runtime_error("Failed to create Vulkan Compute Pipeline!");
      }

      mBindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
    }
     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   GRAPHICS PIPELINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    else
    {
       //mBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
      fcLog("Creating Graphics Pipeline");
      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   DYNAMIC STATE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

      VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
      dynamicStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
      dynamicStateInfo.pDynamicStates = &state[0];
      dynamicStateInfo.dynamicStateCount = 2;


       // -*-*-*-*-*-*-*-*-*-*-*-   GRAPHICS PIPELINE CREATION   -*-*-*-*-*-*-*-*-*-*-*- //
      VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
      graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
      graphicsPipelineInfo.pNext = &pipelineConfig.renderInfo;
      graphicsPipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
      graphicsPipelineInfo.pStages = shaderStages.data();
      graphicsPipelineInfo.pVertexInputState = &pipelineConfig.vertexInputInfo;
      graphicsPipelineInfo.pInputAssemblyState = &pipelineConfig.inputAssemblyInfo;
      graphicsPipelineInfo.pViewportState = &pipelineConfig.viewportInfo;
      graphicsPipelineInfo.pDynamicState = &dynamicStateInfo;
      graphicsPipelineInfo.pRasterizationState = &pipelineConfig.rasterizationInfo;
      graphicsPipelineInfo.pMultisampleState = &pipelineConfig.multiSamplingInfo;
      graphicsPipelineInfo.pColorBlendState = &pipelineConfig.colorBlendInfo;
      graphicsPipelineInfo.pDepthStencilState = &pipelineConfig.depthStencilInfo;
      graphicsPipelineInfo.layout = mPipelineLayout;

       // TODO Can create multiple pipelines that derive from one another for optimisation
      graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
       // index of pipeline being created to derive from (for optimisation of creating multiple pipelines)
      graphicsPipelineInfo.basePipelineIndex = -1;

       // it's easy to error out on the create graphics pipeline call so handle better
      if (vkCreateGraphicsPipelines(pDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &mPipeline)
          != VK_SUCCESS)
      {
        std::cout << "failed to create pipeline" << std::endl;
         //       return  VK_NULL_HANDLE; or -1, etc.
         //throw std::runtime_error("Failed to create Vulkan Graphics Pipeline!");
      }
    }
     // Destroy shader modules, no longer needed after pipeline created
    for (uint32_t i = 0; i < shaderStages.size(); i++)
    {
      vkDestroyShaderModule(pDevice, shaderModules[i], nullptr);
    }

  }


  void FcPipeline::create(const std::string& vertShaderFilename, const std::string& fragShaderFilename
                          , const PipelineConfigInfo& pipelineInfo)
  {

    assert(pipelineInfo.pipelineLayout != VK_NULL_HANDLE &&
           "Cannot create graphics pipeline:: no pipiplineLayout provided in configInfo");
    assert(pipelineInfo.renderPass != VK_NULL_HANDLE &&
           "Cannot create graphics pipeline:: no renderPass provided in configInfo");

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE SHADERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

     // read in SPIR-V code of shaders
     //TODO add ability to specify filename as relative path and load the absolute path






    auto vertexShaderCode = readFile("shaders/" + vertShaderFilename);
    auto fragmentShaderCode = readFile("shaders/" + fragShaderFilename);

     // create the shader modules from our SPIR-V code
    VkShaderModule vertexShaderModule = createShaderModule(vertexShaderCode);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShaderCode);

     // vertex stage create info
    VkPipelineShaderStageCreateInfo vertexShaderInfo{};
    vertexShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderInfo.module = vertexShaderModule;
    vertexShaderInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentShaderInfo{};
    fragmentShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentShaderInfo.module = fragmentShaderModule;
    fragmentShaderInfo.pName = "main";

     // put shader stage creation info into an array (required by pipeline create info)
    VkPipelineShaderStageCreateInfo shaderStages[2] = {vertexShaderInfo, fragmentShaderInfo};

    // TODO check to see if this is necessary to accomplish that an empty binding/attribute vector uses null for vertexInputInfo
    auto& bindingDescriptions = pipelineInfo.bindingDescriptions;
    auto& attributeDescriptions = pipelineInfo.attributeDescriptions;

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data(); //(data spacing / stride info)
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

     // -- GRAPHICS PIPELINE CREATION --

    VkGraphicsPipelineCreateInfo graphicsPipelineInfo{};
    graphicsPipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineInfo.stageCount = 2;                         // number of shader stages
    graphicsPipelineInfo.pStages = shaderStages;                 // list of shader stages
    graphicsPipelineInfo.pVertexInputState = &vertexInputInfo;
    graphicsPipelineInfo.pInputAssemblyState = &pipelineInfo.inputAssemblyInfo;
    graphicsPipelineInfo.pViewportState = &pipelineInfo.viewportInfo;
    graphicsPipelineInfo.pDynamicState = &pipelineInfo.dynamicStateInfo;
    graphicsPipelineInfo.pRasterizationState = &pipelineInfo.rasterizationInfo;
    graphicsPipelineInfo.pMultisampleState = &pipelineInfo.multiSamplingInfo;
    graphicsPipelineInfo.pColorBlendState = &pipelineInfo.colorBlendInfo;
    graphicsPipelineInfo.pDepthStencilState = &pipelineInfo.depthStencilInfo;
    graphicsPipelineInfo.layout = pipelineInfo.pipelineLayout;   // pipeline layout pipeline will use
    graphicsPipelineInfo.renderPass = pipelineInfo.renderPass; // render pass description the pipeline is compatible with
    graphicsPipelineInfo.subpass = pipelineInfo.subpass;                            // subpass of render pass to use with pipeline
    graphicsPipelineInfo.basePipelineHandle = VK_NULL_HANDLE;    // Can create multiple pipelines that derive from one another for optimisation
     // index of pipeline being created to derive from (for optimisation of creating multiple pipelines)
    graphicsPipelineInfo.basePipelineIndex = -1;                 // index of pipeline being created to derive from (for optimisation of creating multiple pipelines)

    // save a pointer to the device instance
    VkDevice pDevice = FcLocator::Device();

    if (vkCreateGraphicsPipelines(pDevice, VK_NULL_HANDLE, 1, &graphicsPipelineInfo, nullptr, &mPipeline) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Vulkan Graphics Pipeline!");
    }

     // Destroy shader modules, no longer needed after pipeline created
    vkDestroyShaderModule(pDevice, vertexShaderModule, nullptr);
    vkDestroyShaderModule(pDevice, fragmentShaderModule, nullptr);
  }



  void FcPipeline::destroy()
  {
    std::cout << "calling: FcPipeline::destroy" << std::endl;

    if (mPipelineLayout != nullptr)
    {
      vkDestroyPipelineLayout(FcLocator::Device(), mPipelineLayout, nullptr);
    }

    if (mPipeline != nullptr)
    {
      vkDestroyPipeline(FcLocator::Device(), mPipeline, nullptr);
    }
  }


   // TODO rewrite to pass shader module in and maybe return a bool for sucess and delete runtime error
  VkShaderModule FcPipeline::createShaderModule(const std::vector<char>& code)
  {
    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
     // BUG note that in vkguide.dev, they multiply the following by * sizeof(uint32_t)
     // I think this is wrong on their end but should find out for sure and notify if so.
    shaderInfo.codeSize = code.size();
     // might be better to see if there's a way to not use reinterpret -> check vkguide.dev
    shaderInfo.pCode = reinterpret_cast<const uint32_t* >(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(FcLocator::Device(), &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Shader Module!");
    }

    return shaderModule;
  }// END  createShaderModule(...)

} //  namespace fc -END-
