// fc_node.cpp
#include "fc_node.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_draw_collection.hpp"
#include "fc_mesh.hpp"
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
    mMesh->setTransform(nodeMatrix);

    FcNode::update(topMatrix, collection);
  }

}// --- namespace fc --- (END)
