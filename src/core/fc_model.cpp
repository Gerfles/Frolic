#include "fc_model.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// #include "fc_gpu.hpp"
#include "utilities.hpp"
#include "fc_descriptors.hpp"
#include "fc_image.hpp"
#include "fc_locator.hpp"
#include "fc_mesh.hpp"
#include "fc_scene_renderer.hpp"
#include "fc_defaults.hpp"
// TODO delete
#include "fc_renderer.hpp"

// -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES -*-*-*-*-*-*-*-*-*-*-*-*-*-
// GLTF loading
#include <cstdint>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
// #include <fastgltf/parser.hpp>
// #include "fastgltf/types.hpp"l
// #include "fastgltf/util.hpp"
//  GLM
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtx/quaternion.hpp>

// TODO delete assimp or isolate it to a separate loader class
#define ASSIMP_USE_HUNTER
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <vulkan/vulkan_core.h>
#include "SDL2/SDL_stdinc.h"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// #include <cstddef>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
// #include <cstdint>
// #include <fstream>
// #include <glm/glm.hpp>


namespace fc {

  // TODO make sure we preallocate vectors before loading anything...

  // void FcModel::bindDescriptorSets(VkCommandBuffer cmd
  //                                  , VkDescriptorSet pDescriptorSet, uint32_t firstSet) const
  // {
  //   vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pMaterial->pPipeline->Layout()
  //                           , firstSet, 1, &pDescriptorSet, 0, nullptr);
  // }

  // void FcModel::bindMaterialDescriptorSet(VkCommandBuffer cmd, uint32_t firstSet) const
  // {
  //   vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pMaterial->pPipeline->Layout()
  //                           , firstSet, 1, &pMaterial->materialSet, 0, nullptr);
  // }


  // void FcModel::bindIndexBuffer(VkCommandBuffer cmd) const
  // {
  //   vkCmdBindIndexBuffer(cmd, mIndexBuffer.getVkBuffer(), 0, VK_INDEX_TYPE_UINT32);
  // }

