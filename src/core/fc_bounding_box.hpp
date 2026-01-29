// fc_bounding_box.hpp
#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
/* #include "fc_draw_collection.hpp" */
#include "fc_frame_assets.hpp"
#include "fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <array>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  class FcDrawCollection;

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
     void init(FcBounds& bounds);
     inline const glm::vec4& operator[](size_t index) const { return mCorners[index]; }
  };

  // TODO extrapolate renderSubsystem into base class...
  class FcBoundingBoxRenderer
  {
   private:
     FcPipeline mBoundingBoxPipeline;
   public:
     void buildPipelines(VkDescriptorSetLayout sceneDescriptorLayout);
     void draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection
               ,FrameAssets& currentFrame, int boundingBoxID = -1);
     void destroy();
  };

}// --- namespace fc --- (END)
