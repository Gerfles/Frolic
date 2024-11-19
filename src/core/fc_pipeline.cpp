#include "fc_pipeline.hpp"

// - FROLIC ENGINE -
#include "core/fc_descriptors.hpp"
#include "core/fc_locator.hpp"
#include "fc_swapChain.hpp"
#include "utilities.hpp"
// - EXTERNAL LIBRARIES -
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <stdexcept>
#include <vector>
// #include <cstddef>
//#include <type_traits>

namespace fc
{
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
    attributeDescriptions[0].binding = 0;                         // which binding the data is at (should be the same as above unless you have multiple streams of data)
    attributeDescriptions[0].location = 0;                        // location in shader where data will be read from
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // format the data will take (also helps define the size of the data)
    attributeDescriptions[0].offset = offsetof(Vertex, position);      // where this attribute is defined in the data for a single vertex

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
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
  }



  void FcPipeline::create(const std::string& vertShaderFilename, const std::string& fragShaderFilename
                          , const PipelineConfigInfo& pipelineInfo)//, const FcSwapChain& swapchain)
  {

    assert(pipelineInfo.pipelineLayout != VK_NULL_HANDLE &&
           "Cannot create graphics pipeline:: no pipiplineLayout provided in configInfo");
    assert(pipelineInfo.renderPass != VK_NULL_HANDLE &&
           "Cannot create graphics pipeline:: no renderPass provided in configInfo");

     //-- CREATE SHADERS -- //

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



  VkShaderModule FcPipeline::createShaderModule(const std::vector<char>& code)
  {
    VkShaderModuleCreateInfo shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderInfo.codeSize = code.size();
    shaderInfo.pCode = reinterpret_cast<const uint32_t* >(code.data());


    VkShaderModule shaderModule;
    if (vkCreateShaderModule(FcLocator::Device(), &shaderInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create Shader Module!");
    }

    return shaderModule;
  }// END  createShaderModule(...)



} //  namespace fc -END-
