//>--- fc_draw_collection.cpp ---<//
#include "fc_draw_collection.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_mesh.hpp"
#include "fc_types.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  void FcDrawCollection::init(FcAllocator* allocator)
  {
    // TODO more flexible initialization of draw collection
    // TODO implement with actual data pool (as opposed to object pool)
    mTextures.init(512);
  }

  // TODO update to work on all surfaces (opaque or transparent)
  // TODO utilize a numbering system to simplify load/indices use
  const FcSubmesh& FcDrawCollection::getSurfaceAtIndex(uint32_t surfaceIndex)
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


  //
  //
  void FcDrawCollection::add(FcMeshNode* node)
  {
    using RenderObject = std::pair<FcMaterial*, std::vector<FcSubmesh>>;

    for (FcSubmesh& subMesh : node->mMesh->SubMeshes())
    {
      // First figure out if this material subMesh will belong in transparent or opaque pipeline
      std::vector<RenderObject>* selectedCollection;

      if (subMesh.material->materialType == FcMaterial::Type::Transparent)
      {
        selectedCollection = &transparentSurfaces;
      } else {
        selectedCollection = &opaqueSurfaces;
      }

      subMesh.node = node;

      // Find the material if it's already in the collection
      bool materialFound = false;

      for (RenderObject& renderObj : *selectedCollection)
      {
        if (subMesh.material.get() == renderObj.first)
        {
          // store the submesh into the draw collection
          renderObj.second.push_back(subMesh);
          materialFound = true;
          break;
        }
      }

      // Add a new material/subMesh pair if we didn't find it in the draw collection
      if (!materialFound)
      {
        RenderObject newRenderObj;
        newRenderObj.first = subMesh.material.get();

        newRenderObj.second.emplace_back(subMesh);

        selectedCollection->push_back(newRenderObj);

        // Grow the vector of vectors that keeps track of which meshes to draw (visible)
        visibleSurfaceIndices.resize(opaqueSurfaces.size());
      }

      numSurfaces++;
    }

    // allocate enough lists in visible surfaces vector to store indices for each material
    // TODO should probably resize in chunks
  }


  //
  //
  void FcDrawCollection::destroy()
  {
    FC_DEBUG_LOG("Destroying FcDrawCollection...");

    mTextures.clear();

    FC_DEBUG_LOG("...DONE");
  }


}// --- namespace fc --- (END)
