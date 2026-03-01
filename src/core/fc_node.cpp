//>--- fc_node.cpp ---<//
#include "fc_node.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_mesh.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  //
  void FcNode::refreshTransforms(const glm::mat4& parentMatrix)
  {
    worldTransform = parentMatrix * localTransform;

    for (std::shared_ptr<FcNode>& child : mChildren)
    {
      child->refreshTransforms(worldTransform);
    }
  }


  //
  //
  void FcNode::addToDrawCollection(FcDrawCollection& collection)
  {
    // FcNodes will not be added to draw collection but recurse through children
    // to make sure any FcMeshNodes gets added
    for (std::shared_ptr<FcNode>& child : mChildren)
    {
      child->addToDrawCollection(collection);
    }
  }


  //
  //
  FcMeshNode::FcMeshNode(std::shared_ptr<FcMesh> mesh)
  {
    mMesh = mesh;
    mVertexBufferAddress = mMesh->VertexBufferAddress();
  };



  //
  //
  void FcMeshNode::refreshTransforms(const glm::mat4& topMatrix)
  {
    FcNode::refreshTransforms(topMatrix);
    invWorldTransform = glm::inverse(glm::transpose(worldTransform));
  }

}// --- namespace fc --- (END)
