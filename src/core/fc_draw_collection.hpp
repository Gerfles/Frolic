// fc_draw_collection.hpp
#pragma once

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_surface.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //



namespace fc
{
  struct FcStats
  {
     float frametime;
     float sceneUpdateTime;
     float meshDrawTime;
     uint32_t fpsAvg;
     uint32_t triangleCount;
     uint32_t objectsRendered;
  };


  // TODO think about expanding this class to include just a single material and
  // a separate vector or FcSurfaces...
  struct FcDrawCollection
  {
     std::vector<std::pair<FcMaterial*, std::vector<FcSurface>>> opaqueSurfaces;
     std::vector<std::vector<size_t>> visibleSurfaceIndices;
     std::vector<std::pair<FcMaterial*, std::vector<FcSurface>>> transparentSurfaces;
     FcStats stats;
     uint32_t numSurfaces{0};
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
