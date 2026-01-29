//>  fc_draw_collection.hpp  <//
#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC CORE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
/* #include "core/fc_renderer.hpp" */
#include "core/fc_resource_pool.hpp"
#include "core/fc_resources.hpp"
#include "fc_surface.hpp"
#include "fc_image.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc
{
  /* typedef u32 FcHandle; */
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
     uint32_t fpsAvg;
     uint32_t triangleCount;
     uint32_t objectsRendered;
     uint32_t frame;
  };


  // TODO think about expanding this to a class and to include just a single material and
  // a separate vector or FcSurfaces...
  // TODO could potentially keep pointers to surfaces instead and use those to draw that
  // way the draw collection doesn't need updating each time a surface is transformed
  struct FcDrawCollection
  {
     using RenderObject = std::pair<FcMaterial*, std::vector<FcSurface>>;
     // TODO should use a hashmap since we are not accessing via index and need fast access!
     std::vector<RenderObject> opaqueSurfaces;
     std::vector<RenderObject> transparentSurfaces;
     std::vector<std::vector<size_t>> visibleSurfaceIndices;
     ResourcePoolTyped<FcImage> mTextures;
     ResourcePoolTyped<FcImage> mBillboards;
     FcStats stats;
     uint32_t numSurfaces{0};

     /* void addTexture(const FcImage& texture); */
     std::vector<ResourceUpdate> bindlessTextureUpdates;

     void init(FcAllocator* allocator);
     const FcSurface& getSurfaceAtIndex(uint32_t surfaceIndex);
     void add(FcMeshNode* surface);
     void DELETEadd(FcMeshNode* surface);
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
    //    uint32_t totalOpaqueSurfaces();
    //    uint32_t totalTransparentSurfaces();
    // };
}// --- namespace fc --- (END)
