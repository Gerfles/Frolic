// fc_bounding_box.hpp
#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
/* #include "fc_draw_collection.hpp" */
#include "fc_frame_assets.hpp"
#include "fc_pipeline.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

namespace fc
{
  class FcDrawCollection;

  struct BoundingBoxPushes
  {
     glm::mat4 modelMatrix;
     glm::vec4 origin;
     glm::vec4 extents;
  };

  struct Bounds
  {
     glm::vec3 origin;
     float sphereRadius;
     glm::vec3 extents;
  };


  class FcBoundingBoxRenderer
  {
   private:
     FcPipeline mBoundingBoxPipeline;
   public:
     void initPipelines(VkDescriptorSetLayout sceneDescriptorLayout);
     void draw(VkCommandBuffer cmd, FcDrawCollection& drawCollection
               ,FrameAssets& currentFrame, int boundingBoxID = -1);
     void destroy();
  };

}// --- namespace fc --- (END)
