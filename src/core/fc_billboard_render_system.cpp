#include "fc_billboard_render_system.hpp"


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_descriptors.hpp"
#include "core/fc_game_object.hpp"
#include "core/fc_gpu.hpp"
#include "core/fc_text.hpp"
#include "core/fc_locator.hpp"
#include "core/fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/utilities.hpp"
#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STD   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <algorithm>
#include <array>
#include <map>
#include <utility>
#include <vector>


namespace fc
{

  FcBillboard::FcBillboard(float width, float height, glm::vec4 color)
  {
    mPush.color  = color;
    mPush.width = width;
    mPush.height = height;

     //placeInHandleTable();
  }




  void FcBillboard::placeInHandleTable()
  {
    std::vector<FcBillboard* >& billboardList = FcLocator::Billboards();

     // first check to see if there's already a slot available that's just been set to nullptr
    for(size_t i = 0; i < billboardList.size(); i++)
    {
      if (billboardList[i] == nullptr)
      {
        billboardList[i] = this;
        mHandleIndex = i;

        // don't think we need uniqueId in this handle system since all will be lights

        return;
      }
    }

     // if no slots are vacant, grow the vector of game objects as long as it doesn't exceed the maximum
     // TODO add error code to handle too big of vector
    if (billboardList.size() < MAX_BILLBOARDS)
    {
      billboardList.push_back(this);
      mHandleIndex = billboardList.size() - 1;
    }
    else
    {
       // if no open slots are found, return the last index, TODO should save last element for special case (ie. invalid index);
      mHandleIndex = MAX_BILLBOARDS;
      std::cout << "ERROR: too many billboards!" << std::endl;
       // BUG dangling pointers and such!!!
       // TODO make sure to delete the light since
    }
  }



  void FcBillboardRenderSystem::createPipeline(FcPipeline& pipeline, VkRenderPass& renderPass)
  {
     // -- CREATE PIPELINE LAYOUT -- //

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BillboardPushComponent);

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

    pipeline.create("billboard.vert.spv", "billboard.frag.spv", pipelineConfig);
  }


  void FcBillboardRenderSystem::sortBillboardsByDistance(glm::vec3& cameraPosition)
  {
    // std::vector<FcBillboard* >& billboards = FcLocator::Billboards();

    // for (size_t i = 0; i < billboards.size(); ++i)
    // {

    // }

    // std::sort(std::begin(billboards), std::end(billboards)
    //           , [](FcBillboard& lhs, FcBillboard& rhs, glm::vec3& cameraPosition)
    //            {
    //              auto tempDist = glm::vec4(cameraPosition, 1.f) - lhs.Push().position;
    //              float distanceToLhs = glm::dot(tempDist, tempDist);

    //              tempDist

    //                }
    //           );
  }

} // _END_ namespace fc
