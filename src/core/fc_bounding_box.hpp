// fc_bounding_box.hpp
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  class FcDrawCollection;
  class FrameAssets;
  class FcSubmesh;

  struct BoundingBoxPushes
  {
     glm::mat4 modelMatrix;
     glm::vec4 origin;
     glm::vec4 extents;
  };

  struct FcBounds
  {
     glm::vec3 origin;
     float sphereRadius;
     glm::vec3 extents;
  };

  //
  //
  class FcBoundaryBox
  {
   private:
     std::array<glm::vec4, 8> mCorners;

   public:
     void init(const FcBounds& bounds) noexcept;
     inline const glm::vec4& operator[](size_t index) const noexcept { return mCorners[index]; }
  };


  //
  //
  // TODO extrapolate renderSubsystem into base class...
  class FcBoundingBoxRenderer
  {
   private:
     FcPipeline mBoundingBoxPipeline;
   public:
     void buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout) noexcept;
     // TODO make draw call draw only the isolated mesh whose bounding box we want
     void draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection
               ,FrameAssets& currentFrame, int boundingBoxID = -1) noexcept;
     void drawSurface(VkCommandBuffer cmd, const FcSubmesh& subMesh) noexcept;
     void destroy() noexcept;
  };

}// --- namespace fc --- (END)
