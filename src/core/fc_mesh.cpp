#include "fc_mesh.hpp"

// - FROLIC ENGINE -
#include "core/utilities.hpp"
#include "fc_model.hpp"

// - EXTERNAL LIBRARIES -
#include "core/fc_locator.hpp"
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <span>
#include <stdexcept>

//#include "utilities.hpp"

namespace fc
{

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-   FCMESH FUNCTIONS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
 // TODO might be better to have as a constructor
  FcMesh::FcMesh(std::vector<Vertex>& vertices
                 , std::vector<uint32_t>& indices, uint32_t descriptorID)
  {
    createVertexBuffer(vertices);
    createIndexBuffer(indices);
  }


   // TODO probably should refrain from using vector here or at least perf. test
   // TODO could create a transferToGpu() function in FcBuffer
//Note that this pattern is not very efficient, as we are waiting for the GPU command to fully
//execute before continuing with our CPU side logic. This is something people generally put on a
//background thread, whose sole job is to execute uploads like this one, and deleting/reusing the
//staging buffers.

  // Typical instantiation of upload mesh
  template void FcMesh::uploadMesh<Vertex>(std::span<Vertex> vertices
                                           , std::span<uint32_t> indices);
  // Instantiation of mesh utilized by loading height map
  template void FcMesh::uploadMesh<glm::vec3>(std::span<glm::vec3> vertices
                                              , std::span<uint32_t> indices);

  template <typename T> void FcMesh::uploadMesh(std::span<T> vertices, std::span<uint32_t> indices)
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


  // TODO de-duplicate
  // TODO probably should refrain from using vector here or at least perf. test
  // TODO could create a transferToGpu() function in FcBuffer
  void FcMesh::createVertexBuffer(std::vector<Vertex>& vertices)
  {
     // initialize the Mesh object
    //mVertexCount = vertices.size();

     // get buffer size needed to store vertices
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();//mVertexCount;
     // map memory to vertex buffer (copy vertex data into buffer)
     // Now create the buffer in GPU memory so we can transfer our RAM data there
    mVertexBuffer.allocate(bufferSize, FcBufferTypes::Vertex);
    mVertexBuffer.write(vertices.data(), bufferSize);
    // mVertexBuffer.storeData(vertices.data(), bufferSize
    //                         , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  }

  // TODO de-duplicate
   // TODO could condense this into one "create() function and just pass both vertices and indices" could also combine with a transferToGpu() function in buffer
  void FcMesh::createIndexBuffer(std::vector<uint32_t>& indices)
  {
    //mIndexCount = static_cast<uint32_t>(indices.size());

     // get buffer size needed to store indices
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();//mIndexCount;

    mIndexBuffer.allocate(bufferSize, FcBufferTypes::Index);
    mIndexBuffer.write(indices.data(), bufferSize);

    //  // Now create the buffer
    // mIndexBuffer.storeData(indices.data(), bufferSize
    //                        , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  }


  void FcMesh::bindIndexBuffer(VkCommandBuffer cmd) const
  {
    vkCmdBindIndexBuffer(cmd, mIndexBuffer.getVkBuffer(), 0, VK_INDEX_TYPE_UINT32);
  }



  // Free the resources in mesh - must do here instead of destructor since Vulkan requires all resources to be free before "shutting down"
  void FcMesh::destroy()
  {
    fcLog("actually destroying mesh buffers!");
    mVertexBuffer.destroy();
    mIndexBuffer.destroy();
  }

} // namespace fc _END_
