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
  FcMeshNode::FcMeshNode(std::shared_ptr<FcSurface> mesh)
  {
    mMesh = mesh;
    mMesh->initSubMeshes(mesh);
    // // ?? not sure why i don't seem to have to init the parent mesh
    /* mesh->init(this); */
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


  // FIXME update for FcSubmesh removal so we can utilize references
  //
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
      for (FcSubmesh& subMesh : renderObject.second)
      {
        // TODO REMOVE function entirely or address this code of...
        if (*mMesh.get() == *subMesh.parent)
        {
          subMesh.parent->setTransform(&updateMatrix);
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
    collection.add(this);
  }

}// --- namespace fc --- (END)
