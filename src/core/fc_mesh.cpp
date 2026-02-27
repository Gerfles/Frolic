// fc_mesh.cpp
#include "fc_mesh.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "log.hpp"
#include "platform.hpp"
#include "fc_locator.hpp"
#include "fc_node.hpp"
#include "fc_types.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

namespace fc
{
  // TODO probably should refrain from using vector here or at least perf. test
  // TODO could create a transferToGpu() function in FcBuffer
  //Note that this pattern is not very efficient, as we are waiting for the GPU command to fully
  //execute before continuing with our CPU side logic. This is something people generally put on a
  //background thread, whose sole job is to execute uploads like this one, and deleting/reusing the
  //staging buffers.


  // Typical instantiation of upload mesh
  template void FcMesh::uploadMesh<Vertex>(std::span<Vertex> vertices, std::span<uint32_t> indices);
  // Instantiation of mesh utilized by loading height map
  template void FcMesh::uploadMesh<glm::vec3>(std::span<glm::vec3> vertices, std::span<uint32_t> indices);

  template <typename T> void FcMesh::uploadMesh(std::span<T> vertices, std::span<uint32_t> indices)
  {
    // mName = name;

    // *-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE VERTEX BUFFER   *-*-*-*-*-*-*-*-*-*-*-*-*- //
    // get buffer size needed to store vertices
    VkDeviceSize bufferSize = sizeof(T) * vertices.size();

    // map memory to vertex buffer (copy vertex data into buffer)
    // Now create the buffer in GPU memory so we can transfer our RAM data there
    mVertexBuffer.allocate(bufferSize, FcBufferTypes::Vertex);
    mVertexBuffer.write(vertices.data(), bufferSize);

    // find the address fo the vertex buffer (used for terrain & shadow renderers as well as comparisons)
    mVertexBufferAddress = mVertexBuffer.getVkDeviceAddress();

    // TODO could condense this into one "create() function and just pass both vertices and indices"
    // could also combine with a transferToGpu() function in buffer

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE INDEX BUFFER   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // get buffer size needed to store indices
    bufferSize = sizeof(uint32_t) * static_cast<uint32_t>(indices.size());

    // Now write the data to the buffer
    mIndexBuffer.allocate(bufferSize, FcBufferTypes::Index);
    mIndexBuffer.write(indices.data(), bufferSize);
  }


  //
  //
  void FcMesh::addSubMesh(FcSubmesh& subMesh)
  {
    mSubMeshes.emplace_back(subMesh);
  }


  //
  //
  //  TODO swap this function for a faster visibility check algorithm so we can do faster
  //  frustum culling
  const bool FcSubmesh::isVisible(const glm::mat4& viewProjection) const
  {
    /* return true; */
    glm::mat4 mvp = viewProjection * node->worldTransform;

    glm::vec3 min = {1.5f, 1.5f, 1.5f};
    glm::vec3 max = {-1.5f, -1.5f, -1.5f};

    for (int cornerIndex = 0; cornerIndex < 8; cornerIndex++)
    {
      // project each cornerIndex into clip space
      glm::vec4 corner = mvp * boundaryBox[cornerIndex];

      // perspective correction
      corner.x = corner.x / corner.w;
      corner.y = corner.y / corner.w;
      corner.z = corner.z / corner.w;

      min = glm::min(glm::vec3 {corner.x, corner.y, corner.z}, min);
      max = glm::max(glm::vec3 {corner.x, corner.y, corner.z}, max);
    }

    // // TEST Alternative to profile but would think the previous is faster
    // check the clip space box is within the view
    // BUG this falsly rejects some surfaces that are actually visible
    //     // ?? This method does not work well when camera is within bounding box
    if (max.x < -1.0f || min.x > 1.0f || max.y < -1.f || min.y > 1.f || max.z < 0.f || min.z > 1.f)
    {
      return false;
    }
    return true;
  }


  //
  //
  const bool FcSubmesh::isInBounds(const glm::vec4& position) const
  {
    glm::vec3 min = {FLOAT_MAX, FLOAT_MAX, FLOAT_MAX};
    glm::vec3 max = {FLOAT_MIN, FLOAT_MIN, FLOAT_MIN};

    for (size_t cornerIndex = 0; cornerIndex < 8; cornerIndex++)
    {
      // project each corner into clip space
      glm::vec4 corner = node->worldTransform * boundaryBox[cornerIndex];

      min = glm::min(glm::vec3 {corner.x, corner.y, corner.z}, min);
      max = glm::max(glm::vec3 {corner.x, corner.y, corner.z}, max);
    }

    if (position.x < max.x && position.x > min.x &&
        position.y < max.y && position.y > min.y &&
        position.z < max.z && position.z > min.z)
    {
      return false;
    }

    return true;
  }


  //
  //
  void FcMesh::destroy()
  {
    mVertexBuffer.destroy();
    mIndexBuffer.destroy();
  }


}// --- namespace fc --- (END)
