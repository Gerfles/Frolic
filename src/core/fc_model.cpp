#include "fc_model.hpp"

// - FROLIC ENGINE -
#include "SDL2/SDL_stdinc.h"
#include "core/fc_descriptors.hpp"
#include "core/fc_gpu.hpp"
#include "core/fc_image.hpp"
#include "core/fc_locator.hpp"
#include "fc_mesh.hpp"
// - EXTERNAL LIBRARIES -
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
// - STD LIBRARIES -


namespace fc {


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
     // first initialze model matrix to identity, to clear out any garbage
    mModelMatrix = glm::mat4(1.0f);

     // import model "scene"
    Assimp::Importer importer;

     // ?? I believe the aiProcess_JoinIdenticalVertices will remove the duplicate vertices so that
     // each is unique but should verify that it's working correctly by using a unordered set (w/
     // hashmap) to store the vertices and make sure we get the same number for both
    const aiScene* scene = importer.ReadFile(modelFile, aiProcess_Triangulate
                                              //| aiProcess_FlipUVs
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
    loadNodes(FcLocator::Gpu(), scene->mRootNode, scene, materialNumToTexID);
  }



   // recursively load all the nodes from a tree of nodes within the scene
  void FcModel::loadNodes(const FcGpu& gpu, aiNode* node, const aiScene* scene, std::vector<int>& matToTex)
  {
     // go through each mesh at this node and create it, then add it to our meshList ()
    for (size_t i = 0; i < node->mNumMeshes; ++i)
    {
       // note that the actual meshes are contained within the aiScene object and the mMeshes in the
       // node object just contains the indices that corresponds to the texture in the scene
      aiMesh* currMesh = scene->mMeshes[node->mMeshes[i]];
       //
      loadMesh(gpu, currMesh, scene, matToTex[currMesh->mMaterialIndex]);
    }
     // go through each nod attached to this node and load it, then append their meshes to this
     // node's mesh list
    for (size_t i = 0; i < node->mNumChildren; ++i)
    {
      loadNodes(gpu, node->mChildren[i], scene, matToTex);
    }
  }


  void FcModel::loadMesh(const FcGpu& gpu, aiMesh* mesh, const aiScene* scene, uint32_t textureID)
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
    mMeshList.emplace_back(&gpu, vertices, indices, textureID);
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
