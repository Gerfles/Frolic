// fc_node.cpp
#include "fc_node.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_draw_collection.hpp"
//

namespace fc
{
  //
  //
  void FcNode::refreshTransforms(const glm::mat4& parentMatrix)
  {
    localTransform = parentMatrix * localTransform;
    for (std::shared_ptr<FcNode>& child : mChildren)
    {
      child->refreshTransforms(localTransform);
    }
  }

  //
  //
  void FcNode::addToDrawCollection(FcDrawCollection& collection)
  {
    // FcNodes will not be added to draw collection but recurse through children
    // to make sure any FcMeshNodes get added
    for (std::shared_ptr<FcNode>& child : mChildren)
    {
      child->addToDrawCollection(collection);
    }
  }

  //
  //
  // TODO solidify the transform updates and document and VERIFY
  void FcNode::update(const glm::mat4& topMatrix, FcDrawCollection& collection)
  {
    for (std::shared_ptr<FcNode>& child : mChildren)
    {
      child->update(topMatrix, collection);
    }
  }

  //
  //
  void FcMeshNode::update(const glm::mat4& topMatrix, FcDrawCollection& collection)
  {
    glm::mat4 nodeMatrix = topMatrix * localTransform;

    // TODO think about making updateDrawCollection() a member function of FcDrawCollection
    FcNode::update(topMatrix, collection);
    updateDrawCollection(collection, nodeMatrix);
  }


  [[deprecated("Not used but could be swaped for FcSurface.isVisible")]]
  //  TODO swap this function for a faster visibility check algorithm so we can do faster
  //  frustum culling
  void FcMeshNode::sortVisibleSurfaces(const glm::mat4& viewProj)
  {
    // //?? make sure this does not deallocate
    // mVisibleSurfaces.clear();

    // for (const std::shared_ptr<FcSurface>& surface : mSurface->mMeshes2)
    // {
    //   std::array<glm::vec3, 8> corners { glm::vec3{1.0f, 1.0f, 1.0f}
    //                                    , glm::vec3{1.0f, 1.0f, -1.0f}
    //                                    , glm::vec3{1.0f, -1.0f, 1.0f}
    //                                    , glm::vec3{1.0f, -1.0f, -1.0f}
    //                                    , glm::vec3{-1.0f, 1.0f, 1.0f}
    //                                    , glm::vec3{-1.0f, 1.0f, -1.0f}
    //                                    , glm::vec3{-1.0f, -1.0f, 1.0f}
    //                                    , glm::vec3{-1.0f, -1.0f, -1.0f} };

    //   glm::mat4 matrix = viewProj * localTransform;

    //   glm::vec3 min = {1.5f, 1.5f, 1.5f};
    //   glm::vec3 max = {-1.5f, -1.5f, -1.5f};

    //   for (int corner = 0; corner < 8; corner++)
    //   {
    //     // project each corner into clip space
    //     glm::vec4 vector = matrix * glm::vec4(surface->mBounds.origin
    //                                           + corners[corner] * surface->mBounds.extents, 1.f);

    //     // perspective correction
    //     vector.x = vector.x / vector.w;
    //     vector.y = vector.y / vector.w;
    //     vector.z = vector.z / vector.w;

    //     min = glm::min(glm::vec3 {vector.x, vector.y, vector.z}, min);
    //     max = glm::max(glm::vec3 {vector.x, vector.y, vector.z}, max);
    //   }

    //   // check the clip space box is within the view
    //   if (max.x < -1.0f || min.x > 1.0f || max.y < -1.f || min.y > 1.f || max.z < 0.f || min.z > 1.f)
    //   {
    //     // ?? This method does not work well when camera is within bounding box
    //     // TODO check that
    //   }
    //   else {
    //     mVisibleSurfaces.push_back(surface);
    //   }
    // }
  }

  // TODO might be able to avoid updating draw collection if we only add references when we call addToDrawCollection()
  void FcMeshNode::updateDrawCollection(FcDrawCollection& collection, glm::mat4& updateMatrix)
  {
    using RenderObject = std::pair<FcMaterial*, std::vector<FcSurface>>;

    // TODO this may be a slow section due to cycling through all renderObjects. Would be better probably
    // just to re-add the updated mesh node to the draw collection!
    int i = 0;
    int j = 0;

    for (auto& renderObject : collection.opaqueSurfaces)
    {
      for (FcSurface& surface : renderObject.second)
      {
        if (*mMesh.get() == surface)
        {
          surface.setTransform(updateMatrix);
          // TODO remove normal transform setting from setTransform and do conditionally
          // based on whether or not we have non-uniform scaling enabled.
          /* break; */
        }
        i++;
      }
      j++;
    }

  }

  // TODO document the following in other places as well.  This structure is structured in
  // a way that allows a given material to keep track of all of the surfaces that require
  // it to draw. That way we can simply bind a material and then draw all of the surfaces
  // associated with that material before moving the the next material...
  void FcMeshNode::addToDrawCollection(FcDrawCollection& collection)
  {
    collection.DELETEadd(this);
    /* add(this); */
    // for (std::shared_ptr<FcNode>& child : mChildren)
    // {
    //   child->addToDrawCollection(collection);
    // }



    // using RenderObject = std::pair<FcMaterial*, std::vector<FcSurface>>;

    // for (const FcSubMesh& surface : mSurface->mMeshes)
    // {
    //   // First figure out if this material surface will belong in transparent or opaque pipeline
    //   std::vector<RenderObject>* selectedCollection;
    //   if (surface.material->materialType == FcMaterial::Type::Transparent)
    //   {
    //     selectedCollection = &collection.transparentSurfaces;
    //   } else {
    //     selectedCollection = &collection.opaqueSurfaces;
    //   }

    //   // Find the material if it's already in the collection
    //   bool materialFound = false;
    //   for (RenderObject& renderObj : *selectedCollection)
    //   {
    //     if (surface.material.get() == renderObj.first)
    //     {
    //       // create the flattened render surface and store in the draw collection
    //       renderObj.second.emplace_back(FcSurface(surface, this));
    //       materialFound = true;
    //       break;
    //     }
    //   }

    //   // Add a new material/surface pair if we didn't find it in the draw collection
    //   if (!materialFound)
    //   {
    //     RenderObject newRenderObj;
    //     newRenderObj.first = surface.material.get();
    //     newRenderObj.second.emplace_back(FcSurface(surface, this));
    //     selectedCollection->push_back(newRenderObj);
    //   }

    //   collection.numSurfaces++;
    // }

    // // allocate enough lists in visible surfaces vector to store indices for each material
    // // TODO should probably resize in chunks
    // collection.visibleSurfaceIndices.resize(collection.opaqueSurfaces.size()+1);

    // recurse down children nodes
    // TODO check the stack frame count to see if this is better handles linearly

  }

}// --- namespace fc --- (END)
