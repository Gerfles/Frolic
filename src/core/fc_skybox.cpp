//>--- fc_skybox.cpp ---<//
#include "fc_skybox.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_frame_assets.hpp"
#include "fc_descriptors.hpp"
#include "fc_locator.hpp"
#include "fc_defaults.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  void FcSkybox::init(const FcBuffer& sceneDataBuffer)
  {
    FcPipelineConfig pipelineConfig;
    pipelineConfig.name = "skybox";
    pipelineConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "skybox.vert.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "skybox.frag.spv");

    // Configure pipeline operation parameters
    pipelineConfig.disableMultiSampling();
    pipelineConfig.enableDepthtest(VK_FALSE, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // add the scene descriptor set layout (eye, view, proj, etc.)
    pipelineConfig.attachUniformBuffer(0, 0, sceneDataBuffer, sizeof(SceneData), 0, VK_SHADER_STAGE_VERTEX_BIT);
    // TODO (LP) extrapolate VK_stages to create diffent combos, etc, and shorter names
    pipelineConfig.attachImage(0, 1, mCubeImage, FcDefaults::Samplers.Linear, VK_SHADER_STAGE_FRAGMENT_BIT);

    // Create the descriptorset we will bind when drawing
    mDescriptorSet = pipelineConfig.createDescriptorSet(0);

    mPipeline.create(pipelineConfig);
  }


  // Must provide a file path and then the load will do the rest assumming things are
  // named correctly as top.jpg or top.png, etc.
  // 1st = right, 2nd = left, 3rd = top, 4th = bottom, 5th = front, 6th = back
  void FcSkybox::loadTextures(std::string_view parentPath, std::string_view extension)
  {
    // TODO add asserts such as
    // if (parentPath.has_extension())
    // {
    //   std::string extension = parentPath.extension().string();
    // }

    std::string sides[NUM_SIDES_CUBE] = {"right", "left", "top", "bottom", "front", "back"};
    std::vector<std::string> filenames(NUM_SIDES_CUBE);
    for (size_t i = 0; i < NUM_SIDES_CUBE; ++i)
    {
      filenames[i] = std::string(parentPath) + "//" + sides[i] + std::string(extension);
    }

    mCubeImage.loadMultipleLayers(filenames, FcImageTypes::Cubemap);
  }


  //
  // Use when the files are not named acording to the required conventions above
  void FcSkybox::loadTextures(std::vector<std::string>& filenames)
  {
    // TODO add asserts such as
    // if (parentPath.has_extension())
    // {
    //   std::string extension = parentPath.extension().string();
    // }
    mCubeImage.loadMultipleLayers(filenames, FcImageTypes::Cubemap);
  }


  //
  void FcSkybox::draw(VkCommandBuffer cmd, FcDescriptorCollection& currentFrame)
  {
    //
    mPipeline.bind(cmd);
    mPipeline.bindDescriptorSet(cmd, mDescriptorSet, 0);

    //
    vkCmdDraw(cmd, 36, 1, 0, 0);
  }


  //
  void FcSkybox::destroy()
  {
    mCubeImage.destroy();
    mPipeline.destroy();
    vkDestroySampler(FcLocator::Device(), mCubeMapSampler, nullptr);
  }
}
