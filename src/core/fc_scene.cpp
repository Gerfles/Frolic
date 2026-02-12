// fc_scene.cpp
#include "fc_scene.hpp"

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "core/fc_image.hpp"
#include "core/fc_resources.hpp"
#include "fc_renderer.hpp"
#include "core/log.hpp"
#include "fc_locator.hpp"
#include "fc_descriptors.hpp"
#include "fc_defaults.hpp"
#include "fc_draw_collection.hpp"
// TODO rename utilities to fc_utilities
#include "utilities.hpp"
/* #include "fc_mesh.hpp" */
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
// GLTF loading
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <iostream>
// Matrix manipulation
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace fc
{

  //
  // FIXME test and finallize
  void FcScene::clearAll()
  {
    VkDevice device = FcLocator::Device();

    FcDescriptorClerk mDescriptorClerk;
    mDescriptorClerk.destroy();
    mMaterialDataBuffer.destroy();

    // for (auto& [key, val] : mMeshes)
    // {
    //   val->destroy();
    // }

    // for (auto& image : mTextures)
    // {
    //   image.destroy();
    // }
    // for (auto& [key, val] : mImages)
    // {
    //   // TODO get rid of dependency on FcRenderer
    //   // Make sure we don't destroy the default image
    //   if (val.Image() != pCreator->mCheckerboardTexture.Image())
    //   {
    //     val.destroy();
    //   }
    // }

    // for (auto& sampler : mSamplers)
    // {
    //   vkDestroySampler(device, sampler, nullptr);
    // }
  }

  //
  //
  void FcScene::loadGltf(FcRenderer& renderer, std::string_view filepath)
  {
    // TODO delete renderer and pDevice dependencies
    VkDevice pDevice = FcLocator::Device();
    pRenderer = &renderer;

    size_t filenamePos = filepath.find_last_of('/');
    mName = filepath.substr(filenamePos + 1);

    std::cout << "Loading GLTF file: " << mName << std::endl;

    constexpr fastgltf::Extensions extensions =
      fastgltf::Extensions::KHR_materials_clearcoat
      | fastgltf::Extensions::KHR_materials_transmission;

    fastgltf::Parser parser{extensions};

    constexpr fastgltf::Options gltfOptions = fastgltf::Options::DontRequireValidAssetMember
                                              | fastgltf::Options::AllowDouble
                                              | fastgltf::Options::LoadExternalBuffers;
    //                                | fastgltf::Options::LoadExternalImages;

    // Now that we know the file is valid, load the data
    fastgltf::Expected<fastgltf::GltfDataBuffer> data = fastgltf::GltfDataBuffer::FromPath(filepath);

    // check that the glTF file was properly loaded
    // TODO note this section is not in the vk-Guide--> should check if
    // fastgltf::Options::DontRequireValidAssetMember negates having this
    if (data.error() != fastgltf::Error::None)
    {
      std::cout << "Failed to load glTF: " << fastgltf::getErrorName(data.error())
                << " - " << fastgltf::getErrorMessage(data.error()) << std::endl;
      // TODO still need a way to return null or empty
      //return{};
    }

    std::filesystem::path parentPath{filepath};
    parentPath = parentPath.parent_path();

    fastgltf::Asset gltf;

    // TODO test that this loads both Json and binary
    fastgltf::Expected<fastgltf::Asset> load = parser.loadGltf(data.get(), parentPath, gltfOptions);

    if (load.error() != fastgltf::Error::None)
    {
      std::cout << "Failed to load glTF (" << fastgltf::to_underlying(load.error())
                << "): " << fastgltf::getErrorName(load.error())
                << " - " << fastgltf::getErrorMessage(load.error()) << std::endl;

      // TODO still need a way to return null or empty
      return; //{}
    }
    else
    {
      std::cout << "Extensions Used in glTF file: ";
      for (size_t i = 0; i < gltf.extensionsUsed.size(); i++)
      {
        std:: cout << gltf.extensionsUsed[i];
      }
      std::cout << std::endl;

      gltf = std::move(load.get());
    }

    // we can estimate the descriptors we will need accurately
    // TODO FIXME descriptor clerk
    // std::vector<PoolSizeRatio> poolRatios = { {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
    //                                           {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
    //                                           {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3} };
    //
    // mDescriptorClerk.initDescriptorPools(gltf.materials.size(), poolRatios);

    // -*-*-*-*-*-*-*-*-*-*-   LOAD ALL TEXTURES AND MATERIALS   -*-*-*-*-*-*-*-*-*-*- //
    mNumMaterials = gltf.materials.size();
    std::cout << "Number of material in Scene: " << mNumMaterials << std::endl;
    std::vector<std::shared_ptr<FcMaterial>> materials(mNumMaterials);

    FcDrawCollection& drawCollection = renderer.DrawCollection();
    bindlessLoadAllMaterials(drawCollection, gltf, materials, parentPath);

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD ALL MESHES   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    loadMeshes(gltf, materials);

    // -*-*-*-*-*-*-*-*-*-*-   LOAD ALL NODES AND CONNECT TO MESHES   -*-*-*-*-*-*-*-*-*-*- //
    // TODO FIXME should load nodes first, then load the associated meshes into them iff there is
    // an associated mesh

    // using this to first collect data then store in unordered maps later
    std::vector<std::shared_ptr<FcNode>> nodes;

    u32 totalMeshNodes = 0;
    std::string nodeName = "newNode";

    for (fastgltf::Node& gltfNode : gltf.nodes)
    {
      std::shared_ptr<FcNode> newNode = std::make_shared<FcNode>();

      // find if the gltfNode has a mesh, if so, hook it it to the mesh pointer and allocate it with MeshNode class
      if (gltfNode.meshIndex.has_value())
      {
        totalMeshNodes++;
        /* newNode = std::make_shared<FcMeshNode>(mMeshes[*gltfNode.meshIndex]); */
        /* newNode = std::make_shared<FcMeshNode>(); */
        newNode = std::make_shared<FcMeshNode>(mMeshes[*gltfNode.meshIndex]);
        /* static_cast<FcMeshNode*>(newNode.get())->mMesh = mMeshes[*gltfNode.meshIndex]; */


        /* static_cast<FcMeshNode*>(newNode.get())->mMesh->init(static_cast<FcMeshNode*>(newNode.get())); */

        // DELETE if possible
        // for (auto& subMesh : static_cast<FcMeshNode*>(newNode.get())->mMesh->mSubMeshes)
        // {
        //   subMesh->init(static_cast<FcMeshNode*>(newNode.get()));
        // }

        /* mIndexBuffer = meshNode->mMesh->IndexBuffer(); */
        // static_cast<FcMeshNode*>(newNode.get())->mSurface->mIndexBuffer.
        // surface->mIndexBuffer.setVkBuffer(meshNode->mSurface->mIndexBuffer.getVkBuffer());
        // surface->mTransform = meshNode->localTransform;

        // // BUG this won't get properly updated when model is transformed,
        // // need to use reference or otherwise update
        // surface->mInvModelMatrix = glm::inverse(glm::transpose(meshNode->localTransform));
        // surface->mVertexBufferAddress = meshNode->mSurface->mVertexBufferAddress;
      }
      else
      {
        std::shared_ptr<FcNode> newNode = std::make_shared<FcNode>();
        // TODO TEST that we are don't need to initialize FcNode with transforms etc.
      }

      // TODO preallocate, etc.
      nodes.push_back(newNode);

      // References
      // https://fastgltf.readthedocs.io/latest/guides.html#id8
      // Extract the node transform matrix from the glTF file
      std::visit(fastgltf::visitor {
          // Call this lambda if the glTF includes a transform matrix already
          [&] (fastgltf::math::fmat4x4 matrix)
           {
             memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
           },
            // Call this lamda if we need to build the transform matrix from rotation, scale, translate
            [&](fastgltf::TRS transform)
             {
               glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
               glm::quat rot(transform.rotation[3], transform.rotation[0]
                             , transform.rotation[1], transform.rotation[2]);
               glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);
               //
               glm::mat4 T = glm::translate(glm::mat4(1.f), tl);
               glm::mat4 R = glm::toMat4(rot);
               glm::mat4 S = glm::scale(glm::mat4(1.f), sc);
               //
               // TODO TEST with only having the transform as part of the node, don't need it within surface?
               newNode->localTransform = T * R * S;
             } },
        gltfNode.transform);
    }

    std::cout << "Total Mesh Nodes: " << totalMeshNodes;
    std::cout << "\nTotal Loaded Meshes: " << mMeshes.size() << std::endl;

    // Run another loop over the nodes to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++)
    {
      fastgltf::Node& gltfNode = gltf.nodes[i];

      std::shared_ptr<FcNode>& sceneNode = nodes[i];

      for (auto& child : gltfNode.children)
      {
        sceneNode->mChildren.push_back(nodes[child]);
        nodes[child]->parent = sceneNode;
      }

      // Find the top nodes, with no parents
      if (sceneNode->parent.lock() == nullptr)
      {
        mTopNodes.push_back(sceneNode);
        // TEST dependency
        sceneNode->refreshTransforms(glm::mat4{1.0f});
      }
    }

    // Finally add the loaded scene into the draw collection structure
    fcPrintEndl("Adding to draw collection");
    addToDrawCollection(drawCollection);
    fcPrintEndl("Added to draw collection");

    std::cout << "Loaded GLTF file: " << filepath << "\n" << std::endl;
    drawSceneGraph();
  }


  void FcScene::loadMeshes(fastgltf::Asset& gltf, std::vector<std::shared_ptr<FcMaterial>>& materials)
  {
    // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD ALL MESHES   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // use the same vectors for all meshes so that memory doesnt reallocate as often
    // TODO std::move
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    MaterialConstants* sceneMaterialConstants =
      static_cast<MaterialConstants*>(mMaterialDataBuffer.getAddress());

    // TODO check that gltf.meshes.size() is equal to meshnodes and not less
    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
      std::shared_ptr<FcSurface> parentMesh = std::make_shared<FcSurface>();
      mMeshes.push_back(parentMesh);

      // KEEP for ref...
      //gltf.materials[primitive.materialIndex.value()].pbrData.;

      // clear the mesh arrays each mesh, we don't want to merge them by error
      // TODO change if can since these operations are O(N) (has to call destructor for each element)
      indices.clear();
      vertices.clear();

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD SUB-MESHES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      for (auto& primitive : mesh.primitives)
      {
        // TODO replace with constructor
        /* std::shared_ptr<FcSurface> newMesh = std::make_shared<FcSurface>(); */

        // TODO use smart point to heap allocated instead...??
        FcSubmesh newSubmesh;
        newSubmesh.parent = parentMesh;
        newSubmesh.startIndex = static_cast<uint32_t>(indices.size());
        newSubmesh.indexCount =
          static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);


        /* parentMesh->mSubMeshes.push_back(newMesh); */

        /* parentMesh->addSubMesh(newSubmesh); */
        /* mMeshes.push_back(newMesh); */
        /* meshes.push_back(newMesh); */
        /* FcSubMesh newSubMesh; */


	// newSurface.mFirstIndex = static_cast<uint32_t>(indices.size());
        // newSubMesh.indexCount =
        //   static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

        size_t initialVertex = vertices.size();

        fastgltf::Accessor& indexAcccessor = gltf.accessors[primitive.indicesAccessor.value()];
        indices.reserve(indices.size() + indexAcccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAcccessor
                                                 , [&](std::uint32_t index)
                                                  {
                                                    indices.push_back(initialVertex + index);
                                                  });

        // *-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX POSITIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //

        // This will always be present in glTF so no need to check
        fastgltf::Accessor& positionAccessor =
          gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
        vertices.resize(vertices.size() + positionAccessor.count);
        fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, positionAccessor,
                                                      [&](glm::vec3 v, size_t index)
                                                       {
                                                         Vertex newVtx;
                                                         newVtx.position = v;
                                                         vertices[initialVertex + index] = newVtx;
                                                       });

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX NORMALS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
        // The rest of the attributes will need to be checked for first since the glTF file may not include
        auto normals = primitive.findAttribute("NORMAL");
        if (normals != primitive.attributes.end())
        {
          //
          fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).accessorIndex],
                                                        [&](glm::vec3 vec, size_t index)
                                                         {
                                                           vertices[initialVertex + index].normal = vec;
                                                         });
        }

        // *-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX TANGENTS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
        fastgltf::Attribute* tangents = primitive.findAttribute("TANGENT");
        if (tangents != primitive.attributes.end())
        {
          // Could indicate an error in the asset artist "pipeline" if the attributes have texture
          // coordinates but no material index... But best to check regardless.
          if (primitive.materialIndex.has_value())
          {
            // Let our shaders know we have vertex tangets available
            sceneMaterialConstants[primitive.materialIndex.value()].flags
              |= MaterialFeatures::HasVertexTangentAttribute;
          }

          fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*tangents).accessorIndex],
                                                        [&](glm::vec4 vec, size_t index)
                                                         {
                                                           vertices[initialVertex + index].tangent =
                                                             vec;
                                                         });
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX UVS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        auto uv = primitive.findAttribute("TEXCOORD_0");
        if (uv != primitive.attributes.end())
        {
          // Could indicate an error in the asset artist "pipeline" if the attributes have texture
          // coordinates but no material index... But best to check regardless.
          if (primitive.materialIndex.has_value())
          {
            sceneMaterialConstants[primitive.materialIndex.value()].flags
              |= MaterialFeatures::HasVertexTextureCoordinates;
          }

          fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex]
                                                        , [&](glm::vec2 vec, size_t index)
                                                         {
                                                           vertices[initialVertex + index].uv_x = vec.x;
                                                           vertices[initialVertex + index].uv_y = vec.y;
                                                         });
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX COLORS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
        // Note: most assets do not color their vertices since the color is generally provided by textures
        // We leave this attribute off but could be easily implemented
        auto colors = primitive.findAttribute("COLOR_0");
        if (colors != primitive.attributes.end())
        {
          // fcPrintEndl("Model has un-utilized colors per vertex");
          // NOTE: Left for reference
          // fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf,
          //                                               gltf.accessors[(*colors).accessorIndex]
          //                                               , [&](glm::vec4 vec, size_t index)
          //                                                {
          //                                                  vertices[initialVertex + index].color = vec;
          //                                                });
        }


        if (primitive.materialIndex.has_value())
        {
          /* newMesh->Material() = materials[primitive.materialIndex.value()]; */
          newSubmesh.material = materials[primitive.materialIndex.value()];
        }
        else
        {
          /* newSubMesh.material = materials[0]; */
          // TODO make sure there is always a default material
          newSubmesh.material = materials[0];
        }
        // Signal flags as to which attributes this material can expect from the vertices
        // ?? not sure if we could have a material that associated with two different
        // meshes, where one has an attribute that the other does not...
        //gltf.materials[primitive.materialIndex.value()].


        // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- MESH BOUNDING BOX   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        // loop the vertices of this surface, find min/max bounds
        glm::vec3 minPos = vertices[initialVertex].position;
        glm::vec3 maxPos = vertices[initialVertex].position;

        for (int i = initialVertex + 1; i < vertices.size(); i++)
        {
          minPos = glm::min(minPos, vertices[i].position);
          maxPos = glm::max(maxPos, vertices[i].position);
        }

        // calculate origin and extents from the min/max use extent length for radius
        FcBounds meshBounds;
        meshBounds.origin = (maxPos + minPos) * 0.5f;
        meshBounds.extents = (maxPos - minPos) * 0.5f;
        meshBounds.sphereRadius = glm::length(newSubmesh.bounds.extents);
        newSubmesh.bounds = meshBounds;

        parentMesh->mSubMeshes2.push_back(newSubmesh);
        /* newMesh->mMeshes.push_back(newSubMesh); */
        /* newMesh->uploadMesh(std::span(vertices), std::span(indices)); */
      }

      // TODO check that we're not causing superfluous calls to copy constructor / destructors
      // TODO start here with optimizations, including a new constructor with name
      // TODO consider going back to vector refererence from span since we can mandate that...
      // however, this limits future implementations that prefer arrays, etc.

      parentMesh->uploadMesh(std::span(vertices), std::span(indices));

      // TODO create constructor for mesh so we can emplace it in place
    }
  }


  void FcScene::DELETEloadMeshes(fastgltf::Asset& gltf, std::vector<std::shared_ptr<FcMaterial>>& materials)
  {
    // // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD ALL MESHES   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // // use the same vectors for all meshes so that memory doesnt reallocate as often
    // // TODO std::move
    // std::vector<uint32_t> indices;
    // std::vector<Vertex> vertices;

    // MaterialConstants* sceneMaterialConstants =
    //   static_cast<MaterialConstants*>(mMaterialDataBuffer.getAddress());

    // // TODO check that gltf.meshes.size() is equal to meshnodes and not less
    // for (fastgltf::Mesh& mesh : gltf.meshes)
    // {
    //   std::shared_ptr<FcSurface> parentMesh = std::make_shared<FcSurface>();
    //   mMeshes.push_back(parentMesh);

    //   // KEEP for ref...
    //   //gltf.materials[primitive.materialIndex.value()].pbrData.;

    //   // clear the mesh arrays each mesh, we don't want to merge them by error
    //   // TODO change if can since these operations are O(N) (has to call destructor for each element)
    //   indices.clear();
    //   vertices.clear();

    //   // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD SUB-MESHES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    //   for (auto& primitive : mesh.primitives)
    //   {
    //     // TODO replace with constructor
    //     std::shared_ptr<FcSurface> newMesh = std::make_shared<FcSurface>();
    //     /* parentMesh->mSubMeshes.push_back(newMesh); */
    //     parentMesh->addSubMesh(newMesh);
    //     /* mMeshes.push_back(newMesh); */
    //     /* meshes.push_back(newMesh); */
    //     /* FcSubMesh newSubMesh; */

    //     u32 firstIndex = static_cast<uint32_t>(indices.size());
    //     u32 indexCount = static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);
    //     newMesh->setIndices(firstIndex, indexCount);
    //     // newSurface.mFirstIndex = static_cast<uint32_t>(indices.size());
    //     // newSubMesh.indexCount =
    //     //   static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

    //     size_t initialVertex = vertices.size();

    //     fastgltf::Accessor& indexAcccessor = gltf.accessors[primitive.indicesAccessor.value()];
    //     indices.reserve(indices.size() + indexAcccessor.count);

    //     fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAcccessor
    //                                              , [&](std::uint32_t index)
    //                                               {
    //                                                 indices.push_back(initialVertex + index);
    //                                               });

    //     // *-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX POSITIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //

    //     // This will always be present in glTF so no need to check
    //     fastgltf::Accessor& positionAccessor =
    //       gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
    //     vertices.resize(vertices.size() + positionAccessor.count);
    //     fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, positionAccessor,
    //                                                   [&](glm::vec3 v, size_t index)
    //                                                    {
    //                                                      Vertex newVtx;
    //                                                      newVtx.position = v;
    //                                                      vertices[initialVertex + index] = newVtx;
    //                                                    });

    //     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX NORMALS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    //     // The rest of the attributes will need to be checked for first since the glTF file may not include
    //     auto normals = primitive.findAttribute("NORMAL");
    //     if (normals != primitive.attributes.end())
    //     {
    //       //
    //       fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).accessorIndex],
    //                                                     [&](glm::vec3 vec, size_t index)
    //                                                      {
    //                                                        vertices[initialVertex + index].normal = vec;
    //                                                      });
    //     }

    //     // *-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX TANGENTS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
    //     fastgltf::Attribute* tangents = primitive.findAttribute("TANGENT");
    //     if (tangents != primitive.attributes.end())
    //     {
    //       // Could indicate an error in the asset artist "pipeline" if the attributes have texture
    //       // coordinates but no material index... But best to check regardless.
    //       if (primitive.materialIndex.has_value())
    //       {
    //         // Let our shaders know we have vertex tangets available
    //         sceneMaterialConstants[primitive.materialIndex.value()].flags
    //           |= MaterialFeatures::HasVertexTangentAttribute;
    //       }

    //       fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*tangents).accessorIndex],
    //                                                     [&](glm::vec4 vec, size_t index)
    //                                                      {
    //                                                        vertices[initialVertex + index].tangent =
    //                                                          vec;
    //                                                      });
    //     }

    //     // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX UVS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    //     auto uv = primitive.findAttribute("TEXCOORD_0");
    //     if (uv != primitive.attributes.end())
    //     {
    //       // Could indicate an error in the asset artist "pipeline" if the attributes have texture
    //       // coordinates but no material index... But best to check regardless.
    //       if (primitive.materialIndex.has_value())
    //       {
    //         sceneMaterialConstants[primitive.materialIndex.value()].flags
    //           |= MaterialFeatures::HasVertexTextureCoordinates;
    //       }

    //       fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex]
    //                                                     , [&](glm::vec2 vec, size_t index)
    //                                                      {
    //                                                        vertices[initialVertex + index].uv_x = vec.x;
    //                                                        vertices[initialVertex + index].uv_y = vec.y;
    //                                                      });
    //     }

    //     // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX COLORS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    //     // Note: most assets do not color their vertices since the color is generally provided by textures
    //     // We leave this attribute off but could be easily implemented
    //     auto colors = primitive.findAttribute("COLOR_0");
    //     if (colors != primitive.attributes.end())
    //     {
    //       // fcPrintEndl("Model has un-utilized colors per vertex");
    //       // NOTE: Left for reference
    //       // fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf,
    //       //                                               gltf.accessors[(*colors).accessorIndex]
    //       //                                               , [&](glm::vec4 vec, size_t index)
    //       //                                                {
    //       //                                                  vertices[initialVertex + index].color = vec;
    //       //                                                });
    //     }


    //     if (primitive.materialIndex.has_value())
    //     {
    //       /* newMesh->Material() = materials[primitive.materialIndex.value()]; */
    //       newMesh->setMaterial(materials[primitive.materialIndex.value()]);
    //     }
    //     else
    //     {
    //       /* newSubMesh.material = materials[0]; */
    //       // TODO make sure there is always a default material
    //       newMesh->setMaterial(materials[0]);
    //     }
    //     // Signal flags as to which attributes this material can expect from the vertices
    //     // ?? not sure if we could have a material that associated with two different
    //     // meshes, where one has an attribute that the other does not...
    //     //gltf.materials[primitive.materialIndex.value()].


    //     // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- MESH BOUNDING BOX   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    //     // loop the vertices of this surface, find min/max bounds
    //     glm::vec3 minPos = vertices[initialVertex].position;
    //     glm::vec3 maxPos = vertices[initialVertex].position;

    //     for (int i = initialVertex + 1; i < vertices.size(); i++)
    //     {
    //       minPos = glm::min(minPos, vertices[i].position);
    //       maxPos = glm::max(maxPos, vertices[i].position);
    //     }

    //     // calculate origin and extents from the min/max use extent length for radius
    //     FcBounds meshBounds;
    //     meshBounds.origin = (maxPos + minPos) * 0.5f;
    //     meshBounds.extents = (maxPos - minPos) * 0.5f;
    //     meshBounds.sphereRadius = glm::length(newMesh->Bounds().extents);
    //     newMesh->setBounds(meshBounds);

    //     /* newMesh->mMeshes.push_back(newSubMesh); */
    //     /* newMesh->uploadMesh(std::span(vertices), std::span(indices)); */
    //   }

    //   // TODO check that we're not causing superfluous calls to copy constructor / destructors
    //   // TODO start here with optimizations, including a new constructor with name
    //   // TODO consider going back to vector refererence from span since we can mandate that...
    //   // however, this limits future implementations that prefer arrays, etc.

    //   parentMesh->uploadMesh(std::span(vertices), std::span(indices));

    //   // TODO create constructor for mesh so we can emplace it in place
    // }
  }

