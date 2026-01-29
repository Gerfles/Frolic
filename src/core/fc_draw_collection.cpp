//> fc_draw_collection.cpp <//

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC CORE   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_draw_collection.hpp"
#include "core/log.hpp"
#include "fc_image.hpp"
#include "fc_node.hpp"
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

  //
  //
  void FcDrawCollection::add(FcMeshNode* node)
  {
    fcPrintEndl("Adding to Draw Collection");
    using RenderObject = std::pair<FcMaterial*, std::vector<FcSurface>>;

    for (const std::shared_ptr<FcSurface>& subMesh : node->mMesh->mSubMeshes)
    {
      /* const std::shared_ptr<FcSurface> subMesh = node->mMesh; */
      // First figure out if this material subMesh will belong in transparent or opaque pipeline
      std::vector<RenderObject>* selectedCollection;

      if (subMesh->material->materialType == FcMaterial::Type::Transparent)
      {
        selectedCollection = &transparentSurfaces;
      } else {
        selectedCollection = &opaqueSurfaces;
      }

      // BUG this will not work properly as is -> need to store the surfaces into a vector of refrences
      // or smart pointers
      // Find the material if it's already in the collection
      bool materialFound = false;

      for (RenderObject& renderObj : *selectedCollection)
      {
        if (subMesh->material.get() == renderObj.first)
        {
          // store the submesh into the draw collection
          // TODO get rid of init method and do within scene load
          subMesh->init(node);
          renderObj.second.emplace_back(*subMesh);
          materialFound = true;
          break;
        }
      }

      // Add a new material/subMesh pair if we didn't find it in the draw collection
      if (!materialFound)
      {
        RenderObject newRenderObj;
        newRenderObj.first = subMesh->material.get();
        subMesh->init(node);
        newRenderObj.second.emplace_back(*subMesh);
        selectedCollection->push_back(newRenderObj);
      }

      numSurfaces++;
    }


    // allocate enough lists in visible surfaces vector to store indices for each material
    // TODO should probably resize in chunks
    // BUG this seems wrong -> should not need to resize this... at least document why
    visibleSurfaceIndices.resize(opaqueSurfaces.size()+1);

    // recurse down children nodes
    // TODO check the stack frame count to see if this is better handles linearly
    /* FcNode::addToDrawCollection(collection); */
  }



  void FcDrawCollection::DELETEadd(FcMeshNode* node)
  {
    fcPrintEndl("Adding to Draw Collection");
    using RenderObject = std::pair<FcMaterial*, std::vector<FcSurface>>;

    for (const std::shared_ptr<FcSurface>& subMesh : node->mMesh->mSubMeshes)
    {
      // First figure out if this material subMesh will belong in transparent or opaque pipeline
      std::vector<RenderObject>* selectedCollection;
      if (subMesh->material->materialType == FcMaterial::Type::Transparent)
      {
        selectedCollection = &transparentSurfaces;
      } else {
        selectedCollection = &opaqueSurfaces;
      }

      // BUG this will not work properly as is -> need to store the surfaces into a vector of refrences
      // or smart pointers
      // Find the material if it's already in the collection
      bool materialFound = false;
      for (RenderObject& renderObj : *selectedCollection)
      {
        if (subMesh->material.get() == renderObj.first)
        {
          // store the submesh into the draw collection
          // TODO get rid of init method and do within scene load
          subMesh->init(node);
          renderObj.second.emplace_back(*subMesh);
          materialFound = true;
          break;
        }
      }

      // Add a new material/subMesh pair if we didn't find it in the draw collection
      if (!materialFound)
      {
        RenderObject newRenderObj;
        newRenderObj.first = subMesh->material.get();
        subMesh->init(node);
        newRenderObj.second.emplace_back(*subMesh);
        selectedCollection->push_back(newRenderObj);
      }

      numSurfaces++;
    }

    // allocate enough lists in visible surfaces vector to store indices for each material
    // TODO should probably resize in chunks
    // BUG this seems wrong -> should not need to resize this... at least document why
    visibleSurfaceIndices.resize(opaqueSurfaces.size()+1);

    // recurse down children nodes
    // TODO check the stack frame count to see if this is better handles linearly
    /* FcNode::addToDrawCollection(collection); */
    fcPrintEndl("added To Draw Collection");
  }




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
