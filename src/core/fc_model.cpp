#include "fc_model.hpp"

 // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //

//#include "fastgltf/types.hpp"l
//#include "fastgltf/util.hpp"
#include "core/utilities.hpp"
#include "fc_descriptors.hpp"
#include "fc_gpu.hpp"
#include "fc_image.hpp"
#include "fc_locator.hpp"
#include "fc_mesh.hpp"
 // -*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL LIBRARIES -*-*-*-*-*-*-*-*-*-*-*-*-*-
#include <cstdint>
// glTF loading
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
// glm
#include <filesystem>
#include <fstream>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
//#include "fastgltf/core.hpp"

// #include <fastgltf/parser.hpp>
// TODO delete assimp or isolate it to a separate loader class
#define ASSIMP_USE_HUNTER
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <vulkan/vulkan_core.h>
#include "SDL2/SDL_stdinc.h"
 // -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
// #include <glm/glm.hpp>


namespace fc {

  void FcModel::loadGltfMeshes(std::filesystem::path filePath)
  {
    filePath = std::filesystem::path{R"(..//models//basicmesh.glb)"};
    //  // First check for errors with files
    // if (!std::filesystem::exists(filePath))
    // {
    //   throw std::runtime_error("Directory does not exist");
    // }
    // else
    // {
    //   std::cout << "Directory: " << filePath.parent_path() << std::endl;
    //   std::ofstream file(filePath);
    //   if (!file.is_open())
    //   {
    //     throw std::runtime_error("file was unable to load");
    //   }
    //   else
    //   {
    //     file.close();
    std::cout << "Loading GLTF: " << filePath.string() << std::endl;
    //   }
    // }

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

    fcLog("opened file");
    auto load = parser.loadGltfBinary(data.get(), filePath.parent_path(), gltfOptions);

    fcLog("attached parser");
    if (load)
    {
      gltf = std::move(load.get());
    }
    else
    {
      std::cout << "Failed to load glTF: " << fastgltf::to_underlying(load.error()) << std::endl;
       // TODO still need a way to return null or empty
      return;
    }



     // use the same vectors for all meshes so that the memory doesn't reallocate as often
     // TODO std::move
    std::vector<uint32_t> indices;
    std::vector<Vertex2> vertices;

    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
       // clear the mesh arrays each mesh, we don't want to merge them by error
       // TODO change if can since these operations are O(N) (has to call destructor for each element)
      indices.clear();
      vertices.clear();

       // TODO this isn't right
      uint32_t startIndex;
      uint32_t Indexcount;

      for (auto&& primitive : mesh.primitives)
      {
         // Surface newSurface;
         // newSurface.startIndex = static_cast<uint32_t>(indices.size());
         // newSurface.count = static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

        startIndex = static_cast<uint32_t>(indices.size());
        Indexcount = static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

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
                                                          Vertex2 newVertex;
                                                          newVertex.position = v;
                                                          newVertex.normal = { 1, 0, 0 };
                                                          newVertex.color = glm::vec4 { 1.f };
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

         // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX COLORS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
        auto colors = primitive.findAttribute("COLOR_0");
        if (colors != primitive.attributes.end())
        {
          fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).accessorIndex]
                                                        , [&](glm::vec4 vec, size_t index) {
                                                          vertices[initialVertex + index].color = vec;
                                                        });
        }

      }

       // -*-*-*-*-*-*-*-*-*-*-*-*-   DISPLAY VERTEX NORMALS   -*-*-*-*-*-*-*-*-*-*-*-*- //
      constexpr bool OverrideColors = true;
      if (OverrideColors)
      {
        for (Vertex2& vertex : vertices)
        {
          vertex.color = glm::vec4(vertex.normal, 1.f);
        }
      }

       // TODO start here with optimizations, including a new constructor with name
      FcMesh newMesh;
      newMesh.uploadMesh2(name, vertices, indices);
      newMesh.setIndexCounts(startIndex, Indexcount);
      mMeshList.emplace_back(std::move(newMesh));

       // TODO try and implement with shared pointers to meshes
       // std::vector<std::shared_ptr<FcMesh>> meshes;
       // mMeshList.emplace_back(std::make_shared<FcMesh>(std::move(newMesh)));
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



  FcModel::FcModel(std::string modelFile)
  {
     // first initialze model matrix to identity
    mModelMatrix = glm::mat4(1.0f);

     // import model "scene"
    Assimp::Importer importer;

     // ?? I believe the aiProcess_JoinIdenticalVertices will remove the duplicate vertices so that
     // each is unique but should verify that it's working correctly by using a unordered set (w/
     // hashmap) to store the vertices and make sure we get the same number for both
    const aiScene* scene = importer.ReadFile(modelFile, aiProcess_Triangulate
                                             | aiProcess_FlipUVs
                                             | aiProcess_JoinIdenticalVertices);
    if (!scene)
    {
      throw std::runtime_error("Failed to load model! (" + modelFile + ")");
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
        materialNumToTexID[i] = 0;
      }
      else
      {	// Otherwise, create texture and set value to index of new texture
        materialNumToTexID[i] = loadTexture(textureNames[i]);
      }
    }

     // load in all our meshes
    loadNodes(scene->mRootNode, scene, materialNumToTexID);

  } // --- FcModel::FcModel (_) --- (END)




   // recursively load all the nodes from a tree of nodes within the scene
  void FcModel::loadNodes(aiNode* node
                          , const aiScene* scene, std::vector<int>& matToTex)
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
     // go through each nod attached to this node and load it, then append their meshes to this
     // node's mesh list
    for (size_t i = 0; i < node->mNumChildren; ++i)
    {
      loadNodes(node->mChildren[i], scene, matToTex);
    }
  }


  void FcModel::loadMesh(aiMesh* mesh, const aiScene* scene, uint32_t textureID)
  {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

     // resize vertex list to hold all vertices for mesh
    vertices.resize(mesh->mNumVertices);

     // go through each vertex and copy it across to our vertices
    for (size_t i = 0; i < vertices.size(); ++i)
    {
       // set position
      vertices[i].position = {mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z};

       // TODO get rid of color from Vertex struct
       // set color (just use random color for now)
      vertices[i].color = { 0.62f, 0.7f, 0.08f };

       // set the vertex normal
       // ?? not sure if this is right
      vertices[i].normal = {mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z};

       // set texture coordinats (ONLY if they exist) (note that [0] here refers to the first SET )
      if (mesh->mTextureCoords[0])
      {
        vertices[i].texCoord = { mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
      }
      else
      {
        vertices[i].texCoord = { 0.0f, 0.0f };
      }
    }

     // iterate over indices through faces and copy across
     // TODO might be better to iterate over this for loop once to resize indices() first, then fill it
    for (size_t i = 0; i < mesh->mNumFaces; ++i)
    {
       // get a face
      aiFace face = mesh->mFaces[i];
       // go through face's indices and add to list
      for (size_t j = 0; j < face.mNumIndices; ++j)
      {
        indices.push_back(face.mIndices[j]);
      }
    }

     // create a new mesh and return it TODO - make efficient
     //FcMesh newMesh;
     //newMesh.createMesh(&gpu, vertices, indices, textureID);
     //mMeshList.push_back(newMesh);
    mMeshList.emplace_back(vertices, indices, textureID);
  }


   // TODO make efficient
  uint32_t FcModel::loadTexture(std::string filename)
  {
    FcImage texture;

    uint32_t mDescriptorId = texture.loadTexture(filename);

    mTextures.emplace_back(std::move(texture));
     //mTextures.push_back(descriptorManager, filename);

    return mDescriptorId;
  }




  void FcModel::destroy()
  {

    for (auto& mesh : mMeshList)
    {
      mesh.destroy();
    }

    for (auto& texture : mTextures)
    {
      texture.destroy();
    }
  }



} // namespace fc _END_
