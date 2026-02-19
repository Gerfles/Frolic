//>_ fc_scene_renderer.hpp _<//
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_resources.hpp"
#include "fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/ext/matrix_float4x4.hpp>
#include <vulkan/vulkan_core.h>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECLS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  class FcRenderer;
  class FcImage;
  class FcMesh;
  class FcMaterial;
  class FrameAssets;
  class FcMeshNode;
  class FcMesh;
  class FcDrawCollection;
  class FcSubmesh;

  // TODO define only if using bindless
  // Used for signalling to the fragment shader which features are available
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


  //
  // TODO unitialize maybe ??
  struct SceneDataUbo
  {
     glm::vec4 eye;
     glm::mat4 view;
     glm::mat4 projection;
     glm::mat4 viewProj;
     glm::mat4 lighSpaceTransform;
     glm::vec4 sunlightDirection; // w for power
  };

  // TODO separate this out from above to pass separately (could have minor benifits in some systems)
  struct modelData
  {
     glm::mat4 model{1.f};
     glm::mat4 viewProj{1.f};
  };

  // ?? TODO could pack these tighter but need to study the downfalls, etc.
  // https://www.khronos.org/opengl/wiki/Interface_Block_(GLSL)#Memory_layout
  // struct alignas(16) MaterialConstants
  struct MaterialConstants
  {
     // TODO create two structs for material constants, one for full feature rendering and
     // raytracing and one for actual game engine.
     glm::vec4 colorFactors;
     glm::vec4 metalRoughFactors;
     glm::vec4 emmisiveFactors; // w = emmisive strength
     float occlusionFactor {1.0f};
     float iorF0;
     uint32_t colorIndex {INVALID_TEXTURE_INDEX};
     uint32_t normalIndex {INVALID_TEXTURE_INDEX};
     uint32_t metalRoughIndex {INVALID_TEXTURE_INDEX};
     uint32_t occlusionIndex {INVALID_TEXTURE_INDEX};
     uint32_t emissiveIndex {INVALID_TEXTURE_INDEX};
     MaterialFeatures flags{0};
     // Vulkan (glsl) has a minimum requirement for uniform buffer alignment. We'll use
     // 128 bytes but 256 bytes might be a more universal gpu target.
     glm::vec4 padding[3];
  };


  // -*-*-*-*-*-*-*-*-*-*-*-*-*-   DRAW INDIRECT DATA   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
  struct IndirectBatch
  {
     FcMesh* mesh;
     FcMaterial* material;
     uint32_t first;
     uint32_t count;
  };



  // TODO relocate or rename fc_materials.hpp
    class FcSceneRenderer
    {
     private:
       FcPipeline mOpaquePipeline;
       FcPipeline mTransparentPipeline;
       FcPipeline mWireframePipeline;
       std::vector<uint32_t> mSortedObjectIndices;
       VkDescriptorSetLayout mMaterialDescriptorLayout;
       FcPipeline* pCurrentPipeline;
       glm::mat4* pViewProjection;
       /* FcMaterial* mPreviousMaterial; */
       VkBuffer mPreviousIndexBuffer;
       float expansionFactor{0};
       /* VkDescriptorSetLayout mSceneDataDescriptorLayout; */
       // TODO should probably pass this in each frame
       // SceneDataUbo mSceneData;
       // FcBuffer mSceneDataBuffer;
       std::vector<VkDescriptorSet> mExternalDescriptors {3};
       void sortByVisibility(FcDrawCollection& drawCollection);
       uint32_t drawMeshNode(VkCommandBuffer cmd,
                             const FcMeshNode& surface,
                             FrameAssets& currentFrame);
       void drawSurface(VkCommandBuffer cmd, const FcSubmesh& surface) noexcept;
       void buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout, std::vector<FrameAssets>& frames);
     public:
       // TODO think about including a local descriptorClerk
       void init(VkDescriptorSetLayout sceneDescriptorLayout, glm::mat4& viewProj, std::vector<FrameAssets>& frames);
       FcPipeline* TransparentPipeline() { return &mTransparentPipeline; }
       FcPipeline* OpaquePipeline() { return &mOpaquePipeline; }
       //
       void draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection,
                 FrameAssets& currentFrame, bool shouldDrawWireFrame);
       //
       void destroy();
       float& ExpansionFactor() { return expansionFactor; }
    };


}// --- namespace fc --- (END)
