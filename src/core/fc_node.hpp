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
  // TODO Can incrementally update push constants according to:
  // https://docs.vulkan.org/guide/latest/push_constants.html
  // May provide some benefits if done correctly
  // TODO this is too much data for some GPU push constant limits
  // ScenePCs are not created directly but instead are the values that the shader will recieve from
  // pushing a FcSurface object with vkCmdPushConstants and using this struct as the size of the
  // data sent. Then FcSurface must have the first members exactly match this structure. This will
  // save us from having to create a ScenePushConstant to
  // This structure should not be used directly but instead serves as a size indicator
  struct ScenePushConstants
  {
     glm::mat4 worldMatrix;
     glm::mat4 normalTransform;
     VkDeviceAddress vertexBuffer;

     ScenePushConstants() = delete;
  };


  //
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
