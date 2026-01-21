// fc_node.hpp
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>
#include <memory>


namespace fc
{
  // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FORWARD DECL'S   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
  class FcMesh;
  class FcSurface;
  class FcSubMesh;
  class FcDrawCollection;

  // // base class for a renderable dynamic object
  // class FcRenderable
  // {
  //    virtual void draw(DrawContext& ctx) = 0;
  //    virtual void update(const glm::mat4& topMatrix) = 0;
  // };

  // Implementation of a drawable scene node. The scene node can hold Children and will
  // also keep a transform to propagate to them May in the future derive from parent
  // Renderable class if we decided to add an extra type of node that is renderable but
  // does not have/need scene graph behaviors
  struct FcNode // : FcRenderable
  {
     // Weak pointer to parent node to avoid circular references
     std::weak_ptr<FcNode> parent;
     std::vector<std::shared_ptr<FcNode>> mChildren;
     glm::mat4 localTransform {1.0f};
     void refreshTransforms(const glm::mat4& parentMatrix);
     virtual void addToDrawCollection(FcDrawCollection& collection);
     virtual void update(const glm::mat4& topMatrix, FcDrawCollection& collection);
  };


  struct FcMeshNode : public FcNode
  {
     std::shared_ptr<FcSurface> mSurface;
     // TODO reserve space for visible surfaces in constructor (enough for all surfaces in mMesh)
     std::vector<const FcSubMesh*> visibleSurfaces;
     // // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
     void sortVisibleSurfaces(const glm::mat4& viewProj);
     // ?? should perhaps be removed or added to draw collection methods instead
     void updateDrawCollection(FcDrawCollection& collection, glm::mat4& updateMatrix);
     virtual void addToDrawCollection(FcDrawCollection& collection) override;
     virtual void update(const glm::mat4& topMatrix, FcDrawCollection& collection) override;
  };


}// --- namespace fc --- (END)
