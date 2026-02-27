//>> fc_types.hpp <//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_resources.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_core.h>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcMesh; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  // TODO handle this differently
  const int MAX_OBJECTS = 30;
  const int MAX_LIGHTS = 10;
  const int MAX_BILLBOARDS = 30;

  // Index locations of queue families
  struct QueueFamilyIndices
  {
     int graphicsFamily = -1;
     int presentationFamily = -1;
      // check if queue family indices are valid
     inline const bool isValid() const
      {
        return graphicsFamily >= 0 && presentationFamily >= 0;
      }
  };




  // TODO define only if using bindless
  // Used for signalling to the fragment shader which features are available
  enum class MaterialFeatures : uint32_t
  {
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


  // TEST see if initialization is necessary since usually we know they must be set
  // I've seen recommendations to have a binding for anything needed for calculating
  // gl_Position, and a second binding for everything else. So hardware that does a pre-pass
  // to bin by position only has to touch the relevant half of the data.
  struct Vertex
  {
     glm::vec3 position;
     float uv_x;
     glm::vec3 normal;
     float uv_y;
     //glm::vec4 color; // not really needed with pbr
     glm::vec4 tangent;
     // TODO could add some features like a print function, etc.
  };


  //
  //
  // TODO this could be more intuitively named
  struct FcMaterial
  {
     // TODO merge with MaterialFeatures
     enum class Type : uint8_t
     {
       	Opaque,
	Transparent,
	Other,
     };
     // FIXME the vkDescriptorSet may be better placed within FcSurface instead of FcSubmesh??
     VkDescriptorSet materialSet;
     Type materialType;
  };


  //
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

  // TODO MOVE to types
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



  //
  //
  struct TerrainPushConstants
  {
     VkDeviceAddress address;
     float time;
  };

  //
  struct BillboardPushConstants
  {
     // BillboardPushes are not created directly but instead are the values that the shader
     // will recieve from pushing an Billboard object with vkCmdPushConstants and using this
     // struct as the size of the data sent. Billboard must have the first members exactly
     // match this structure. This will save us from having to copy to a separate structure
     // every time
     BillboardPushConstants() = delete;
     glm::vec3 position;
     float width;
     float height;
     u32 textureIndex;
  };
  //
  //

} // namespace fc _END_
