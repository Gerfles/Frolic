//> fc_draw_collection.cpp <//

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC CORE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_draw_collection.hpp"
#include "core/log.hpp"
#include "fc_image.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc
{
  void FcDrawCollection::init(FcAllocator* allocator)
  {
    // TODO more flexible initialization of draw collection
    mTextures.init(allocator, 512);
    mBillboards.init(allocator, 512);
  }

  // TODO update to work on all surfaces (opaque or transparent)
  const FcSurface& FcDrawCollection::getSurfaceAtIndex(uint32_t surfaceIndex)
  {
    // Figure out where to find desired surface since it should be per surface not per meshNode
    size_t materialIndex = 0;
    for (auto& material : opaqueSurfaces)
    {
      size_t numSurfaces =  material.second.size();
      if (surfaceIndex < numSurfaces)
      {
        break;
      }
      ++materialIndex;
      surfaceIndex -= numSurfaces;
    }

    return opaqueSurfaces[materialIndex].second[surfaceIndex];
  }


  // void FcDrawCollection::addTexture(const FcImage& texture)
  // {
  //   if (mIsBindlessSupported)
  //   {

  //   }
  // }

// uint32_t FcDrawCollection::totalOpaqueSurfaces()
  // {
  //   uint32_t tab = 0;
  //   for (FcMeshNode& meshNode : opaqueSurfaces)
  //   {
  //     tab += meshNode.mMesh->Surfaces().size();
  //   }

  //   return tab;
  // }


  // uint32_t FcDrawCollection::totalTransparentSurfaces()
  // {
  //   uint32_t tab = 0;
  //   for (FcMeshNode& meshNode : transparentSurfaces)
  //   {
  //     tab += meshNode.mMesh->Surfaces().size();
  //   }
  //   return tab;
  // }

  //
  //
  void FcDrawCollection::destroy()
  {
    // TODO check that this works appropriately for FcImage type
    mTextures.freeAll();
    mTextures.shutdown();

    mBillboards.freeAll();
    mBillboards.shutdown();
  }


}// --- namespace fc --- (END)
