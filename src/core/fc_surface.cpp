// fc_surface.cpp
#include "fc_surface.hpp"
#include "core/platform.hpp"
#include "fc_locator.hpp"
#include "fc_node.hpp"
//

namespace fc
{

  // TODO probably should refrain from using vector here or at least perf. test
  // TODO could create a transferToGpu() function in FcBuffer
  //Note that this pattern is not very efficient, as we are waiting for the GPU command to fully
  //execute before continuing with our CPU side logic. This is something people generally put on a
  //background thread, whose sole job is to execute uploads like this one, and deleting/reusing the
  //staging buffers.

  // Typical instantiation of upload mesh
  template void FcSurface::uploadMesh<Vertex>(std::span<Vertex> vertices
                                              , std::span<uint32_t> indices);
  // Instantiation of mesh utilized by loading height map
  template void FcSurface::uploadMesh<glm::vec3>(std::span<glm::vec3> vertices
                                                 , std::span<uint32_t> indices);

  template <typename T> void FcSurface::uploadMesh(std::span<T> vertices, std::span<uint32_t> indices)
  {
    // mName = name;

    // *-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE VERTEX BUFFER   *-*-*-*-*-*-*-*-*-*-*-*-*- //
    // get buffer size needed to store vertices
    mIndexCount = indices.size();
    VkDeviceSize bufferSize = sizeof(T) * vertices.size();

    // map memory to vertex buffer (copy vertex data into buffer)
    // Now create the buffer in GPU memory so we can transfer our RAM data there
    mVertexBuffer.allocate(bufferSize, FcBufferTypes::Vertex);
    mVertexBuffer.write(vertices.data(), bufferSize);
    // mVertexBuffer.storeData(vertices.data(), bufferSize
    //                         , VK_BUFFER_USAGE_TRANSFER_DST_BIT
    //                         | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
    //                         | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
    //                         | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
    //                         );

    // find the address fo the vertex buffer
    VkBufferDeviceAddressInfo deviceAddressInfo{};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = mVertexBuffer.getVkBuffer();

    mVertexBufferAddress = vkGetBufferDeviceAddress(FcLocator::Device(), &deviceAddressInfo);
    //mVertexBufferAddress = mVertexBuffer.getAddres();

    // TODO could condense this into one "create() function and just pass both vertices and indices"
    // could also combine with a transferToGpu() function in buffer

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE INDEX BUFFER   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // get buffer size needed to store indices
    bufferSize = sizeof(uint32_t) * static_cast<uint32_t>(indices.size());

    // Now write the data to the buffer
    mIndexBuffer.allocate(bufferSize, FcBufferTypes::Index);
    mIndexBuffer.write(indices.data(), bufferSize);
    // mIndexBuffer.storeData(indices.data(), bufferSize
    //                        , VK_BUFFER_USAGE_TRANSFER_DST_BIT
    //                        | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  }



// TODO should probably hash the FcSurface for quicker lookup so we can update the surface within the draw collection by using a simple hash lookup
  FcSurface::FcSurface(const FcSubMesh& surface, FcMeshNode* meshNode)
  {
    mFirstIndex = surface.startIndex;
    mIndexCount = surface.indexCount;

    // FIXME
    // TODO this seems like a bad idea to Copy this data when it really should only be used in one class
    // This should be done when the gltf file is initially loaded.
    mBounds = surface.bounds;
    mBoundaryBox.init(mBounds);

    /* mIndexBuffer = meshNode->mMesh->IndexBuffer(); */
    mIndexBuffer.setVkBuffer(meshNode->mMesh->mIndexBuffer.getVkBuffer());
    mTransform = meshNode->worldTransform;

    // BUG this won't get properly updated when model is transformed,
    // need to use reference or otherwise update
    mInvModelMatrix = glm::inverse(glm::transpose(meshNode->worldTransform));
    mVertexBufferAddress = meshNode->mMesh->mVertexBufferAddress;
  }


  bool FcSurface::isVisible(const glm::mat4& viewProjection)
  {
    /* return true; */
    glm::mat4 matrix = viewProjection * mTransform;

    glm::vec3 min = {1.5f, 1.5f, 1.5f};
    glm::vec3 max = {-1.5f, -1.5f, -1.5f};

    for (int cornerIndex = 0; cornerIndex < 8; cornerIndex++)
    {
      // project each cornerIndex into clip space
      glm::vec4 corner = matrix * mBoundaryBox[cornerIndex];

      // perspective correction
      corner.x = corner.x / corner.w;
      corner.y = corner.y / corner.w;
      corner.z = corner.z / corner.w;

      min = glm::min(glm::vec3 {corner.x, corner.y, corner.z}, min);
      max = glm::max(glm::vec3 {corner.x, corner.y, corner.z}, max);
    }

    // check the clip space box is within the view
    // BUG this falsly rejects some surfaces that are actually visible
    if (max.x > -1.0f && min.x < 1.0f && max.y > -1.f && min.y < 1.f && max.z > 0.f && min.z < 1.f)
    {
      return true;
      // TODO remove the following BUG this algorithm is inefficient and inprecise
      // if ((max.x > -1.0f && min.x < 1.0f) && (max.y > -1.f && min.y < 1.f))
      // {
      //   return false;
      // }
    }

    // // TEST Alternative to profile but would think the previous is faster
    // if (max.x < -1.0f || min.x > 1.0f || max.y < -1.f || min.y > 1.f || max.z < 0.f || min.z > 1.f)
    // {
    //   return false;
    // }

    return false;
  }

  //
  //
  const bool FcSurface::isInBounds(const glm::vec4& position) const
  {
    glm::vec3 min = {FLOAT_MAX, FLOAT_MAX, FLOAT_MAX};
    glm::vec3 max = {FLOAT_MIN, FLOAT_MIN, FLOAT_MIN};

    for (size_t cornerIndex = 0; cornerIndex < 8; cornerIndex++)
    {
      // project each corner into clip space
      glm::vec4 corner = mTransform * mBoundaryBox[cornerIndex];

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


  void FcSurface::destroy()
  {
    mVertexBuffer.destroy();
    mIndexBuffer.destroy();
  }

}// --- namespace fc --- (END)
