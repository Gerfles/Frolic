// fc_scene_renderer.hpp
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_model.hpp"
#include "fc_descriptors.hpp"
#include "fc_pipeline.hpp"
// #include "fc_image.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include <vulkan/vulkan_handles.hpp>
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
//#include <cstdint>


namespace fc
{
  class FcRenderer;
  class FcImage;
  class FrameAssets;
  class FcMeshNode;
  class FcSurface;

  // Useful for signalling to the fragment shader which features are available
  enum class MaterialFeatures : uint32_t {
    HasColorTexture = 0b1 << 0,
    HasNormalTexture = 0b1 << 1,
    HasRoughMetalTexture = 0b1 << 2,
    HasOcclusionTexture = 0b1 << 3,
    HasEmissiveTexture = 0b1 << 4,
    HasVertexTangentAttribute = 0b1 << 5,
    HasVertexTextureCoordinates = 0b1 << 6,
  };
  MaterialFeatures operator~ (MaterialFeatures const & rhs);
  MaterialFeatures operator|(MaterialFeatures lhs, MaterialFeatures rhs);
  MaterialFeatures operator&(MaterialFeatures lhs, MaterialFeatures rhs);
  MaterialFeatures& operator|=(MaterialFeatures& lhs, MaterialFeatures const& rhs);
  MaterialFeatures& operator&=(MaterialFeatures& lhs, MaterialFeatures const& rhs);


  // TODO Can incrementally update push constants according to:
  // https://docs.vulkan.org/guide/latest/push_constants.html
  // May provide some benefits if done correctly
  // TODO this may be too much data for most push constants
  struct DrawPushConstants
  {
     glm::mat4 worldMatrix;
     glm::mat4 normalTransform;
     VkDeviceAddress vertexBuffer;
  };


  // TODO unitialize maybe
  struct SceneDataUbo
  {
     glm::vec4 eye {0.0};
     glm::mat4 view {1.f};
     glm::mat4 projection {1.f};
     glm::mat4 viewProj{1.f};
     glm::mat4 lighSpaceTransform{1.f};
     glm::vec4 ambientLight {1.f, 1.f, 1.f, 0.1f}; // w is light intensity
     glm::vec4 sunlightDirection; // w for power
     glm::vec4 sunlightColor;
  };

  // TODO separate this out from above to pass separately (could have minor benifits in some systems)
  struct modelData
  {
     glm::mat4 model{1.f};
     glm::mat4 viewProj{1.f};
  };


  // ?? TODO could pack these tighter but need to study the downfalls, etc.
  // https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Memory_layout
  struct alignas(16) MaterialConstants
  {
     // TODO create two structs for material constants, one for full feature rendering and
     // raytracing and one for actual game engine.
     glm::vec4 colorFactors;
     glm::vec4 metalRoughFactors;
     glm::vec4 emmisiveFactors; // w = emmisive strength
     //
     float  occlusionFactor;
     float iorF0;
     MaterialFeatures flags{0};
     // Vulkan has a minimum requirement for uniform buffer alignment.
     // glm::vec4 padding[14]; // We'll use 256 bytes as its a fairly universal gpu target
  };


  // TODO relocate or rename fc_materials.hpp
  class FcSceneRenderer
  {
   private:
     FcPipeline mOpaquePipeline;
     FcPipeline mTransparentPipeline;
     std::vector<uint32_t> mSortedObjectIndices;
     VkDescriptorSetLayout mMaterialDescriptorLayout;
     FcPipeline* pCurrentPipeline;
     /* FcMaterial* mPreviousMaterial; */
     VkBuffer mPreviousIndexBuffer;
     float expansionFactor{0};
     VkDescriptorSetLayout mSceneDataDescriptorLayout;
     // TODO should probably pass this in each frame
     SceneDataUbo mSceneData;
     FcBuffer mSceneDataBuffer;
     void sortByVisibility(FcDrawCollection& drawCollection);
     uint32_t drawMeshNode(VkCommandBuffer cmd,
                           const FcMeshNode& surface,
                           FrameAssets& currentFrame);
     void drawSurface(VkCommandBuffer cmd, const FcSurface& surface);
   public:
     // TODO think about including a local descriptorClerk
     void init(std::vector<FrameAssets>& frames);
     void buildPipelines(FcRenderer* renderer);
     FcPipeline* TransparentPipeline() { return &mTransparentPipeline; }
     FcPipeline* OpaquePipeline() { return &mOpaquePipeline; }
     void clearResources(VkDevice device);
     void draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection, FrameAssets& currentFrame);
     // TODO DELETE or refactor the following
     VkDescriptorSetLayout getSceneDescriptorLayout() { return mSceneDataDescriptorLayout; }
     SceneDataUbo* getSceneDataUbo() { return &mSceneData; }
     void updateSceneDataBuffer() { mSceneDataBuffer.write(&mSceneData, sizeof(SceneDataUbo)); }
     float& ExpansionFactor() { return expansionFactor; }
};


}// --- namespace fc --- (END)
