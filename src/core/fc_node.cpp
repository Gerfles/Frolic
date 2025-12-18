// fc_node.cpp
#include "fc_node.hpp"
#include "core/log.hpp"
#include "core/utilities.hpp"
#include "fc_mesh.hpp"
#include "fc_draw_collection.hpp"
// DELETE
/* #include "fc_model.hpp" // FOR drawContext */

namespace fc
{

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
    // draw children
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
    /* fcPrintEndl("In FcNode::update()"); */
    /* printMat(topMatrix, "topMatrix"); */
    /* worldTransform = localTransform * topMatrix; */
    /* localTransform = glm::mat4(0.4f,0.f,0.f,0.f, 0.f,0.f,-0.4f,0.f, 0.f,0.4f,0.f,0.f, 45.f,9.f,20.f,1.f); */
    /* localTransform = glm::mat4{1.0f}; */
    /* worldTransform = topMatrix * localTransform; */

    // printMat(worldTransform, "worldTransform");
    // printMat(localTransform, "localTransform");
    /* worldTransform = glm::mat4(1.f,0.f,0.f,45.f, 0.f,1.f,0.f,9.f, 0.f,0.f,1.f,20.f, 0.f,0.f,0.f,1.f); */
    /* worldTransform = glm::mat4(1.f,0.f,0.f,0.f, 0.f,1.f,0.f,0.f, 0.f,0.f,1.f,0.f, 45.f,9.f,20.f,1.f); */
    /* worldTransform = glm::mat4(0.4f,0.f,0.f,0.f, 0.f,0.f,-0.4f,0.f, 0.f,0.4f,0.f,0.f, 45.f,9.f,20.f,1.f); */

    /* printMat(worldTransform, "update(node) worldTransform"); */
    // fcPrintEndl("world transform");
    /* printMat(localTransform, "local Transform before:"); */
    /* localTransform = topMatrix * localTransform; */
    /* printMat(localTransform, "local Transform after:"); */
    /* localTransform = topMatrix * localTransform * worldTransform * topMatrix; */
    /* glm::mat4 transform = topMatrix * worldTransform; */
    /* localTransform = topMatrix * localTransform; */
    /* glm::mat4 nodeMatrix = topMatrix * localTransform; */

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

    // TODO think about making update() a member function of FcDrawCollection


