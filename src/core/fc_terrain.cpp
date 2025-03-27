#include "fc_terrain.hpp"

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include "core/utilities.hpp"
#include "fc_renderer.hpp"
#include "fc_locator.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include <stb_image.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include <filesystem>
#include <glm/ext/matrix_clip_space.hpp>


namespace fc
{
  void FcTerrain::init(FcRenderer* renderer, std::filesystem::path filename)
  {
    pRenderer = renderer;

    // std::vector<glm::ivec2> tests(64 * 64);
    // for(size_t i = 0; i < 64; ++i)
    // {
    //   for (size_t j = 0; j < 64; ++j)
    //   {
    //     tests[i + j * 64] = glm::vec2{i, j};
    //   }
    // }

    // std::function<int(glm::ivec2)> lambda([](glm::ivec2 vector)
    //  {
    //    uint32_t numPatches = 64;
    //    uint32_t width = 1001;
    //    uint32_t scale = width / numPatches;
    //    glm::ivec2 rpos = vector;
    //    int x = vector.x;
    //    int y = vector.y;

    //    bool method1 = false;
    //    if (method1)
    //    {
    //      // rpos = (x, y) * (8, 8)
    //      rpos *= glm::ivec2(scale);

    //      // rpos.x = clamp(rpos.x, 0, 511)
    //      rpos.x = std::max(0, std::min(rpos.x, static_cast<int>(width) - 1));
    //      // rpos.x = clamp(rpos.y, 0, 511)
    //      rpos.y = std::max(0, std::min(rpos.y, static_cast<int>(width) - 1));
    //      // rpos = rpos / 8
    //      rpos /= glm::ivec2(scale);
    //      // rpos = data[(rpos.x + rpos.y * 512) * 8] / float(MAX_UINT_32)
    //      return (x + y * width) * scale;
    //    }

    //    if (!method1)
    //    {
    //      x *= scale;
    //      y *= scale;
    //      return (x + y * width);
    //    }
    //  });

    // printIOtable(tests, lambda);

    createSampler();
    loadHeightmap(filename, 64);
    // TODO Must reinitialize pipelines if new heightmap is loaded it seems
    initPipelines();

    // Initialize uniform buffer object

    ubo.tessellationFactor = 0.75f;
    // TODO allow tess factor to be set to zero to allow passthrough
    // TODO document -> desired size of the tessellated quad patch edge
    ubo.tessellatedEdgeSize = 20.0f;
    ubo.displacementFactor = 32.0f;
    // TODO clarify screen dim vs screen pix
    //ubo.viewportDim = {renderer->ScreenWidth(), renderer->ScreenHeight()};
    ubo.viewportDim = {1200, 900};
    mModelTransform = glm::mat4{1.0f};
  }

  // TODO DELETE after creating sampler atlas
  void FcTerrain::createSampler()
  {
     VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // How to render when image is magnified on the screen
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    // How to render when image is minified on the screen
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    // How to handle wrap in the U (x) direction
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    // How to handle wrap in the V (y) direction
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    // How to handle wrap in the W (z) direction
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    // Border beyond texture (when clamp to border is used--good for shadow maps)
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    // WILL USE NORMALIZED COORD. (coords will be between 0-1)
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    // Mipmap interpolation mode (between two levels of mipmaps)
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    // used to force vulkan to use lower level of detail and mip level
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

    // maximum level of detail to pick mip level
    samplerInfo.maxLod = 0.0f;
    // enable anisotropy
    samplerInfo.anisotropyEnable = VK_FALSE;
    // TODO should allow this to be user definable or at least profiled at install/runtime
    // Amount of anisotropic samples being taken
    samplerInfo.maxAnisotropy = VK_SAMPLE_COUNT_1_BIT;

    FcGpu& gpu = FcLocator::Gpu();
    if (vkCreateSampler(gpu.getVkDevice(), &samplerInfo, nullptr, &mHeightMapSampler) != VK_SUCCESS)
    {
      throw std::runtime_error("Failed to create a Vulkan Texture Sampler!");
    }
  }

