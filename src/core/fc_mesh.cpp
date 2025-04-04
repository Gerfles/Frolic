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
  // TODO swap this function for a faster visibility check algorithm so we can do faster frustum culling
  bool RenderObject::isVisible(const glm::mat4& viewProj)
  {
    std::array<glm::vec3, 8> corners { glm::vec3{1.0f, 1.0f, 1.0f}
                                     , glm::vec3{1.0f, 1.0f, -1.0f}
                                     , glm::vec3{1.0f, -1.0f, 1.0f}
                                     , glm::vec3{1.0f, -1.0f, -1.0f}
                                     , glm::vec3{-1.0f, 1.0f, 1.0f}
                                     , glm::vec3{-1.0f, 1.0f, -1.0f}
                                     , glm::vec3{-1.0f, -1.0f, 1.0f}
                                     , glm::vec3{-1.0f, -1.0f, -1.0f} };

    glm::mat4 matrix = viewProj * transform;

    glm::vec3 min = {1.5f, 1.5f, 1.5f};
    glm::vec3 max = {-1.5f, -1.5f, -1.5f};

    for (int corner = 0; corner < 8; corner++)
    {
      // project each corner into clip space
      glm::vec4 vector = matrix * glm::vec4(bounds.origin + corners[corner] * bounds.extents, 1.f);

      // perspective correction
       vector.x = vector.x / vector.w;
       vector.y = vector.y / vector.w;
       vector.z = vector.z / vector.w;

      min = glm::min(glm::vec3 {vector.x, vector.y, vector.z}, min);
      max = glm::max(glm::vec3 {vector.x, vector.y, vector.z}, max);
    }

    // check the clip space box is within the view
    if (max.x < -1.0f || min.x > 1.0f || max.y < -1.f || min.y > 1.f || max.z < 0.f || min.z > 1.f)
    {
      return false;
    }
    else {
      return true;
    }
  }



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

  void Node::draw(DrawContext& context)
  {
    // draw children
    for (std::shared_ptr<Node>& child : children)
    {
      child->draw(context);
    }
  }

  void Node::update(const glm::mat4& topMatrix)
  {
    worldTransform = topMatrix * localTransform;
    for (std::shared_ptr<Node>& child : children)
    {
      child->update(topMatrix);
    }
  }

  // TODO change name
  // TODO find a way to not update if already current
  void MeshNode::draw(DrawContext& context)
  {
    for (const Surface& surface : mesh->mSurfaces)
    {
      RenderObject renderObj;
      renderObj.indexCount = surface.count;
      renderObj.firstIndex = surface.startIndex;
      renderObj.indexBuffer = mesh->IndexBuffer();
      renderObj.material = &surface.material->data;
      renderObj.bounds = surface.bounds;
      renderObj.transform = worldTransform;//localTransform;
      renderObj.invModelMatrix = glm::inverse(glm::transpose(worldTransform));
      renderObj.vertexBufferAddress = mesh->VertexBufferAddress();

      if (surface.material->data.passType == MaterialPass::Transparent)
      {
        context.transparentSurfaces.push_back(renderObj);
      }
      else {
        context.opaqueSurfaces.push_back(renderObj);
      }
    }

    // recurse down children nodes
    // TODO check the stack frame count to see if this is better handles linearly
    Node::draw(context);
  }


  void MeshNode::update(const glm::mat4& topMatrix)
  {
    //localTransform = topMatrix * worldTransform;
    //worldTransform = topMatrix * localTransform;

    Node::update(topMatrix);
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

  // Typical instantiation of upload mesh
  template void FcMesh::uploadMesh<Vertex>(std::span<Vertex> vertices
                                           , std::span<uint32_t> indices);
  // Instantiation of mesh utilized by loading height map
  template void FcMesh::uploadMesh<glm::vec3>(std::span<glm::vec3> vertices
                                              , std::span<uint32_t> indices);
  // Instantiation of mesh utilized by terrain generation
  template void FcMesh::uploadMesh<SimpleVertex>(std::span<SimpleVertex> vertices
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


  // // DELETE
  // void FcMesh::uploadMeshExtra(std::span<glm::vec4> vertices, std::span<uint32_t> indices)
  // {
  //   fcLog("\n\nNumber of indices:");
  //   std::cout << vertices.size() << std::endl;

  //    // *-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE VERTEX BUFFER   *-*-*-*-*-*-*-*-*-*-*-*-*- //
  //    // get buffer size needed to store vertices
  //   VkDeviceSize bufferSize = sizeof(glm::vec4) * vertices.size();

  //    // map memory to vertex buffer (copy vertex data into buffer)
  //    // Now create the buffer in GPU memory so we can transfer our RAM data there
  //   mVertexBuffer.storeData(vertices.data(), bufferSize
  //                           , VK_BUFFER_USAGE_TRANSFER_DST_BIT
  //                           | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
  //                           | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
  //                           | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
  //                           );

  //    // find the address fo the vertex buffer
  //   VkBufferDeviceAddressInfo deviceAddressInfo{};
  //   deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
  //   deviceAddressInfo.buffer = mVertexBuffer.getVkBuffer();

  //   mVertexBufferAddress = vkGetBufferDeviceAddress(FcLocator::Device(), &deviceAddressInfo);
  //   //mVertexBufferAddress = mVertexBuffer.getAddres();

  //  // TODO could condense this into one "create() function and just pass both vertices and indices"
  //    // could also combine with a transferToGpu() function in buffer

  //    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   CREATE INDEX BUFFER   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
  //    // get buffer size needed to store indices
  //   bufferSize = sizeof(uint32_t) * static_cast<uint32_t>(indices.size());

  //    // Now create the buffer
  //   mIndexBuffer.storeData(indices.data(), bufferSize
  //                          , VK_BUFFER_USAGE_TRANSFER_DST_BIT
  //                          | VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
  // }

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

  // Free the resources in mesh - must do here instead of destructor since Vulkan requires all resources to be free before "shutting down"
  void FcMesh::destroy()
  {
    fcLog("actually destroying mesh buffers!");
    mVertexBuffer.destroy();
    mIndexBuffer.destroy();
  }


} // namespace fc _END_
