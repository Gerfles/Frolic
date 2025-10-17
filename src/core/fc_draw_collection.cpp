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
}// --- namespace fc --- (END)
