#include "fc_mesh.hpp"

// - FROLIC ENGINE -
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


  void RenderObject::bindDescriptorSet(VkCommandBuffer cmd
                                   , VkDescriptorSet descriptorSet, uint32_t firstSet) const
  {
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, material->pPipeline->Layout()
                            , firstSet, 1, &descriptorSet, 0, nullptr);
  }


  void RenderObject::bindIndexBuffer(VkCommandBuffer cmd) const
  {
    vkCmdBindIndexBuffer(cmd, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
  }


  void Node::refreshTransform(const glm::mat4& parentMatrix)
  {
    worldTransform = parentMatrix * localTransform;

    for (std::shared_ptr<Node> child : children)
    {
      child->refreshTransform(worldTransform);
    }
  }


  void Node::draw(const glm::mat4& topMatrix, DrawContext& context)
  {
    // draw children
    // ?? do we need the & here??
    for (std::shared_ptr<Node>& child : children)
    {
      child->draw(topMatrix, context);
    }
  }


  void MeshNode::draw(const glm::mat4& topMatrix, DrawContext& context)
  {
    glm::mat4 nodeMatrix = topMatrix * worldTransform;

    for (const Surface& surface : mesh->mSurfaces)
    {
      RenderObject renderObj;
      renderObj.indexCount = surface.count;
      renderObj.firstIndex = surface.startIndex;
      renderObj.indexBuffer = mesh->IndexBuffer();
      renderObj.material = &surface.material->data;

      renderObj.transform = nodeMatrix;
      renderObj.vertexBufferAddress = mesh->VertexBufferAddress();

      context.opaqueSurfaces.push_back(renderObj);
    }

    // recurse down children nodes
    Node::draw(topMatrix, context);
  }



   // TODO might be better to have as a constructor
  FcMesh::FcMesh(std::vector<Vertex>& vertices
                 , std::vector<uint32_t>& indices, uint32_t descriptorID)
  {
    createVertexBuffer(vertices);
    createIndexBuffer(indices);
    mDescriptorID = descriptorID;
  }


   // TODO probably should refrain from using vector here or at least perf. test
   // TODO could create a transferToGpu() function in FcBuffer
//Note that this pattern is not very efficient, as we are waiting for the GPU command to fully
//execute before continuing with our CPU side logic. This is something people generally put on a
//background thread, whose sole job is to execute uploads like this one, and deleting/reusing the
//staging buffers.

  void FcMesh::uploadMesh2(std::span<Vertex2> vertices, std::span<uint32_t> indices)
  {
    // mName = name;

     // *-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE VERTEX BUFFER   *-*-*-*-*-*-*-*-*-*-*-*-*- //
     // get buffer size needed to store vertices
    VkDeviceSize bufferSize = sizeof(Vertex2) * vertices.size();

     // map memory to vertex buffer (copy vertex data into buffer)
     // Now create the buffer in GPU memory so we can transfer our RAM data there
    mVertexBuffer.storeData(vertices.data(), bufferSize
                            , VK_BUFFER_USAGE_TRANSFER_DST_BIT
                            | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
                            | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
                             // | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
                            );

     // find the address fo the vertex buffer
    VkBufferDeviceAddressInfo deviceAddressInfo{};
    deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    deviceAddressInfo.buffer = mVertexBuffer.getVkBuffer();

    mVertexBufferAddress = vkGetBufferDeviceAddress(FcLocator::Device(), &deviceAddressInfo);

   // TODO could condense this into one "create() function and just pass both vertices and indices"
     // could also combine with a transferToGpu() function in buffer

     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE INDEX BUFFER   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
     // get buffer size needed to store indices
    bufferSize = sizeof(uint32_t) * static_cast<uint32_t>(indices.size());

     // Now create the buffer
    mIndexBuffer.storeData(indices.data(), bufferSize
                           , VK_BUFFER_USAGE_TRANSFER_DST_BIT
                           | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  }

// Mesh::Mesh(const FcGpu& gpu, std::vector<Vertex> * vertices)
  //   		: mDevice{gpu.vkDevice()}, mPhysicalDevice{gpu.physicalDevice()}
  // {
  //   mVertexCount = vertices->size();
  //   createVertexBuffer(vertices);
  // }


  // void FcMesh::setIndexCounts(uint32_t start, uint32_t count)
  // {
  //   // startIndex = start;
  //   // mIndexCount = count;
  // }




   // TODO combine these or have one call the other
  // void FcMesh::createMesh(std::vector<Vertex>& vertices
  //                 , std::vector<uint32_t>& indices, uint32_t textureID)
  // {
  //   createVertexBuffer(vertices);
  //   createIndexBuffer(indices);
  //   mTextureID = textureID;
  // }

// Mesh::Mesh(const FcGpu& gpu, std::vector<Vertex> * vertices)
  //   		: mDevice{gpu.vkDevice()}, mPhysicalDevice{gpu.physicalDevice()}
  // {
  //   mVertexCount = vertices->size();
  //   createVertexBuffer(vertices);
  // }

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
    mVertexBuffer.storeData(vertices.data(), bufferSize
                            , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  }


   // TODO could condense this into one "create() function and just pass both vertices and indices" could also combine with a transferToGpu() function in buffer
  void FcMesh::createIndexBuffer(std::vector<uint32_t>& indices)
  {
    //mIndexCount = static_cast<uint32_t>(indices.size());

     // get buffer size needed to store indices
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();//mIndexCount;

     // Now create the buffer
    mIndexBuffer.storeData(indices.data(), bufferSize
                           , VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  }


   // Free the resources in mesh - must do here instead of destructor since Vulkan requires all resources to be free before "shutting down"
  void FcMesh::destroy()
  {
    mVertexBuffer.destroy();
    mIndexBuffer.destroy();
  }


} // namespace fc _END_
