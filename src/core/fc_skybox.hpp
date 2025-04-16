// fc_skybox.hpp
#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC CORE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_image.hpp"
#include "fc_buffer.hpp"
#include "fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include "glm/vec3.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <filesystem>


namespace fc
{
  class FrameData;
// DEL
  // struct CubeVertex
  // {
  //    glm::vec3 position;
  //    //glm::vec3 normal;
  //    //glm::vec4 color; // not needed with pbr
  //    // TODO could add some features like a print function, etc.
  // };



  class FcSkybox
  {
   private:
// set up vertex data (and buffer(s)) and configure vertex attributes
     // ------------------------------------------------------------------
     // TODO DELETE
       float skyboxVertices[108] = {
        // positions
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

     FcImage mCubeImage;
     /* VkDescriptorSetLayout mDescriptorLayout; */
     /* VkDescriptorSet mDescriptor; */
     FcPipeline mPipeline;
     VkSampler mCubeMapSampler;
     FcBuffer mVertexBuffer;
     FcBuffer mIndexBuffer;
     //void initPipeline();
   public:
     FcSkybox();
     void loadTextures(std::string parentPath, std::string extension);
     void loadTextures(std::vector<std::filesystem::path>& filenames);
//     void loadTextures(std::array<std::filesystem::path, 6>& filenames);
     // TODO try and remove sceneDescriptorLayout
     void init(VkDescriptorSetLayout sceneDescriptorLayout, std::vector<FrameData>& frames);
     void draw(VkCommandBuffer cmd, FrameData& currentFrame);
     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   GETTERS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     const FcImage& Image() { return mCubeImage; }
     /* const VkDescriptorSetLayout DescriptorLayout() { return mDescriptorLayout; } */
     /* const VkDescriptorSet Descriptor() { return mDescriptor; } */
};



}// --- namespace --- (END)