//
  //
  // TODO further extrapolate functions to reduce redundancies
  // Load all the textures and materials for a scene using descriptor indexing (bindless resources)
  void FcScene::bindlessLoadAllMaterials(FcDrawCollection& drawCollection,
                                         fastgltf::Asset& gltf,
                                         std::vector<std::shared_ptr<FcMaterial>>& materials,
                                         std::filesystem::path& parentPath)
  {
    fcPrintEndl("Bindlessly Loading all  materials...");
    u32 textureOffset = drawCollection.mTextures.getFirstFreeIndex();

    for (fastgltf::Texture& gltfTexture : gltf.textures)
    {
      // ?? Doesn't seem very efficient to do it this way with the pool of memory set
      // to all zeros when we have to initialize some members of the class, which we
      // do within the getNextFree() function
      FcImage* newTexture = drawCollection.mTextures.getNextFree();

      if (newTexture == nullptr)
      {
        // TODO handle grow etc.
        /* return textureIndex; */
        throw std::runtime_error("invalid texture index");
      }

      // Load the Texture image
      newTexture->loadFromGltf(parentPath, gltf, gltf.images[gltfTexture.imageIndex.value()]);

      // Check that the glTF has a filter and if not, set a default
      if (gltfTexture.samplerIndex.has_value())
      {
        newTexture->setSampler(gltf.samplers[gltfTexture.samplerIndex.value()]);
      }
      else
      {
        fastgltf::Sampler dummySampler;
        dummySampler.minFilter = fastgltf::Filter::Nearest;
        dummySampler.magFilter = fastgltf::Filter::Nearest;
        newTexture->setSampler(dummySampler);
      }

      // If image failed to load, assign default image so we can continue loading scene
      if (!newTexture->isValid())
      {
        // TEST by deleting some textures from a gltf
        *newTexture = FcDefaults::Textures.checkerboard;
        fcPrint("Failed to load texture: %s\n", gltfTexture.name.data());
      }

      // TODO defer adding textures until we know we are not in the middle of a frame
      bool isBindlessSupported = true;
      if (isBindlessSupported)
      {
        ResourceUpdate resourceUpdate{ResourceDeletionType::Texture
                                    , newTexture->Handle()
                                    , drawCollection.stats.frame};
        drawCollection.bindlessTextureUpdates.push_back(resourceUpdate);
      }
    } //(END) -- for (fastgltf::Texture& gltfTexture : gltf.textures)

    fcPrint("Number of Textures Loaded: %i\n",
            drawCollection.mTextures.getFirstFreeIndex() - textureOffset);

    // Create buffer to hold the material data
    mMaterialDataBuffer.allocate(sizeof(MaterialConstants) * gltf.materials.size()
                                 , FcBufferTypes::Uniform);

    // MaterialConstants* sceneMaterialConstants =
    //   static_cast<MaterialConstants*>(mMaterialDataBuffer.getAddress());

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD ALL MATERIALS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // Preallocate material vector
    // TODO ?? eliminate materials in favor of two separate but equally sized vectors (vkDescriptorSet / type)

    for (size_t i = 0; i < materials.size(); ++i)
    {
      materials[i] = std::make_shared<FcMaterial>();

      // TODO could sort here by material (pipeline) and also perhaps alphaMode/alphaCutoff
      fastgltf::Material& material = gltf.materials[i];

      // Save the type of material so we can determine which pipeline to use later
      if (material.alphaMode == fastgltf::AlphaMode::Blend)
      {
        //std::cout << "Adding transparent material" << std::endl;
        materials[i]->materialType = FcMaterial::Type::Transparent;
      } else {
        materials[i]->materialType = FcMaterial::Type::Opaque;
      }

      FcDescriptorBindInfo bindInfo{};

      // TODO TEST to see if this part can be outside of the loop
      bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                          , VK_SHADER_STAGE_FRAGMENT_BIT);

      // create the descriptor set layout for the material
      mMaterialDescriptorLayout =
        FcLocator::DescriptorClerk().createDescriptorSetLayout(bindInfo);

      MaterialConstants constants{};
      constants.colorFactors.x = material.pbrData.baseColorFactor[0];
      constants.colorFactors.y = material.pbrData.baseColorFactor[1];
      constants.colorFactors.z = material.pbrData.baseColorFactor[2];
      constants.colorFactors.w = material.pbrData.baseColorFactor[3];
      // Metal Rough
      constants.metalRoughFactors.x = material.pbrData.metallicFactor;
      constants.metalRoughFactors.y = material.pbrData.roughnessFactor;
      // Index of Refraction
      constants.iorF0 = pow((1 - material.ior) / (1 + material.ior), 2);

      // emmisive factors
      constants.emmisiveFactors = glm::vec4(material.emissiveFactor.x()
                                            , material.emissiveFactor.y()
                                            , material.emissiveFactor.z()
                                            , material.emissiveStrength);

      // TODO look into specular colors... These checks fail w/ seg fault on material.specular
      // if (material.specular->specularTexture.has_value())
      // {
      //   fcPrintEndl("Has unused specular texture!");
      // }

      // if (material.specular->specularColorTexture.has_value())
      // {
      //   fcPrintEndl("Has unused specular color texture!");
      // }

      // *-*-*-*-*-*-*-*-*-*-*-*-*-   MATERIAL DATA BUFFER   *-*-*-*-*-*-*-*-*-*-*-*-*- //
      uint32_t dataBufferOffset = i * sizeof(MaterialConstants);

      bindInfo.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mMaterialDataBuffer
                            , sizeof(MaterialConstants), dataBufferOffset);

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   BINDLESS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      size_t index;

      if (material.pbrData.baseColorTexture.has_value())
      {
        index = material.pbrData.baseColorTexture.value().textureIndex + textureOffset;
        constants.colorIndex = drawCollection.mTextures.get(index)->Handle();
      }

      // *-*-*-*-*-*-*-*-*-*-*-*-   METALIC-ROUGHNESS TEXTURE   *-*-*-*-*-*-*-*-*-*-*-*- //
      if (material.pbrData.metallicRoughnessTexture.has_value())
      {
        index = material.pbrData.metallicRoughnessTexture.value().textureIndex + textureOffset;
        constants.metalRoughIndex = drawCollection.mTextures.get(index)->Handle();
      }

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-   NORMAL MAP TEXTURE   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
      if (material.normalTexture.has_value())
      {
        index = material.normalTexture.value().textureIndex + textureOffset;
        constants.normalIndex = drawCollection.mTextures.get(index)->Handle();
      }

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   OCCLUSION TEXTURE   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      if (material.occlusionTexture.has_value())
      {
        // TODO show what materials a texture has within imGUI
        index = material.occlusionTexture.value().textureIndex + textureOffset;
        constants.occlusionIndex = drawCollection.mTextures.get(index)->Handle();
        constants.occlusionFactor = material.occlusionTexture.value().strength;
      }

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   EMMISSION TEXTURE   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      if (material.emissiveTexture.has_value())
      {
        index = material.emissiveTexture.value().textureIndex + textureOffset;
        constants.emissiveIndex = drawCollection.mTextures.get(index)->Handle();
      }

      // *-*-*-*-*-*-*-*-*-*-   UNIMPLEMENTED MATERIAL PROPERTIES   *-*-*-*-*-*-*-*-*-*- //
      if (material.alphaMode == fastgltf::AlphaMode::Mask)
      {
        // TODO implement alpha flag for material and within pipeline (see below)
        if (material.alphaCutoff != 0.0)
        {
          // TODO implement alpha flag for material and within pipeline
          // TODO check and make sure that this property is always within alphaMode
        }
      }
      if (material.sheen != nullptr)
      {
        fcPrintEndl("Unimplemented Sheen Data");
        if (material.sheen->sheenColorTexture.has_value())
        {
          fcPrintEndl("Unimplemented Sheen Color Texture");
        }
      }

      if (material.transmission != nullptr)
      {
        if (material.transmission->transmissionTexture.has_value())
        {
          fcPrintEndl("Unimplemented Transmission Data");
        }
        float transmission = material.transmission->transmissionFactor;
        std::cout << "Unimplemented transmission factor: " << transmission << std::endl;

      }
      if (material.clearcoat != nullptr)
      {
        fcPrintEndl("Unimplemented Clearcoat Data");
      }
      if (material.anisotropy != nullptr)
      {
        fcPrintEndl("Unimplemented Anisotropy Data");
      }
      if (material.iridescence != nullptr)
      {
        fcPrintEndl("Unimplemented Iridescence Data");
      }

      // Write material parameters to buffer.
      // TODO rename to newMaterialConstants
      MaterialConstants* sceneMaterialConstants =
        static_cast<MaterialConstants*>(mMaterialDataBuffer.getAddress());
      sceneMaterialConstants[i] = constants;

      // TODO make the ubo descriptor set the same for all materials:
      // perhaps store an addressable buffer within the the GPU mem:

      // Build descriptor sets for each material
      materials[i]->materialSet
        = FcLocator::DescriptorClerk().createDescriptorSet(mMaterialDescriptorLayout, bindInfo);
    }

    fcPrintEndl("All materials bindlessly loaded...");
  }



  // FIXME repair binded material/texture approach (note that only the bindless approach has been
  // updated to the latest methodology so probably best to just copy that function and then add in
  // the binded techniques from this function)
  // Load all the textures and materials for a scene using descriptor indexing (bindless resources)
  void FcScene::loadAllMaterials(FcDrawCollection& drawCollection,
                             fastgltf::Asset& gltf,
                             std::vector<std::shared_ptr<FcMaterial>>& materials,
                             std::filesystem::path& parentPath)
  {
    uint32_t size = 0;

    u32 textureOffset = drawCollection.mTextures.getFirstFreeIndex();

    for (fastgltf::Image& gltfImage : gltf.images)
    {
      // ?? Doesn't seem very efficient to do it this way with the pool of memory set
      // to all zeros when we have to initialize some members of the class
      FcImage* newTexture = drawCollection.mTextures.getNextFree();

      if (newTexture == nullptr)
      {
        // TODO handle grow etc.
        /* return textureIndex; */
        throw std::runtime_error("invalid texture index");
      }

      newTexture->loadFromGltf(parentPath, gltf, gltfImage);

      if (!newTexture->isValid())
      { // failed to load image so assign default image so we can continue loading scene
        // TODO check that this is working properly by deleting some textures from a gltf
        newTexture = &FcDefaults::Textures.checkerboard;
        /* mTextures.push_back(FcDefaults::Textures.checkerboard); */
        std::cout << "Failed to load texture: " << gltfImage.name << std::endl;
      }

      // // Add textures to draw collection to defer upload to GPU until we are no
      // // longer drawing a frame.
      // bool isBindlessSupported = true;
      // if (isBindlessSupported)
      // {
      //   ResourceUpdate resourceUpdate{ResourceDeletionType::Texture
      //                               , newTexture->Handle()
      //                               , drawCollection.stats.frame};
      //   drawCollection.bindlessTextureUpdates.push_back(resourceUpdate);
      // }

      size++;
    }


    // Load samplers using helper functions that translate from OpenGL to vulkan specs
    std::vector<VkSampler> samplers;

    for (fastgltf::Sampler& sampler : gltf.samplers)
    {
      if (sampler.minFilter.has_value() && sampler.magFilter.has_value())
      {
        VkSamplerMipmapMode mipMode;
        mipMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Linear));

        if (sampler.minFilter.value() == fastgltf::Filter::Nearest
            && sampler.magFilter.value() == fastgltf::Filter::Nearest)
        {
          fcPrintEndl("USING NEAREST VKSAMPLER");
          // Get a pointer to default nearest sampler
          samplers.push_back(FcDefaults::Samplers.Nearest);
        }
        else
        {
          if (mipMode == VK_SAMPLER_MIPMAP_MODE_NEAREST)
          {
            fcPrintEndl("USING BILINEAR VKSAMPLER");
            // Get a pointer to default linear sampler
            samplers.push_back(FcDefaults::Samplers.Bilinear);
          }
          else if (mipMode == VK_SAMPLER_MIPMAP_MODE_LINEAR)
          {
            fcPrintEndl("USING TRILINEAR VKSAMPLER");
            // Get a pointer to default linear sampler
            samplers.push_back(FcDefaults::Samplers.Trilinear);
          }
          else
          {
            fcPrintEndl("USING LINEAR VKSAMPLER");
            // Get a pointer to default linear sampler
            samplers.push_back(FcDefaults::Samplers.Linear);
          }
        }
      }
      else
      {
        fcPrintEndl("USING DEFAULT VKSAMPLER");
        // Get a pointer to default linear sampler
        samplers.push_back(FcDefaults::Samplers.Trilinear);
      }
    }


    std::cout << "Number of Textures Loaded: " << size << std::endl;
    /* throw std::runtime_error("STOPPING"); */

    // Create buffer to hold the material data
    mMaterialDataBuffer.allocate(sizeof(MaterialConstants) * gltf.materials.size()
                                 , FcBufferTypes::Uniform);

    // MaterialConstants* sceneMaterialConstants =
    //   static_cast<MaterialConstants*>(mMaterialDataBuffer.getAddress());

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD ALL MATERIALS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
// Preallocate material vector
    // TODO ?? eliminate materials in favor of two separate but equally sized vectors (vkDescriptorSet / type)

    for (size_t i = 0; i < materials.size(); ++i)
    {
      materials[i] = std::make_shared<FcMaterial>();

      // TODO could sort here by material (pipeline) and also perhaps alphaMode/alphaCutoff
      fastgltf::Material& material = gltf.materials[i];

      // Save the type of material so we can determine which pipeline to use later
      if (material.alphaMode == fastgltf::AlphaMode::Blend)
      {
        //std::cout << "Adding transparent material" << std::endl;
        materials[i]->materialType = FcMaterial::Type::Transparent;
      } else {
        materials[i]->materialType = FcMaterial::Type::Opaque;
      }

      FcDescriptorBindInfo bindInfo{};

      // TODO TEST to see if this part can be outside of the loop
      bindInfo.addBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
                          , VK_SHADER_STAGE_FRAGMENT_BIT);
      // Color texture
      bindInfo.addBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                          , VK_SHADER_STAGE_FRAGMENT_BIT);
      // Metal-Rough texture
      bindInfo.addBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                          , VK_SHADER_STAGE_FRAGMENT_BIT);
      // Normal texture
      bindInfo.addBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                          , VK_SHADER_STAGE_FRAGMENT_BIT);
      // Occlusion texture
      bindInfo.addBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                          , VK_SHADER_STAGE_FRAGMENT_BIT);
      // Emissive texture
      bindInfo.addBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                          , VK_SHADER_STAGE_FRAGMENT_BIT);

      // create the descriptor set layout for the material
      mMaterialDescriptorLayout =
        FcLocator::DescriptorClerk().createDescriptorSetLayout(bindInfo);


      MaterialConstants constants;
      constants.colorFactors.x = material.pbrData.baseColorFactor[0];
      constants.colorFactors.y = material.pbrData.baseColorFactor[1];
      constants.colorFactors.z = material.pbrData.baseColorFactor[2];
      constants.colorFactors.w = material.pbrData.baseColorFactor[3];
      // Metal Rough
      constants.metalRoughFactors.x = material.pbrData.metallicFactor;
      constants.metalRoughFactors.y = material.pbrData.roughnessFactor;
      // Index of Refraction
      constants.iorF0 = pow((1 - material.ior) / (1 + material.ior), 2);

      // emmisive factors
      constants.emmisiveFactors = glm::vec4(material.emissiveFactor.x()
                                            , material.emissiveFactor.y()
                                            , material.emissiveFactor.z()
                                            , material.emissiveStrength);


      // TODO look into specular colors... These checks fail w/ seg fault on material.specular
      // if (material.specular->specularTexture.has_value())
      // {
      //   fcLog("Has unused specular texture!");
      // }

      // if (material.specular->specularColorTexture.has_value())
      // {
      //   fcLog("Has unused specular color texture!");
      // }

      // *-*-*-*-*-*-*-*-*-*-*-*-*-   MATERIAL DATA BUFFER   *-*-*-*-*-*-*-*-*-*-*-*-*- //
      uint32_t dataBufferOffset = i * sizeof(MaterialConstants);

      bindInfo.attachBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, mMaterialDataBuffer
                            , sizeof(MaterialConstants), dataBufferOffset);

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   BINDED APPROACH   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   COLOR TEXTURE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      if (material.pbrData.baseColorTexture.has_value())
      {
        // Indicate that our material has a color texture available
        constants.flags |= MaterialFeatures::HasColorTexture;
        //
        size_t index = material.pbrData.baseColorTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();
        //
        bindInfo.attachImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , *drawCollection.mTextures.get(textureOffset + imageIndex)
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , samplers[samplerIndex]);
      }
      else {  // set to defualt texture/sampler/values if none exist in material
        bindInfo.attachImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , FcDefaults::Textures.checkerboard
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , FcDefaults::Samplers.Linear);
      }

      // *-*-*-*-*-*-*-*-*-*-*-*-   METALIC-ROUGHNESS TEXTURE   *-*-*-*-*-*-*-*-*-*-*-*- //
      if (material.pbrData.metallicRoughnessTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasRoughMetalTexture;

        size_t index = material.pbrData.metallicRoughnessTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();

        bindInfo.attachImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , *drawCollection.mTextures.get(textureOffset + imageIndex)
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , samplers[samplerIndex]);
      }
      else {  // set to defualt texture/sampler/values if none exist in material
        bindInfo.attachImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , FcDefaults::Textures.white
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , FcDefaults::Samplers.Linear);
      }

      // -*-*-*-*-*-*-*-*-*-*-*-*-*-   NORMAL MAP TEXTURE   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
      if (material.normalTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasNormalTexture;

        size_t index = material.normalTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();
        //
        bindInfo.attachImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , *drawCollection.mTextures.get(textureOffset + imageIndex)
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , samplers[samplerIndex]);
      }
      else {  // set to defualt texture/sampler/values if none exist in material
        bindInfo.attachImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , FcDefaults::Textures.white
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , FcDefaults::Samplers.Linear);
      }

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   OCCLUSION TEXTURE   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      if (material.occlusionTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasOcclusionTexture;

        // TODO show what materials a texture has within imGUI
        std::cout << "Model has Occlusion Map Texture: (Scale = "
                  << material.occlusionTexture->strength
                  << ", Default = 1)" << std::endl;
        //
        size_t index = material.occlusionTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();
        //
        bindInfo.attachImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , *drawCollection.mTextures.get(textureOffset + imageIndex)
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , samplers[samplerIndex]);
        // Occlusion Factors
        constants.occlusionFactor = material.occlusionTexture.value().strength;
      } else
      {  // set to defualt texture/sampler/values if none exist in material
        bindInfo.attachImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , FcDefaults::Textures.white
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , FcDefaults::Samplers.Linear);

        //TODO verify what to set default as
        constants.occlusionFactor = 1.0f;
      }

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   EMMISSION TEXTURE   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      if (material.emissiveTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasEmissiveTexture;

        std::cout << "Model has Emmision Map Texture..." << std::endl;
        size_t index = material.emissiveTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();

        bindInfo.attachImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , *drawCollection.mTextures.get(textureOffset + imageIndex)
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , samplers[samplerIndex]);
      } else
      {  // set to defualt texture/sampler/values if none exist in material
        bindInfo.attachImage(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , FcDefaults::Textures.black
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , FcDefaults::Samplers.Linear);
      }

      // *-*-*-*-*-*-*-*-*-*-   UNIMPLEMENTED MATERIAL PROPERTIES   *-*-*-*-*-*-*-*-*-*- //
      // std::cout << "--------------------------------------------------------------\n";
      // std::cout << "Unimplemented properties for material - " << material.name << " :\n";
      if (material.alphaMode == fastgltf::AlphaMode::Mask)
      {
        // TODO implement alpha flag for material and within pipeline (see below)
        if (material.alphaCutoff != 0.0)
        {
          // TODO implement alpha flag for material and within pipeline
          // TODO check and make sure that this property is always within alphaMode
        }
      }
      if (material.sheen != nullptr)
      {
        fcLog("Unimplemented Sheen Data");
        if (material.sheen->sheenColorTexture.has_value())
        {
          fcLog("Unimplemented Sheen Color Texture");
        }
      }

      if (material.transmission != nullptr)
      {
        if (material.transmission->transmissionTexture.has_value())
        {
          fcLog("Unimplemented Transmission Data");
        }

        float transmission = material.transmission->transmissionFactor;
        std::cout << "Unimplemented transmission factor: " << transmission << std::endl;
      }
      if (material.clearcoat != nullptr)
      {
        fcLog("Unimplemented Clearcoat Data");
      }
      if (material.anisotropy != nullptr)
      {
        fcLog("Unimplemented Anisotropy Data");
      }
      if (material.iridescence != nullptr)
      {
        fcLog("Unimplemented Iridescence Data");
      }

      // Write material parameters to buffer.
      // TODO rename to newMaterialConstants
      MaterialConstants* sceneMaterialConstants =
      static_cast<MaterialConstants*>(mMaterialDataBuffer.getAddress());
      sceneMaterialConstants[i] = constants;

      // TODO make the ubo descriptor set the same for all materials:
      // perhaps store an addressable buffer within the the GPU mem:

      // Build descriptor sets for each material
      materials[i]->materialSet
        = FcLocator::DescriptorClerk().createDescriptorSet(mMaterialDescriptorLayout, bindInfo);

      /* newMaterial->data = renderer->mSceneRenderer.writeMaterial(pDevice, passType, materialResources); */
    }
  }

  //
  //
  VkSampler FcScene::extractSampler(fastgltf::Sampler& sampler)
  {
    if (sampler.minFilter.has_value() && sampler.magFilter.has_value())
    {
      VkSamplerMipmapMode mipMode;
      mipMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Linear));

      if (sampler.minFilter.value() == fastgltf::Filter::Nearest
          && sampler.magFilter.value() == fastgltf::Filter::Nearest)
      {
        fcPrintEndl("USING NEAREST VKSAMPLER");
        return FcDefaults::Samplers.Nearest;
      }
      else
      {
        if (mipMode == VK_SAMPLER_MIPMAP_MODE_NEAREST)
        {
          fcPrintEndl("USING BILINEAR VKSAMPLER");
          return FcDefaults::Samplers.Bilinear;
        }
        else if (mipMode == VK_SAMPLER_MIPMAP_MODE_LINEAR)
        {
          fcPrintEndl("USING TRILINEAR VKSAMPLER");
          return FcDefaults::Samplers.Trilinear;
        }
        else
        {
          fcPrintEndl("USING LINEAR VKSAMPLER");
          return FcDefaults::Samplers.Linear;
        }
      }
    }
    else
    {
      fcPrintEndl("USING DEFAULT VKSAMPLER");
      return FcDefaults::Samplers.Trilinear;
    }
  }

  //
  // glTF samplers use the numbers and properties of OpenGL, so create conversion functions
  VkFilter FcScene::extractFilter(fastgltf::Filter filter) noexcept
  {
    switch (filter)
    {
      // Nearest samplers
        case fastgltf::Filter::Nearest:
        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::NearestMipMapNearest:
          return VK_FILTER_NEAREST;

          // Linear samplers
        case fastgltf::Filter::Linear:
        case fastgltf::Filter::LinearMipMapNearest:
        case fastgltf::Filter::LinearMipMapLinear:
          return VK_FILTER_LINEAR;
    }
  }

  //
  // TODO combine with above filter function if possible
  VkSamplerMipmapMode FcScene::extractMipmapMode(fastgltf::Filter filter) noexcept
  {
    switch (filter)
    {
        case fastgltf::Filter::NearestMipMapNearest:
        case fastgltf::Filter::LinearMipMapNearest:
          // use a single mipmap without any blending
          return VK_SAMPLER_MIPMAP_MODE_NEAREST;

        case fastgltf::Filter::NearestMipMapLinear:
        case fastgltf::Filter::LinearMipMapLinear:
        default:
          // blend multiple mipmaps
          return VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }
  }

  //
  void FcScene::addToDrawCollection(FcDrawCollection& collection)
  {
    // Only loop the topnodes which will recurse through their child nodes
    fcPrintEndl("number of topnodes: %i", mTopNodes.size());
    for (auto& node : mTopNodes)
    {
      node->addToDrawCollection(collection);
    }
  }

  //
  //
  std::vector<std::string> FcScene::LoadMaterials(const aiScene* scene)
  {
    fcPrintEndl("Loading materials...");
    // create 1:1 sized list of textures
    std::vector<std::string> textureList(scene->mNumMaterials);

    // go through each material and copy its texture file name (if it exists)
    for (size_t i = 0; i < scene->mNumMaterials; ++i)
    {
      aiMaterial* material = scene->mMaterials[i];

      // initialze the texture to empty string (will be replaced if texture exists)
      textureList[i] = "";

      // check for at least one diffuse texture (standard detail texture)
      if (material->GetTextureCount(aiTextureType_DIFFUSE))
      {
        // get the path of the teture file
        aiString path;
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
        {
          //TODO handle filenames better
          // cut off any directory information already present in current path (probably a bad idea)
          size_t lastBackSlash = std::string(path.data).rfind("\\");
          std::string filename = std::string(path.data).substr(lastBackSlash + 1);

          textureList[i] = filename;
        }
      }
    }

    // TODO need to refactor all of this to use move constructors and emplace_back
    return textureList;
  }


  FcScene::FcScene(std::string fileName, VkDescriptorSetLayout descriptorLayout)
  {
    // first initialze model matrix to identity
    /* mModelMatrix = glm::mat4(1.0f); */

    // import model "scene"
    Assimp::Importer importer;

    // ?? I believe the aiProcess_JoinIdenticalVertices will remove the duplicate vertices so that
    // each is unique but should verify that it's working correctly by using a unordered set (w/
    // hashmap) to store the vertices and make sure we get the same number for both
    const aiScene* scene = importer.ReadFile(fileName, aiProcess_Triangulate
                                             | aiProcess_FlipUVs
                                             | aiProcess_JoinIdenticalVertices);
    if (!scene)
    {
      throw std::runtime_error("Failed to load model! (" + fileName + ")");
    }

    // get vector of all materials with 1:1 ID placement
    // TODO remove more copying UGH
    std::vector<std::string> textureNames = LoadMaterials(scene);

    // TODO eliminate -- find a better method
    // conversion from the materials list IDs to our descriptor array IDs (since won't be 1:1 in descriptor)
    std::vector<int> materialNumToTexID(textureNames.size());

    // loop over textureNames and create textures for them
    for (size_t i = 0; i < textureNames.size(); ++i)
    {
      // if material has no texture, set '0' to indicate no texture, texture 0 will be reserved for a default texture
      if (textureNames[i].empty())
      {
        // TODO Set descriptor set to default or simply check at render time...
        materialNumToTexID[i] = 0;
      }
      else
      {	// Otherwise, create texture and set value to index of new texture
        // materialNumToTexID[i] = loadTexture(textureNames[i], descriptorLayout);
        // TODO maybe save descriptorset to mesh
      }
    }

    // load in all our meshes
    loadAssimpNodes(scene->mRootNode, scene, materialNumToTexID);

  } // --- FcScene::FcScene (_) --- (END)


  //
  // recursively load all the nodes from a tree of nodes within the scene
  void FcScene::loadAssimpNodes(aiNode* node, const aiScene* scene, std::vector<int>& matToTex)
  {
    // go through each mesh at this node and create it, then add it to our meshList ()
    for (size_t i = 0; i < node->mNumMeshes; ++i)
    {
      // note that the actual meshes are contained within the aiScene object and the mMeshes in the
      // node object just contains the indices that corresponds to the texture in the scene
      aiMesh* currMesh = scene->mMeshes[node->mMeshes[i]];
      //
      loadAssimpMesh(currMesh, scene, matToTex[currMesh->mMaterialIndex]);
    }
    // go through each node attached to this node and load it, then append their meshes to this
    // node's mesh list
    for (size_t i = 0; i < node->mNumChildren; ++i)
    {
      loadAssimpNodes(node->mChildren[i], scene, matToTex);
    }
  }

  //
  //
  void FcScene::loadAssimpMesh(aiMesh* mesh, const aiScene* scene, uint32_t descriptorID)
  {

    // std::vector<Vertex> vertices;
    // std::vector<uint32_t> indices;

    // // resize vertex list to hold all vertices for mesh
    // vertices.resize(mesh->mNumVertices);

    // // go through each vertex and copy it across to our vertices
    // for (size_t i = 0; i < vertices.size(); ++i)
    // {
    //   // set position
    //   vertices[i].position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

    //   // TODO get rid of color from Vertex struct
    //   // set color (just use random color for now)
    //   vertices[i].color = { 0.62f, 0.7f, 0.08f, 1.0f };

    //   // set the vertex normal
    //   // ?? not sure if this is right
    //   vertices[i].normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

    //   // set texture coordinats (ONLY if they exist) (note that [0] here refers to the first SET )
    //   if (mesh->mTextureCoords[0])
    //   {
    //     vertices[i].uv_x = mesh->mTextureCoords[0][i].x;
    //     vertices[i].uv_y = mesh->mTextureCoords[0][i].y;
    //   }
    //   else
    //   {
    //     vertices[i].uv_x = 0.f;
    //     vertices[i].uv_y = 0.f;
    //   }
    // }

    // // iterate over indices through faces and copy across
    // // TODO might be better to iterate over this for loop once to resize indices() first, then fill it
    // for (size_t i = 0; i < mesh->mNumFaces; ++i)
    // {
    //   // get a face
    //   aiFace face = mesh->mFaces[i];
    //   // go through face's indices and add to list
    //   for (size_t j = 0; j < face.mNumIndices; ++j)
    //   {
    //     indices.push_back(face.mIndices[j]);
    //   }
    // }
    // // create a new mesh and return it TODO - make efficient
    // //FcMesh newMesh;
    // //newMesh.createMesh(&gpu, vertices, indices, textureID);
    // //mMeshList.push_back(newMesh);
    // mMeshList.emplace_back(vertices, indices, descriptorID);
  }

  //
  //
  void FcScene::rotate(float angleDegrees, glm::vec3& axis)
  {
    // TODO optimize rotational proceedure via matrix/quaternion manipulation
    mRotationMat = glm::toMat4(glm::angleAxis(angleDegrees, axis));
    /* mTransformMat = glm::rotate(mTransformMat, angleDegrees, axis); */
    mTransformMat = mRotationMat * mTransformMat;
  }

  //
  //
  void FcScene::rotateInPlace(float angleDegrees, glm::vec3& axis)
  {
    // // First create rotation matrix
    mRotationMat = glm::toMat4(glm::angleAxis(angleDegrees, axis));

    // Next, translate the rotation matrix (back to origin) by the inverse of the original translation
    mRotationMat[3] = mRotationMat[0] * -mTranslationMat[3][0]
                      + mRotationMat[1] * -mTranslationMat[3][1]
                      + mRotationMat[2] * -mTranslationMat[3][2]
                      + mRotationMat[3];

    // Finally, rotate then translate back to the original position
    /* mTransformMat = mTranslationMat * mRotationMat * mTransformMat; */
  }

  //
  //
  void FcScene::translate(glm::vec3& offset)
  {
    // TODO manually update mTranslationMat with inline function
    /* mTranslationMat = glm::translate(ID_MATRIX, offset); */
    mTranslationMat = glm::translate(mTranslationMat, offset);
  }

  //
  //
  // Since scaling is less common in practice, we will update the transform matrix after
  // a scale in order to save an additional... TODO
  void FcScene::scale(const glm::vec3& axisFactors)
  {
    glm::mat4 scale = glm::scale(ID_MATRIX, axisFactors);
    mTransformMat = scale * mTransformMat;
  }

  //
  //
  void FcScene::update(glm::mat4& mat, FcDrawCollection& collection)
  {
    // TODO
    /* mTransformMat = mRotationMat * mTransformMat; */
  }

  //
  //
  void FcScene::update()
  {
    mTransformMat = mTranslationMat * mRotationMat * mTransformMat;

    for (std::shared_ptr<FcNode>& node : mTopNodes)
    {
      node->update(mTransformMat, pRenderer->DrawCollection());
    }

    // TODO write math function that alters a given matrix to the identity matrix;
    // Reset the transformation matrix back to zero-transforms
    /* mTransformMat = ID_MATRIX; */
  }

  //
  // TODO dedicate a texture enum to just the textures renderable without features
  // TODO should be able to create a index pool that leaves an extra space for an INVALID_TEXTURE_INDEX
  // that way when updating the texture to not use for instance, we could place the old index into the
  // slot that is at pos+1 and then at pos we place an INVALID_TEXTURE_INDEX so that if we are
  // trying to replace the current I_T_INDEX and by simply referencing the slot one above current slot
  void FcScene::toggleTextureUse(MaterialFeatures texture,
                                 std::array<std::vector<u32>, 5>& currentIndices)
  {
    // Get the address for the buffer of material constant data being refd by the shader
    MaterialConstants* currentMaterial =
      static_cast<MaterialConstants*>(mMaterialDataBuffer.getAddress());

    // If this is the first time using this structure (size[i]() == 0) then we must initialize
    // the 5 vectors to hold all of the numMaterial indices
    if (currentIndices[0].size() == 0)
    {
      for (size_t i = 0; i < currentIndices.size(); i++)
      {
        currentIndices[i].resize(mNumMaterials, INVALID_TEXTURE_INDEX);
      }
    }

    for (size_t i = 0; i < mNumMaterials; i++)
    {
      u32* currentTextureIndex;
      // 0 = color, 1 = normal, 2 = occlusion, 3 = emissive, 4 = roughness/metalic
      size_t textureType;

      // First set the parameters for the indices we want to change
      switch (texture)
      {
          case MaterialFeatures::HasColorTexture:
            textureType = 0; // COLOR TEXTURE
            currentTextureIndex = &currentMaterial->colorIndex;
            break;

          case MaterialFeatures::HasNormalTexture:
            textureType = 1; // NORMAL MAP
            currentTextureIndex = &currentMaterial->normalIndex;
            break;

          case MaterialFeatures::HasOcclusionTexture:
            textureType = 2; // OCCLUSION MAP
            currentTextureIndex = &currentMaterial->occlusionIndex;
            break;

          case MaterialFeatures::HasEmissiveTexture:
            textureType = 3; // EMISSIVE TEXTURE
            currentTextureIndex = &currentMaterial->emissiveIndex;
            break;

          case MaterialFeatures::HasRoughMetalTexture:
            textureType = 4; // ROUGHNESS METALIC MAP
            currentTextureIndex = & currentMaterial->metalRoughIndex;
            break;

          default:
            break;
      }

      // Make the actual swap of indices, by checking to see if the texture is already turned
      // off (INVALID_TEXTURE_INDEX) and if so, assume the passed indices has the previous
      // texture and swap. Otherwise, store current index for later and update to INVALID
      if (*currentTextureIndex == INVALID_TEXTURE_INDEX)
      {
        // Toggle texture use ON
        *currentTextureIndex = currentIndices[textureType][i];
        currentIndices[textureType][i] = INVALID_TEXTURE_INDEX;
      } else {
        // Toggle texture use OFF
        currentIndices[textureType][i] = *currentTextureIndex;
        *currentTextureIndex = INVALID_TEXTURE_INDEX;
      }

      currentMaterial++;
    }
  }

  //
  //
  void FcScene::drawSceneGraph()
  {
    fcPrintEndl("\nScene: %s  -- (%i top nodes)", mName.c_str(), mTopNodes.size());

    for (size_t i = 0; i < mTopNodes.size(); ++i)
    {
      std::string nodeID = "";
      nodeID = nodeID + std::to_string(i+1);
      fcPrint("Top Node #%i (%i children)\n", i+1, mTopNodes[i]->mChildren.size());
      printNode(mTopNodes[i], nodeID);
    }
    std::cout << std::endl;
  }

  //
  // Visualizing scene nodes to be used for debugging purposes only
  void FcScene::printNode(std::shared_ptr<FcNode>& node, std::string& nodeID)
  {
    int childNum = 1;
    for (std::shared_ptr<FcNode>& node : node->mChildren)
    {
      // Print the properly formated/nested ID number
      std::string subNodeID = nodeID;
      subNodeID = subNodeID + "." + std::to_string(childNum);
      size_t count = std::ranges::count(subNodeID, '.');
      for (size_t i = 0; i < count; ++i)
      {
        std::cout << "   ";
      }
      fcPrint("|-->Child Node #%s (%i children)\n", subNodeID.c_str(), node->mChildren.size());

      // print any child nodes of this current node
      printNode(node, subNodeID);

      childNum++;
    }
  }




  //
  //
  void FcScene::destroy()
  {
    clearAll();
    // TODO additional housekeeping
  }

}// --- namespace fc --- (END)
