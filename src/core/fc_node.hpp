// fc_node.hpp
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_draw_collection.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>
#include <memory>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  class FcMesh;
  //
  //
  // Implementation of a drawable scene node. The scene node can hold Children and will
  // also keep a transform to propagate to them May in the future derive from parent
  // Renderable class if we decided to add an extra type of node that is renderable but
  // does not have/need scene graph behaviors
  // TODO make class
  struct FcNode // : FcRenderable
  {
     // Weak pointer to parent node to avoid circular references
     std::weak_ptr<FcNode> parent;
     //
     std::vector<std::shared_ptr<FcNode>> mChildren;
     //
     // TODO determine if this is needed along with the one in FcSurface
     glm::mat4 localTransform {1.0f};
     //
     void refreshTransforms(const glm::mat4& parentMatrix);
     //
     virtual void addToDrawCollection(FcDrawCollection& collection);
     //
     virtual void update(const glm::mat4& topMatrix, FcDrawCollection& collection);
  };

  //
  //
  // TODO make class
  struct FcMeshNode : public FcNode
  {
     FcMeshNode() = default;
     inline FcMeshNode(std::shared_ptr<FcMesh> mesh) { mMesh = mesh; };
     // TODO reserve space for visible surfaces in constructor (enough for all surfaces in mMesh)
     std::vector<std::shared_ptr<const FcMesh>> mVisibleSurfaces;
     std::shared_ptr<FcMesh> mMesh;

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void sortVisibleSurfaces(const glm::mat4& viewProj);
     virtual inline void addToDrawCollection(FcDrawCollection& collection) override { collection.add(this); }
     virtual void update(const glm::mat4& topMatrix, FcDrawCollection& collection) override;
  };

}// --- namespace fc --- (END)
