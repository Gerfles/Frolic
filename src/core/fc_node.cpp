// fc_node.cpp
#include "fc_node.hpp"
#include "core/log.hpp"
#include "fc_mesh.hpp"
#include "fc_draw_collection.hpp"
// DELETE
/* #include "fc_model.hpp" // FOR drawContext */

namespace fc
{

  void FcNode::refreshTransform(const glm::mat4& parentMatrix)
  {
    worldTransform = parentMatrix * localTransform;

    for (std::shared_ptr<FcNode> child : children)
    {
      child->refreshTransform(worldTransform);
    }
  }

  //
  //
  void FcNode::addToDrawCollection(FcDrawCollection& collection)
  {
    // draw children
    for (std::shared_ptr<FcNode>& child : children)
    {
      child->addToDrawCollection(collection);
    }
  }


  //
  // TODO solidify the transform updates
  void FcNode::update(const glm::mat4& topMatrix, FcDrawCollection& collection)
  {
    worldTransform = topMatrix * localTransform;

    for (std::shared_ptr<FcNode>& child : children)
    {
      child->update(topMatrix, collection);
    }
  }

  // DELETE
  //  TODO swap this function for a faster visibility check algorithm so we can do faster
  //  frustum culling
  void FcMeshNode::sortVisibleSurfaces(const glm::mat4& viewProj)
  {
    //?? make sure this does not unallocate
    visibleSurfaces.clear();

    for (const FcSubMesh& surface : mMesh->Surfaces())
    {
      std::array<glm::vec3, 8> corners { glm::vec3{1.0f, 1.0f, 1.0f}
                                       , glm::vec3{1.0f, 1.0f, -1.0f}
                                       , glm::vec3{1.0f, -1.0f, 1.0f}
                                       , glm::vec3{1.0f, -1.0f, -1.0f}
                                       , glm::vec3{-1.0f, 1.0f, 1.0f}
                                       , glm::vec3{-1.0f, 1.0f, -1.0f}
                                       , glm::vec3{-1.0f, -1.0f, 1.0f}
                                       , glm::vec3{-1.0f, -1.0f, -1.0f} };

      glm::mat4 matrix = viewProj * worldTransform;

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
      }
      else {
        visibleSurfaces.push_back(&surface);
      }
    }
  }


  void FcMeshNode::updateDrawCollection(FcDrawCollection& collection)
  {
    using MaterialSurfacePair = std::pair<FcMaterial*, std::vector<FcSurface>>;

    for (const FcSubMesh& subMesh : mMesh->Surfaces())
    {
      // First figure out if this material subMesh will belong in transparent or opaque pipeline
      std::vector<MaterialSurfacePair>* selectedCollection;
      if (subMesh.material->materialType == FcMaterial::Type::Transparent)
      {
        selectedCollection = &collection.transparentSurfaces;
      } else {
        selectedCollection = &collection.opaqueSurfaces;
      }

      // Narrow by finding the associated material
      for (MaterialSurfacePair& pair : *selectedCollection)
      {
        if (subMesh.material.get() == pair.first)
        {
          // go through all the associated FcSurfaces
          for (FcSurface& surface : pair.second)
          {
            // update the subMesh
            if (surface.firstIndex == subMesh.startIndex
                && surface.indexCount == subMesh.indexCount)
            {
              surface.transform = worldTransform;
              break;
            }
          }
        }
      }
    }
    // recurse down children nodes
    /* FcNode::addToDrawCollection(collection); */
  }

  // TODO document the following in other places as well.  This structure is structured in
  // a way that allows a given material to keep track of all of the surfaces that require
  // it to draw. That way we can simply bind a material and then draw all of the surfaces
  // associated with that material before moving the the next material...
  void FcMeshNode::addToDrawCollection(FcDrawCollection& collection)
  {
    using MaterialSurfacePair = std::pair<FcMaterial*, std::vector<FcSurface>>;

    for (const FcSubMesh& surface : mMesh->Surfaces())
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
    collection.visibleSurfaceIndices.resize(collection.opaqueSurfaces.size()+1);

    // recurse down children nodes
    // TODO check the stack frame count to see if this is better handles linearly
    FcNode::addToDrawCollection(collection);
  }



  void FcMeshNode::update(const glm::mat4& topMatrix, FcDrawCollection& collection)
  {
    /* localTransform = topMatrix * worldTransform; */
    worldTransform = topMatrix * localTransform;
    updateDrawCollection(collection);
    FcNode::update(topMatrix, collection);

  }

}// --- namespace fc --- (END)