  void FcTerrain::initPipelines()
  {
    FcPipelineConfig terrainPipeline{4};
    terrainPipeline.name = "Terrain Pipeline";
    terrainPipeline.shaders[0].stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
    terrainPipeline.shaders[0].filename = "terrain.vert.spv";
    terrainPipeline.shaders[1].stageFlag = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    terrainPipeline.shaders[1].filename = "terrain.tesc.spv";
    terrainPipeline.shaders[2].stageFlag = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    terrainPipeline.shaders[2].filename = "terrain.tese.spv";
    terrainPipeline.shaders[3].stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
    terrainPipeline.shaders[3].filename = "terrain.frag.spv";

    // Setup pipeline parameters
    terrainPipeline.setColorAttachment(VK_FORMAT_R16G16B16A16_SFLOAT);

    // ?? Needed if disabling depthtest
    terrainPipeline.setDepthFormat(VK_FORMAT_D32_SFLOAT);
    //terrainPipeline.disableDepthtest();

    terrainPipeline.setInputTopology(VK_PRIMITIVE_TOPOLOGY_PATCH_LIST);
    terrainPipeline.setPolygonMode(VK_POLYGON_MODE_FILL);
    /* terrainPipeline.setPolygonMode(VK_POLYGON_MODE_LINE); */
    terrainPipeline.setCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

    terrainPipeline.enableDepthtest(VK_TRUE, VK_COMPARE_OP_GREATER_OR_EQUAL);
    terrainPipeline.setMultiSampling(FcLocator::Gpu().Properties().maxMsaaSamples);
    terrainPipeline.disableBlending();
    terrainPipeline.enableTessellationShader(4);

    // add Vertex shader push constants
    VkPushConstantRange vertexShaderPCs;
    vertexShaderPCs.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    vertexShaderPCs.offset = 0;
    // ?? BUG this may be minimum alignment needed DELETE if VERTEXBUFFERPCs is not needed
    vertexShaderPCs.size = sizeof(VertexBufferPCs);
    //
    terrainPipeline.addPushConstants(vertexShaderPCs);

    // TODO should no longer pass model matrix but could pass a different matrix perhaps
    // and cut down on ubo if that matters
    VkPushConstantRange tessellationShaderPCs;
    tessellationShaderPCs.stageFlags = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

    tessellationShaderPCs.offset = sizeof(VertexBufferPCs);
    tessellationShaderPCs.size = sizeof(glm::mat4);
    //
    terrainPipeline.addPushConstants(tessellationShaderPCs);

    // Setup descriptor sets
    FcDescriptorBindInfo bindInfo{};

    // set up buffer
    mUboBuffer.allocateBuffer(sizeof(UBO), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    // TODO do in one functioncall overload if need be but first check to see if ever separate
    bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                        , VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
                        | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

    bindInfo.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mUboBuffer, sizeof(UBO), 0);

    bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                        , VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
                        | VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

    bindInfo.attachImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mHeightMap
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mHeightMapSampler);

    FcDescriptorClerk& deskClerk = FcLocator::DescriptorClerk();
    // TODO do in one function call overload
    mHeightMapDescriptorLayout = deskClerk.createDescriptorSetLayout(bindInfo);
    mHeightMapDescriptor = deskClerk.createDescriptorSet(mHeightMapDescriptorLayout
                                                         , bindInfo);

    terrainPipeline.addDescriptorSetLayout(mHeightMapDescriptorLayout);

    // Create the Mesh pipeline
    mPipeline.create(terrainPipeline);

