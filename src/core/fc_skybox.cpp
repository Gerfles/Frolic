#include "fc_skybox.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_frame_assets.hpp"
#include "fc_descriptors.hpp"
#include "fc_locator.hpp"
#include "fc_defaults.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //



namespace fc
{
  FcSkybox::FcSkybox()
  {
  }


  void FcSkybox::init(VkDescriptorSetLayout sceneDescriptorLayout, std::vector<FrameAssets>& frames)
  {
    // Load default sampler
    //VkDeviceSize bufferSize = sizeof(CubeVertex) * 36;
    VkDeviceSize bufferSize = sizeof(skyboxVertices[0]) * 108;
    mVertexBuffer.allocate(bufferSize, FcBufferTypes::Vertex);
    mVertexBuffer.write(skyboxVertices, bufferSize);

    // mVertexBuffer.storeData(skyboxVertices, bufferSize,
    //                         // TODO don't need
    //                         VK_BUFFER_USAGE_TRANSFER_DST_BIT
    //                         | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    //                         | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

    FcPipelineConfig pipelineConfig{2};
    pipelineConfig.name = "skybox";
    pipelineConfig.shaders[0].filename = "skybox.vert.spv";
    pipelineConfig.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    pipelineConfig.shaders[1].filename = "skybox.frag.spv";
    pipelineConfig.shaders[1].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;

    // add the scene descriptor set layout (eye, view, proj, etc.)
    pipelineConfig.addDescriptorSetLayout(sceneDescriptorLayout);

    // create and then add the second descriptor set
    FcDescriptorBindInfo bindInfo{};
    VkDescriptorSetLayout mDescriptorLayout;
    FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
    //
    mDescriptorLayout = descClerk.createDescriptorSetLayout(bindInfo);
    //
    bindInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mCubeImage
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                         , FcDefaults::Samplers.Linear);
    /* mDescriptor = descClerk.createDescriptorSet(mDescriptorLayout, bindInfo); */

    pipelineConfig.addDescriptorSetLayout(mDescriptorLayout);

    // enable the vertex input attributes (position only here)
    pipelineConfig.setVertexInputPositionOnly();

    pipelineConfig.setInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    pipelineConfig.setPolygonMode(VK_POLYGON_MODE_FILL);
    pipelineConfig.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
    pipelineConfig.setMultiSampling(VK_SAMPLE_COUNT_1_BIT);
    pipelineConfig.enableDepthtest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);
    pipelineConfig.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);
    pipelineConfig.setDepthFormat(VK_FORMAT_D32_SFLOAT);

    mPipeline.create(pipelineConfig);


        // Allocate a descriptorSet to each frame buffer
    for (FrameAssets& frame : frames)
    {
      // ?? TODO should create a new Descriptor set per frame instead
      // and may be able to then remove from shadowMap class
      frame.skyBoxDescriptorSet = descClerk.createDescriptorSet(mDescriptorLayout, bindInfo);

      // frame.sceneDataDescriptorSet = descClerk.createDescriptorSet(
      //   mSceneDataDescriptorLayout, sceneDescriptorBinding);
    }
  }


  // Must provide a file path and then the load will do the rest assumming things are named correctly as
  // top.jpg or top.png, etc.
  // 1st = right
  // 2nd = left
  // 3rd = top
  // 4th = bottom
  // 5th = front
  // 6th = back
  void FcSkybox::loadTextures(std::string parentPath, std::string extension)
  {
    // TODO add asserts such as
    // if (parentPath.has_extension())
    // {
    //   std::string extension = parentPath.extension().string();
    // }
    constexpr int NUM_SIDES = 6;
    std::string sides[NUM_SIDES] = {"right", "left", "top", "bottom", "front", "back"};
    std::vector<std::filesystem::path> filenames(NUM_SIDES);
    for (size_t i = 0; i < NUM_SIDES; ++i)
    {
      filenames[i] = parentPath + "//" + sides[i] + extension;
    }

    mCubeImage.loadMultipleLayers(filenames, FcImageTypes::Cubemap);
  }

  // Use when the files are not named acording to the required conventions above
  void FcSkybox::loadTextures(std::vector<std::filesystem::path>& filenames)
  {
    // TODO add asserts such as
    // if (parentPath.has_extension())
    // {
    //   std::string extension = parentPath.extension().string();
    // }
    mCubeImage.loadMultipleLayers(filenames, FcImageTypes::Cubemap);
  }


  void FcSkybox::draw(VkCommandBuffer cmd, FrameAssets& currentFrame)
  {
    // first draw the skycube
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.getVkPipeline());

    // ?? Would binding both descriptors simultaneously be a better practice
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.Layout()
                            , 0, 1, &currentFrame.sceneDataDescriptorSet, 0, nullptr);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.Layout()
                            , 1, 1, &currentFrame.skyBoxDescriptorSet, 0, nullptr);

    // Required to bind vertex buffer
    VkDeviceSize offsets[] = { 0 }; // offsets into buffers being bound
    vkCmdBindVertexBuffers(cmd, 0, 1, &mVertexBuffer.getVkBuffer(), offsets);

    vkCmdDraw(cmd, 36, 1, 0, 0);
  }
}
