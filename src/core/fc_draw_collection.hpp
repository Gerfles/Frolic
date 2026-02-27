//>  fc_draw_collection.hpp  <//
#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC CORE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_resource_pool.hpp"
#include "core/fc_resources.hpp"
#include "fc_image.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECL'   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcMaterial; class FcSubmesh; class FcMeshNode; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  using FcHandle = u32;

  //
  // TODO either eliminate or expand usefulness
  struct ResourceUpdate
  {
     ResourceDeletionType type;
     FcHandle handle;
     u32 currentFrame;
  };


  struct FcStats
  {
     float frametime;
     float sceneUpdateTime;
     float meshDrawTime;
     u32 fpsAvg;
     u32 triangleCount;
     u32 objectsRendered;
     u32 frame;
  };


  // TODO think about expanding this to a class and to include just a single material and
  // a separate vector or FcSurfaces...
  // TODO could potentially keep pointers to surfaces instead and use those to draw that
  // way the draw collection doesn't need updating each time a surface is transformed
  struct FcDrawCollection
  {
     // This structure is structured in a way that allows a given material to keep track
     // of all of the surfaces that require it to draw. That way we can simply bind a
     // material and then draw all of the surfaces associated with that material before
     // moving the the next material...
     using RenderObject = std::pair<FcMaterial*, std::vector<FcSubmesh>>;
     // TODO should use a hashmap since we are not accessing via index and need fast access!
     std::vector<RenderObject> opaqueSurfaces;
     std::vector<RenderObject> transparentSurfaces;
     std::vector<std::vector<size_t>> visibleSurfaceIndices;
     ResourcePoolTyped<FcImage> mTextures;
     ResourcePoolTyped<FcImage> mBillboards;
     FcStats stats;
     u32 numSurfaces{0};

     std::vector<ResourceUpdate> bindlessTextureUpdates;

     void init(FcAllocator* allocator);
     const FcSubmesh& getSurfaceAtIndex(u32 surfaceIndex);
     void add(FcMeshNode* node);
     void destroy();
  };

  // TODO create a class that is a collection of objects to draw with a particular pipeline
  // and descriptorSets etc, that way we can just render all those objects and then only
  // bind DSs and pipelines at the start of the draw call
    // struct FcDrawCollection
    // {
    //    std::vector<std::reference_wrapper<FcMeshNode>> opaqueSurfaces;
    //    std::vector<std::reference_wrapper<FcMeshNode>> transparentSurfaces;
    //    FcStats stats;
    //    //
    //    u32 totalOpaqueSurfaces();
    //    u32 totalTransparentSurfaces();
    // };
}// --- namespace fc --- (END)
