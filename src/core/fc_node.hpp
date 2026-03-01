//>--- fc_node.hpp ---<//
#pragma once
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_draw_collection.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vulkan/vulkan_core.h>
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>
#include <memory>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FWD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
namespace fc { class FcMesh; }
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // Implementation of a drawable scene node. The scene node can hold Children and will
  // also keep a transform to propagate to them May in the future derive from parent
  // Renderable class if we decided to add an extra type of node that is renderable but
  // does not have/need scene graph behaviors
  // TODO make class perhaps
  struct FcNode // : FcRenderable
  {
     virtual void refreshTransforms(const glm::mat4& parentMatrix);
     //
     virtual void addToDrawCollection(FcDrawCollection& collection);
     //
     //
     // Weak pointer to parent node to avoid circular references
     std::weak_ptr<FcNode> parent;
     //
     std::vector<std::shared_ptr<FcNode>> mChildren;
     //

     // *-*-*-*-*-*-*-*-   WARNING: ALIGNED WITH FCSCENEPUSHCONSTANTS   *-*-*-*-*-*-*-*- //
     //
     glm::mat4 localTransform;
     //
     glm::mat4 worldTransform;
  };


  //
  //
  // TODO make class perhaps
  struct FcMeshNode : public FcNode
  {
     glm::mat4 invWorldTransform;
     //
     VkDeviceAddress mVertexBufferAddress;
     // -*-*-*-*-*-*-*-*-*-*-*-   END: ALIGNMENT REQUIREMENTS   -*-*-*-*-*-*-*-*-*-*-*- //

     //
     std::vector<std::shared_ptr<const FcMesh>> mVisibleSurfaces;
     //
     std::shared_ptr<FcMesh> mMesh;
     //
     FcMeshNode() = default;
     //
     FcMeshNode(std::shared_ptr<FcMesh> mesh);
     // TODO reserve space for visible surfaces in constructor (enough for all surfaces in mMesh)
     //
     void sortVisibleSurfaces(const glm::mat4& viewProj);
     //
     virtual inline void addToDrawCollection(FcDrawCollection& collection) override { collection.add(this); }
     //
     virtual void refreshTransforms(const glm::mat4& topMatrix) override;
  };

}// --- namespace fc --- (END)