    FcNode::update(topMatrix, collection);
    updateDrawCollection(collection, nodeMatrix);
    /* FcNode::addToDrawCollection(collection); */
  }



  // DELETE
  //  TODO swap this function for a faster visibility check algorithm so we can do faster
  //  frustum culling
  void FcMeshNode::sortVisibleSurfaces(const glm::mat4& viewProj)
  {
    //?? make sure this does not unallocate
    visibleSurfaces.clear();

    for (const FcSubMesh& surface : mMesh->mMeshes)
    {
      std::array<glm::vec3, 8> corners { glm::vec3{1.0f, 1.0f, 1.0f}
                                       , glm::vec3{1.0f, 1.0f, -1.0f}
                                       , glm::vec3{1.0f, -1.0f, 1.0f}
                                       , glm::vec3{1.0f, -1.0f, -1.0f}
                                       , glm::vec3{-1.0f, 1.0f, 1.0f}
                                       , glm::vec3{-1.0f, 1.0f, -1.0f}
                                       , glm::vec3{-1.0f, -1.0f, 1.0f}
                                       , glm::vec3{-1.0f, -1.0f, -1.0f} };

      glm::mat4 matrix = viewProj * localTransform;

      glm::vec3 min = {1.5f, 1.5f, 1.5f};
      glm::vec3 max = {-1.5f, -1.5f, -1.5f};

      for (int corner = 0; corner < 8; corner++)
      {
        // project each corner into clip space
        glm::vec4 vector = matrix * glm::vec4(surface.bounds.origin
                                              + corners[corner] * surface.bounds.extents, 1.f);

        // perspective correction
        vector.x = vector.x / vector.w;
        vector.y = vector.y / vector.w;
        vector.z = vector.z / vector.w;

        min = glm::min(glm::vec3 {vector.x, vector.y, vector.z}, min);
        max = glm::max(glm::vec3 {vector.x, vector.y, vector.z}, max);
      }

      // check the clip space box is within the view
      if (max.x < -1.0f || min.x > 1.0f || max.y < -1.f || min.y > 1.f || max.z < 0.f || min.z > 1.f)
      {
        // ??
      }
      else {
        visibleSurfaces.push_back(&surface);
      }
    }
  }

  // TODO might be able to avoid updating draw collection if we only add references when we call addToDrawCollection()
  void FcMeshNode::updateDrawCollection(FcDrawCollection& collection, glm::mat4& updateMatrix)
  {
    using RenderObject = std::pair<FcMaterial*, std::vector<FcSurface>>;

    // TODO this may be a slow section due to cycling through all renderObjects. Would be better probably
    // just to re-add the updated mesh node to the draw collection!
    int i = 0;
    int j = 0;

    fcPrintEndl("Updating: Mesh Node");
    for (auto& renderObject : collection.opaqueSurfaces)
    {
      for (FcSurface& surface : renderObject.second)
      {
        if (*mMesh.get() == surface)
        {
          surface.setTransform(updateMatrix);
          // TODO remove normal transform setting from setTransform and do conditionally
          // based on whether or not we have non-uniform scaling enabled.
          fcPrintEndl("found surface at renderObject#%i, surface#%i", i, j);
          /* break; */
        }
        i++;
      }
      j++;
    }


    // // iterate through all the submeshes within the surface of FcMeshNode
    // for (const FcSubMesh& subMesh : mMesh->mMeshes)
    // {
    //   // First figure out if this material subMesh will belong in transparent or opaque pipeline
    //   std::vector<RenderObject>* selectedCollection;
    //   if (subMesh.material->materialType == FcMaterial::Type::Transparent)
    //   {
    //     selectedCollection = &collection.transparentSurfaces;
    //   } else {
    //     selectedCollection = &collection.opaqueSurfaces;
    //   }

    //   // Narrow by finding the associated material
    //   for (RenderObject& pair : *selectedCollection)
    //   {
    //     if (subMesh.material.get() == pair.first)
    //     {
    //       // go through all the associated FcSurfaces
    //       fcPrintEndl("surfaceSize: %i", pair.second.size() );
    //       for (FcSurface& surface : pair.second)
    //       {
    //         // update the subMesh
    //         // if (surface.FirstIndex() == subMesh.startIndex
    //         //     && surface.IndexCount() == subMesh.indexCount)
    //         // {
    //         // TODO check to see that we're not copying un-needed matrices when we can just
    //         // confine them to one location and use refrences

    //         // if (surface.FirstIndex() == mMesh->FirstIndex())
    //         // {
    //         printMat(localTransform, "localTransform");
    //         surface.setTransform(localTransform);
    //         /* } */

    //         //   break;
    //         // }
    //       }
    //     }
    //   }
    // }
    // // recurse down children nodes ?? This may be necessary but uncertain
    /* FcNode::addToDrawCollection(collection); */

  }

  // TODO document the following in other places as well.  This structure is structured in
  // a way that allows a given material to keep track of all of the surfaces that require
  // it to draw. That way we can simply bind a material and then draw all of the surfaces
  // associated with that material before moving the the next material...
  void FcMeshNode::addToDrawCollection(FcDrawCollection& collection)
  {
    using MaterialSurfacePair = std::pair<FcMaterial*, std::vector<FcSurface>>;

    for (const FcSubMesh& surface : mMesh->mMeshes)
    {
      // First figure out if this material surface will belong in transparent or opaque pipeline
      std::vector<MaterialSurfacePair>* selectedCollection;
      if (surface.material->materialType == FcMaterial::Type::Transparent)
      {
        selectedCollection = &collection.transparentSurfaces;
      } else {
        selectedCollection = &collection.opaqueSurfaces;
      }

      // Find the material if it's already in the collection
      bool materialFound = false;
      for (MaterialSurfacePair& pair : *selectedCollection)
      {
        if (surface.material.get() == pair.first)
        {
          // create the flattened render surface and store in the draw collection
          pair.second.emplace_back(FcSurface(surface, this));
          materialFound = true;
          break;
        }
      }

      // Add a new material/surface pair if we didn't find it in the draw collection
      if (!materialFound)
      {
        MaterialSurfacePair newPair;
        newPair.first = surface.material.get();
        newPair.second.emplace_back(FcSurface(surface, this));
        selectedCollection->push_back(newPair);
      }

      collection.numSurfaces++;
    }

    // allocate enough lists in visible surfaces vector to store indices for each material
    // TODO should probably resize in chunks
    collection.visibleSurfaceIndices.resize(collection.opaqueSurfaces.size()+1);

    // recurse down children nodes
    // TODO check the stack frame count to see if this is better handles linearly
    FcNode::addToDrawCollection(collection);
  }

}// --- namespace fc --- (END)
