#include "fc_skybox.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_frame_assets.hpp"
#include "fc_descriptors.hpp"
#include "fc_locator.hpp"
#include "fc_defaults.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  //
  void FcSkybox::init(VkDescriptorSetLayout sceneDescriptorLayout,
                      std::vector<FrameAssets>& frames)
  {
    FcPipelineConfig pipelineConfig;
    pipelineConfig.name = "skybox";
    pipelineConfig.addStage(VK_SHADER_STAGE_VERTEX_BIT, "skybox.vert.spv");
    pipelineConfig.addStage(VK_SHADER_STAGE_FRAGMENT_BIT, "skybox.frag.spv");

    // Configure pipeline operation parameters
    pipelineConfig.disableMultiSampling();
    pipelineConfig.enableDepthtest(VK_FALSE, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // add the scene descriptor set layout (eye, view, proj, etc.)
    pipelineConfig.addDescriptorSetLayout(sceneDescriptorLayout);

    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

    // create and then add the second descriptor set
    FcDescriptorBindInfo bindInfo{};
    VkDescriptorSetLayout descriptorLayout;

    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    //
    descriptorLayout = descClerk.createDescriptorSetLayout(bindInfo);
    //
    bindInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mCubeImage
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                         , FcDefaults::Samplers.Linear);
    //
    pipelineConfig.addDescriptorSetLayout(descriptorLayout);

    // Allocate a descriptorSet to each frame buffer
    for (FrameAssets& frame : frames)
    {
      frame.skyBoxDescriptorSet = descClerk.createDescriptorSet(descriptorLayout, bindInfo);
    }

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
  //
  void FcSkybox::draw(VkCommandBuffer cmd, FrameAssets& currentFrame)
  {
    // first draw the skycube
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.getVkPipeline());

    // ?? Would binding both descriptors simultaneously be a better practice
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.Layout()
                            , 0, 1, &currentFrame.sceneDataDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.Layout()
                            , 1, 1, &currentFrame.skyBoxDescriptorSet, 0, nullptr);
    //
    vkCmdDraw(cmd, 36, 1, 0, 0);
  }
}
