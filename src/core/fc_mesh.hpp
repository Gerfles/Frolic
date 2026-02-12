// TODO rename file or elimiate and distribute objects


#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
/* #include "core/fc_surface.hpp" */
#include "core/platform.hpp"
#include "fc_bounding_box.hpp"
#include "fc_buffer.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "vulkan/vulkan_core.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstdint>
#include <memory>
#include <span>
#include <vector>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

namespace fc
{
  class FcSurface;

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
  //
  struct VertexBufferPushes
  {
     VkDeviceAddress address;
     float time;
  };

} // namespace fc _END_
