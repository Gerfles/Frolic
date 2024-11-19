#include "fc_mesh.hpp"


// - FROLIC ENGINE -
//
// - EXTERNAL LIBRARIES -
#include "vulkan/vulkan_core.h"
// - STD LIBRARIES -
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>



namespace fc
{
   // TODO might be better to have as a constructor
  FcMesh::FcMesh(const FcGpu* gpu, std::vector<Vertex>& vertices
                          , std::vector<uint32_t>& indices, uint32_t textureID)
  {
    createVertexBuffer(gpu, vertices);
    createIndexBuffer(gpu, indices);
    mTextureID = textureID;
  }


   // TODO combine these or have one call the other
  void FcMesh::createMesh(const FcGpu* gpu, std::vector<Vertex>& vertices
                  , std::vector<uint32_t>& indices, uint32_t textureID)
  {
    createVertexBuffer(gpu, vertices);
    createIndexBuffer(gpu, indices);
    mTextureID = textureID;
  }
// Mesh::Mesh(const FcGpu& gpu, std::vector<Vertex> * vertices)
  //   		: mDevice{gpu.vkDevice()}, mPhysicalDevice{gpu.physicalDevice()}
  // {
  //   mVertexCount = vertices->size();
  //   createVertexBuffer(vertices);
  // }

  // TODO could create a transferToGpu() function in FcBuffer
  void FcMesh::createVertexBuffer(const FcGpu* gpu, std::vector<Vertex>& vertices)
  {
     //mUboModel.model = glm::mat4(1.0f);

     // initialize the Mesh object
    mVertexCount = vertices.size();
     //std::cout << "Number of vertices in Mesh: " << mVertexCount << std::endl;

     // get buffer size needed to store vertices
    VkDeviceSize bufferSize = sizeof(Vertex) * mVertexCount;

     // create staging buffer to store vertices in RAM
    FcBuffer stagingBuffer;
    stagingBuffer.create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                         , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

     // map memory to vertex buffer (copy vertex data into buffer)
     // NOTE: the pointer to the actual data must be used even though the program compiles if the pointer to the vector (or any other pointer) is given
    stagingBuffer.storeData(vertices.data(), bufferSize);

     // Now create the buffer in GPU memory so we can transfer our RAM data there
    mVertexBuffer.create(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                         , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    mVertexBuffer.copyBuffer(stagingBuffer, bufferSize);

    // finally, free the resources of the staging buffer since it's no longer needed
    stagingBuffer.destroy();


    // for (auto vertex : vertices)
    // {
    //   vertex.print();
    // }

  }


   // TODO could condense this into one "create() function and just pass both vertices and indices" could also combine with a transferToGpu() function in buffer
  void FcMesh::createIndexBuffer(const FcGpu* gpu, std::vector<uint32_t>& indices)
  {
    mIndexCount = static_cast<uint32_t>(indices.size());
     //std::cout << "Number of polygons in Mesh: " << mIndexCount / 3 << std::endl;

     // get buffer size needed to store indices
    VkDeviceSize bufferSize = sizeof(uint32_t) * mIndexCount;

     // create staging buffer to store indices in RAM
    FcBuffer stagingBuffer;
    stagingBuffer.create(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
                         , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

     // map memory to index buffer (copy index data into buffer)
     // NOTE: the pointer to the actual data must be used even though the program compiles if the pointer to the vector (or any other pointer) is given
    stagingBuffer.storeData(indices.data(), bufferSize);

     // Now create the buffer in GPU memory so we can transfer our RAM data there
    mIndexBuffer.create(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT
                        , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    mIndexBuffer.copyBuffer(stagingBuffer, bufferSize);

     // finally, free the resources of the staging buffer since it's no longer needed
    stagingBuffer.destroy();
  }



  void FcMesh::setModel(glm::mat4 newModel)
  {
     //mUboModel.model = newModel;
  }


   // Free the resources in mesh - must do here instead of destructor since Vulkan requires all resources to be free before "shutting down"
  void FcMesh::destroy()
  {
    mVertexBuffer.destroy();
    mIndexBuffer.destroy();
  }


} // namespace fc _END_
