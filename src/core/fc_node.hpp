// fc_node.hpp
#pragma once
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
/* #include "fc_mesh.hpp" */
#include "fc_surface.hpp"
/* #include "fc_scene.hpp" */
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
#include <glm/mat4x4.hpp>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <vector>
#include <memory>


namespace fc
{
  class FcMesh;
  class FcSubMesh;
  class FcDrawCollection;

  // implementation of a drawable scene node. The scene node can hold Children and will
  // also keep a transform to propagate to them

  // // base class for a renderable dynamic object
  // class IRenderable
  // {
  //    virtual void draw(DrawContext& ctx) = 0;
  //    virtual void update(const glm::mat4& topMatrix) = 0;
  // };

  // May in the future derive from parent Renderable class if we decided to add an extra
  // type of node that is renderable but does not have/need scene graph behaviors
  struct FcNode // : IRenderable
  {
     // TODO find out what is meant by the following
     // parent pointer must be a weak pointer to avoid circular dependencies
     std::weak_ptr<FcNode> parent;
     std::vector<std::shared_ptr<FcNode>> mChildren;
     glm::mat4 localTransform {1.0f};

     // DELETE
     glm::mat4 worldTransform {1.0f};

     void refreshTransforms(const glm::mat4& parentMatrix);
     virtual void addToDrawCollection(FcDrawCollection& collection);
     virtual void update(const glm::mat4& topMatrix, FcDrawCollection& collection);
  };

  // TODO reserve space for visible surfaces in constructor (enough for all surfaces in mMesh)
  struct FcMeshNode : public FcNode
  {
     /* std::shared_ptr<FcMesh> mMesh; */
     std::shared_ptr<FcSurface> mMesh;
     std::vector<const FcSubMesh*> visibleSurfaces;
     void sortVisibleSurfaces(const glm::mat4& viewProj);
     // ?? should perhaps be removed or added to draw collection methods instead
     void updateDrawCollection(FcDrawCollection& collection);
     virtual void addToDrawCollection(FcDrawCollection& collection) override;
     virtual void update(const glm::mat4& topMatrix, FcDrawCollection& collection) override;
  };


}// --- namespace fc --- (END)