  // TODO delete
  [[deprecated("use load")]]
  void FcModel::loadGltfMeshes(std::filesystem::path filePath)
  {
    //filePath = std::filesystem::path{R"(..//models//basicmesh.glb)"};
    //  // First check for errors with files
    if (!std::filesystem::exists(filePath))
    {
      throw std::runtime_error("Directory does not exist");
    }

    std::cout << "Loading GLTF: " << filePath.string() << std::endl;

    // Now that we know the file is valid, load the data
    fastgltf::Expected<fastgltf::GltfDataBuffer> data = fastgltf::GltfDataBuffer::FromPath(filePath);

    // check that the glTF file was properly loaded
    if (data.error() != fastgltf::Error::None)
    {
      throw std::invalid_argument("Error: Could not load the glTF file.");
    }

    constexpr auto gltfOptions = fastgltf::Options::LoadExternalBuffers;

    fastgltf::Parser parser{};
    fastgltf::Asset gltf;

    auto load = parser.loadGltfBinary(data.get(), filePath.parent_path(), gltfOptions);

    if (load)
    {
      gltf = std::move(load.get());
    }
    else
    {
      std::cout << "Failed to load glTF: " << fastgltf::to_underlying(load.error())
                << ": " << fastgltf::getErrorName(load.error())
                << " - " << fastgltf::getErrorMessage(load.error());

      // TODO still need a way to return null or empty
      return;
    }

    // use the same vectors for all meshes so that the memory doesn't reallocate as often
    // TODO std::move
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
      fc::FcMesh newMesh;
      //newMesh.name = mesh.name;
      //std::cout << "meshes(loadgltfmeshes): " << &*address << std::endl;
      // clear the mesh arrays each mesh, we don't want to merge them by error
      // TODO change if can since these operations are O(N) (has to call destructor for each element)
      indices.clear();
      vertices.clear();

      // TODO this isn't right
      // uint32_t startIndex;
      // uint32_t Indexcount;

      for (auto&& primitive : mesh.primitives)
      {
        FcSurface newSurface;
        newSurface.startIndex = static_cast<uint32_t>(indices.size());
        newSurface.indexCount = static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

        size_t initialVertex = vertices.size();
        // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD INDICES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        // ?? not sure why these need to have the following scope
        {
          fastgltf::Accessor& indexAccessor = gltf.accessors[primitive.indicesAccessor.value()];
          indices.reserve(indices.size() + indexAccessor.count);

          fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAccessor, [&](std::uint32_t idx) {
            indices.push_back(idx + initialVertex); });
        }

        // *-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX POSITIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
        {
          // This will always be present in glTF so no need to check
          fastgltf::Accessor& posAccessor = gltf.accessors[primitive.findAttribute("POSITION")->accessorIndex];
          vertices.resize(vertices.size() + posAccessor.count);

          fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, posAccessor
                                                        , [&](glm::vec3 v, size_t index) {
                                                          Vertex newVertex;
                                                          newVertex.position = v;
                                                          newVertex.normal = { 1, 0, 0 };
                                                          newVertex.uv_x = 0;
                                                          newVertex.uv_y = 0;
                                                          vertices[initialVertex + index] = newVertex;
                                                        });
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX NORMALS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
        // The rest of the attributes will need to be checked for first since the glTF file may not include
        auto normals = primitive.findAttribute("NORMAL");
        if (normals != primitive.attributes.end())
        {
          fastgltf::iterateAccessorWithIndex<glm::vec3>(gltf, gltf.accessors[(*normals).accessorIndex]
                                                        , [&](glm::vec3 vec, size_t index) {
                                                          vertices[initialVertex + index].normal = vec;
                                                        });
        }

        // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX UVS   -*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
        auto uv = primitive.findAttribute("TEXCOORD_0");
        if (uv != primitive.attributes.end())
        {
          fastgltf::iterateAccessorWithIndex<glm::vec2>(gltf, gltf.accessors[(*uv).accessorIndex]
                                                        , [&](glm::vec2 vec, size_t index) {
                                                          vertices[initialVertex + index].uv_x = vec.x;
                                                          vertices[initialVertex + index].uv_y = vec.y;
                                                        });
        }

        // TODO add color back in
        // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX COLORS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
        auto colors = primitive.findAttribute("COLOR_0");
        if (colors != primitive.attributes.end())
        {
          fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).accessorIndex]
                                                        , [&](glm::vec4 vec, size_t index) {
                                                          printVec(vec, "Color");
                                                          //vertices[initialVertex + index].color = vec;
                                                        });
        }

        // newMesh.mSurfaces.push_back(newSurface);
      }



      // -*-*-*-*-*-*-*-*-*-*-*-*-   DISPLAY VERTEX NORMALS   -*-*-*-*-*-*-*-*-*-*-*-*- //
      // constexpr bool OverrideColors = false;
      // if (OverrideColors)
      // {
      //   for (Vertex& vertex : vertices)
      //   {
      //     vertex.color = glm::vec4(vertex.normal, 1.f);
      //   }
      // }

      // TODO check that we're not causing superfluous calls to copy constructor
      // TODO start here with optimizations, including a new constructor with name
      newMesh.mName = mesh.name;

      // TODO create constructor for mesh so we can emplace it in place
      newMesh.uploadMesh(std::span(vertices), std::span(indices));
      //mMeshList.emplace_back(std::move(newMesh));

      // TODO try and implement with shared pointers to meshes
      // std::vector<std::shared_ptr<FcMesh>> meshes;

      // BUG the following is required to save mesh but wont work with deleted copy constructor
      // meshes.emplace_back(std::make_shared<FcMesh>(std::move(newMesh)));


      fc::FcMesh* add2 = meshes.back().get();
      std::cout << "meshes(loadgltfmeshes): " << &*add2
                << std::endl;
    }
  }



  // TODO rename most of these function to what they actually do - ie generateTextureList();
  std::vector<std::string> FcModel::LoadMaterials(const aiScene* scene)
  {
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


  FcModel::FcModel(std::string fileName, VkDescriptorSetLayout descriptorLayout)
  {
    // first initialze model matrix to identity
    mModelMatrix = glm::mat4(1.0f);

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
        //materialNumToTexID[i] = loadTexture(textureNames[i]);
        materialNumToTexID[i] = loadTexture(textureNames[i], descriptorLayout);
        // TODO maybe save descriptorset to mesh
      }
    }

    // load in all our meshes
    loadNodes(scene->mRootNode, scene, materialNumToTexID);

  } // --- FcModel::FcModel (_) --- (END)



  // recursively load all the nodes from a tree of nodes within the scene
  void FcModel::loadNodes(aiNode* node, const aiScene* scene, std::vector<int>& matToTex)
  {
    // go through each mesh at this node and create it, then add it to our meshList ()
    for (size_t i = 0; i < node->mNumMeshes; ++i)
    {
      // note that the actual meshes are contained within the aiScene object and the mMeshes in the
      // node object just contains the indices that corresponds to the texture in the scene
      aiMesh* currMesh = scene->mMeshes[node->mMeshes[i]];
      //
      loadMesh(currMesh, scene, matToTex[currMesh->mMaterialIndex]);
    }
    // go through each node attached to this node and load it, then append their meshes to this
    // node's mesh list
    for (size_t i = 0; i < node->mNumChildren; ++i)
    {
      loadNodes(node->mChildren[i], scene, matToTex);
    }
  }


  void FcModel::loadMesh(aiMesh* mesh, const aiScene* scene, uint32_t descriptorID)
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



  uint32_t FcModel::loadTexture(std::string filename, VkDescriptorSetLayout layout)
  {
    // TODO i think the texture is being copied here, don't do that !!
    FcImage texture;
//    texture.loadTexture(filename);
    mTextures.emplace_back(std::move(texture));

    FcDescriptorBindInfo bindInfo{};
    bindInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texture
                         // TODO attach a default or passed in sampler
                         , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , nullptr);

    VkDescriptorSet descriptorSet;
    descriptorSet = FcLocator::DescriptorClerk().createDescriptorSet(layout, bindInfo);
    mDescriptorSets.emplace_back(std::move(descriptorSet));

    return mTextures.size() - 1;
  }


  void FcModel::destroy()
  {
    // for (auto& mesh : meshes)
    // {
    //   mesh->destroy();
    // }


    // for (auto& mesh : mMeshList)
    // {
    //   mesh.destroy();
    // }

    // for (auto& texture : mTextures)
    // {
    //   texture.destroy();
    // }
  }



} // namespace fc _END_
