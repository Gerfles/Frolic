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

  void FcBillboard::loadTexture(std::string filename, VkDescriptorSetLayout layout, FcBindingInfo& bindInfo)
  {
     // TODO implement
  }


  void FcBillboard::loadTexture(VkDescriptorSetLayout layout, FcBindingInfo& bindInfo)
  {
     // TODO implement
  }




  [[deprecated("No longer valid")]]
  void FcBillboardRenderSystem::createPipeline(FcPipeline& pipeline)
  {
    FcPipelineConfig billboardConfig{2};
    billboardConfig.name = "Billboard";
    billboardConfig.shaders[0].filename = "billboard.vert.spv";
    billboardConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    billboardConfig.shaders[1].filename = "bilboard.frag.spv";
    billboardConfig.shaders[1].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(BillboardPushComponent);

    billboardConfig.addPushConstants(pushConstantRange);

    billboardConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);

    // make sure to clear these out since we won't be reading any vertex data and MUST signal vulkan as such
    // pipelineConfig.bindingDescriptions.clear();
    // pipelineConfig.attributeDescriptions.clear();
    // billboardConfig.disableVertexRendering();

    pipeline.create3(billboardConfig);
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
