#include "fc_billboard_render_system.hpp"


// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_descriptors.hpp"
#include "core/fc_frame_assets.hpp"
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


  [[deprecated("Delete soon")]]
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
    }
  }

  void FcBillboard::loadTexture(std::string filename, VkDescriptorSetLayout layout)
  {
     // TODO implement
  }


  void FcBillboard::loadTexture(VkDescriptorSetLayout layout)
  {
     // TODO implement
  }



  void FcBillboardRenderer::createPipeline()
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
    pushConstantRange.size = sizeof(BillboardPushes);

    billboardConfig.addPushConstants(pushConstantRange);

    billboardConfig.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);

    // MUST signal vulkan that we won't be reading any vertex data
    billboardConfig.disableVertexReading();

    mPipeline.create(billboardConfig);
  }


  void FcBillboardRenderer::sortBillboardsByDistance(glm::vec3& cameraPosition)
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



  // void FcBillboardRenderer::draw(VkCommandBuffer cmd, std::vector<FcBillboard>& billboards,
  //                                glm::vec3 cameraPosition, FrameAssets& currentFrame)
  // {
  //   // sort the billboards by distance to the camera
  //   // TODO sort within update instead
  //   std::multimap<float, size_t> sortedIndices; // TODO?? uint32 or size_t
  //   for (size_t i = 0; i < billboards.size(); ++i)
  //   {
  //     // calculate distance
  //     auto distance = ubo.eye - billboards[i].PushComponent().position;
  //     float distanceSquared = glm::dot(distance, distance);
  //     sortedIndices.insert(std::pair(distanceSquared, i));
  //   }

  //   // bind pipeline to be used in render pass
  //   mPipeline.bind(cmd);

  //   // iterate through billboards in reverse order (to draw them back to front)
  //   for (auto index = sortedIndices.rbegin(); index != sortedIndices.rend(); ++index )
  //   {
  //     vkCmdPushConstants(cmd, mPipeline.Layout()
  //                        , VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(BillboardPushes)
  //                        , &billboards[index->second].PushComponent());

  //     //VkDeviceSize offsets[] = { 0 };
  //     // vkCmdBindVertexBuffers(mCommandBuffers[swapChainImageIndex], 0, 1, &font.VertexBuffer(), offsets);
  //     // vkCmdBindIndexBuffer(mCommandBuffers[swapChainImageIndex], font.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

  //     FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

  //     // TODO  update the global Ubo only once per frame, not each draw call
  //     // descClerk.update(swapchainImageIndex, &ubo);

  //     std::array<VkDescriptorSet, 2> descriptorSets;
  //     descriptorSets[0] = currentFrame.sceneDataDescriptorSet;
  //     descriptorSets[1] = billboards[index->second].getDescriptor();

  //     vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS
  //                             , mPipeline.Layout(), 0, static_cast<uint32_t>(descriptorSets.size())
  //                             , descriptorSets.data() , 0, nullptr);

  //     //vkCmdDrawIndexed(mCommandBuffers[swapChainImageIndex], font.IndexCount(), 1, 0, 0, 0);
  //     vkCmdDraw(cmd, 6, 1, 0, 0);
  //   }
  // }

} // _END_ namespace fc