    // Create wireframe pipeline
    // TODO check for capability
    if (true)
    {
      terrainPipeline.setPolygonMode(VK_POLYGON_MODE_LINE);
      mWireframePipeline.create(terrainPipeline);
    }

  }

  // TODO place check or automatically delete previously loaded heightmap
  void FcTerrain::loadHeightmap(std::filesystem::path filename, uint32_t numPatches)
  {
    // TODO allow for deleting current height map and loading new one
    mHeightMap.loadKtx(filename);//, VK_FORMAT_R16_UNORM);
    //mHeightMap.loadTestImage(512, 512);

    mNumPatches = numPatches;


    generateTerrain();
  }

  // TODO document lots / full class
  void FcTerrain::generateTerrain()
  {
    // // TODO allow to pass in the values for scale and shift during initialization
    // // Height in texel will be in range [0,255], yScale will convert that range to [0.0f,64.0f]
    // float yScale = 64.f / 256.f;
    // // Shift will be applied to give us the final range we want to work with [-16.f, 48.f]
    // float yShift = 16.f;

    // Create a grid of mNumPatches quads
    //constexpr uint32_t mNumPatches = 64;

    uint32_t numVertices = mNumPatches * mNumPatches;
    const float uvScale = 1.0f;

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   GENERATE VERTICES   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    std::vector<Vertex> vertices(numVertices);

    const float wx = 2.0f;
    const float wy = 2.0f;

    //     // TODO
    //     // float oneHalf = 1.f/2.f;
    //     // float oneOverRez = 1.f / (float)rez;

    uint32_t index;
    for(uint32_t x = 0; x < mNumPatches; ++x)
    {
      // TODO change to z or diff index that is more informative
      for(uint32_t y = 0; y < mNumPatches; ++y)
      {
        index = x + y * mNumPatches;

        vertices[index].position.x =
          x * wx + wx / 2.0f - static_cast<float>(mNumPatches) * wx / 2.0f;
        // Y is always 0 since it will be determined within the shader
        vertices[index].position.y = 0.0f;

        vertices[index].position.z =
          y * wy + wy / 2.0f - static_cast<float>(mNumPatches) * wy / 2.0f;

        vertices[index].uv_x = static_cast<float>(x) / mNumPatches * uvScale;
        vertices[index].uv_y = static_cast<float>(y) / mNumPatches * uvScale;
      }
    }

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   CALCULATE NORMALS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    mHeightMap.copyToCPUAddress();
    //FcLog log1("log2", true);

    // We break the heigt map down into mNumPatches grid and then sample from those patches
    int pixelStepLength = mHeightMap.size().width / mNumPatches;
    int offsetX = 0; // the true x coord of pixel we want to sample from
    int offsetY = 0; // the true y coord of pixel we want to sample from
    uint16_t pixel;
    //
    for(int x = 0; x < mNumPatches; ++x)
    {
      for(int y = 0; y < mNumPatches; ++y)
      {
        // Get height samples centered around current position using a sobel filter
        float heights[3][3];
        for(int hx = -1; hx <= 1; ++hx)
        {
          for(int hy = -1; hy <= 1; ++hy)
          {
            int offsetX = (x + hx) * pixelStepLength;
            int offsetY = (y + hy) * pixelStepLength;

            mHeightMap.fetchPixel(offsetX, offsetY, pixel);
            /* pixel = mHeightMap.saschaFetchPixel(offsetX, offsetY, pixelStepLength); */
            // Normalize pixel value to be in [0,1] range
            heights[hx + 1][hy + 1] = pixel / static_cast<double>(UINT16_MAX);

            // USED FOR TESTING pixel values, DELETE eventually
            // if (true)//pixel != 0)
            // {
            // log1 << "x: " << offsetX << ", y: "  << offsetY
            //      << " | Raw: (" << pixel << ")"
            //      << " | Normalized: (" << heights[hx + 1][hy + 1] << ")"
            //   // << " | PixelVal x: " << (pixel >> 16) <<  " y: " << (pixel & UINT16_MAX)
            //   << "\n";
            // }
          }
        }
        // calculate the normal
        glm::vec3 normal;

	// Gx sobel filter
        normal.x = heights[0][0] - heights[2][0] + 2.0f * heights[0][1]
                   - 2.0f * heights[2][1] + heights[0][2] - heights[2][2];
        // Gy sobel filter
        normal.z = heights[0][0] + 2.0f * heights[1][0] + heights[2][0] - heights[0][2]
                   - 2.0f * heights[1][2] - heights[2][2];
        // Calculate missing up componet of the normal using the filtered x and y axis
        // The first value constrols the bump?? strength
        normal.y = 0.25f * sqrt(1.0f - normal.x * normal.x - normal.z * normal.z);

        vertices[x + y * mNumPatches].normal = glm::normalize(normal * glm::vec3(2.f, 1.f, 2.f));
      }
    }

    // no longer need the mapped data
    mHeightMap.destroyCpuCopy();
    //log1.closeLogOutput();

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   GENERATE INDICES   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    uint32_t w = mNumPatches - 1;
    mNumIndices = w * w * 4;
    std::vector<uint32_t> indices(mNumIndices);

    for(uint32_t x = 0; x < w; ++x)
    {
      for(uint32_t y = 0; y < w; ++y)
      {
        index = (x + y * w) * 4;
        indices[index] = x + y * mNumPatches;
        indices[index + 1] = indices[index] + mNumPatches;
        indices[index + 2] = indices[index + 1] + 1;
        indices[index + 3] = indices[index] + 1;
      }
    }

    // Create the final mesh
    mMesh.uploadMesh(std::span(vertices), std::span(indices));
  }


  void FcTerrain::update(FcFrustum& frustum)
  {
    // update the frustum after view has potentially changed
    // TODO make sure we aren't duplicating operations anywhere with m, mv, mvp, vp, etc.
    memcpy(ubo.frustumPlanes, frustum.Planes().data(), sizeof(glm::vec4) * 6);
  }


  void FcTerrain::draw(VkCommandBuffer cmd, SceneData* pSceneData, bool drawWireframe)
  {

    ubo.modelView = pSceneData->view * mModelTransform;
    ubo.projection = pSceneData->projection;
    ubo.modelViewProj = ubo.projection * ubo.modelView;
    // TODO might prefer to update the frustum here instead

    //printMat(ubo.modelViewProj);
    mUboBuffer.overwriteData(&ubo, sizeof(UBO));

    if (drawWireframe)
    {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS
                        , mWireframePipeline.getVkPipeline());
    }
    else
    {
      vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS
                        , mPipeline.getVkPipeline());
    }

    // TODO abstract bind descriptor sets to make less error prone!
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline.Layout()
                            , 0, 1, &mHeightMapDescriptor, 0, nullptr);

    // TODO abstract to mesh class
    vkCmdBindIndexBuffer(cmd, mMesh.IndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    VertexBufferPCs pushConstants;
    pushConstants.address = mMesh.VertexBufferAddress();
    // TODO simp
    pushConstants.padding = mMesh.VertexBufferAddress();

    // BUG
    vkCmdPushConstants(cmd, mPipeline.Layout(), VK_SHADER_STAGE_VERTEX_BIT
                       , 0, sizeof(VertexBufferPCs), &pushConstants);

    // DELETE and relocate model to sceneData eventually
    glm::mat4 model = glm::mat4(1.0f);

    // BUG no longer need to pass model here
    vkCmdPushConstants(cmd, mPipeline.Layout()
                       , VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT
                       , sizeof(VertexBufferPCs), sizeof(glm::mat4), &model);

    vkCmdDrawIndexed(cmd, mNumIndices, 1, 0, 0, 0);


    // render the mesh triangle strip by triangle strip - each row at a time
//     for (size_t strip = 0; strip < mNumStrips; ++strip)
//     {
// //      std::cout << "drawing strip: " << strip << std::endl;
//       vkCmdDrawIndexed(cmd, mNumVertsPerStrip, 1, mNumVertsPerStrip * strip, 0, 0);

//     }
  }// --- FcTerrain::draw (_) --- (END)

}// --- namespace fc --- (END)
