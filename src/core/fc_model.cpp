#include "fc_model.hpp"

// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC ENGINE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// #include "fc_gpu.hpp"
#include "utilities.hpp"
#include "fc_descriptors.hpp"
#include "fc_image.hpp"
#include "fc_locator.hpp"
#include "fc_mesh.hpp"
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


  void LoadedGLTF::clearAll()
  {
    VkDevice device = FcLocator::Device();

    FcDescriptorClerk mDescriptorClerk;
    mDescriptorClerk.destroy();
    mMaterialDataBuffer.destroy();

    for (auto& [key, val] : mMeshes)
    {
      val->destroy();
    }

    for (auto& image : mImages)
    {
      image.destroy();
    }

    // for (auto& [key, val] : mImages)
    // {
    //   // TODO get rid of dependency on FcRenderer
    //   // Make sure we don't destroy the default image
    //   if (val.Image() != pCreator->mCheckerboardTexture.Image())
    //   {
    //     val.destroy();
    //   }
    // }

    for (auto& sampler : mSamplers)
    {
      vkDestroySampler(device, sampler, nullptr);
    }
  }


// TODO combine with below method (comments and such) then delete below
  // TODO brush up on the modern use of fastgltf -- seems vkguide is not the most up to date
  void LoadedGLTF::loadGltf(FcRenderer* renderer, std::string_view filepath)
  {
    // TODO delete renderer and pDevice dependencies
    VkDevice pDevice = FcLocator::Device();
    pCreator = renderer;

    std::cout << "Loading GLTF file: " << filepath << std::endl;

    constexpr fastgltf::Extensions extensions = fastgltf::Extensions::KHR_materials_clearcoat
                                                | fastgltf::Extensions::KHR_materials_transmission;

    fastgltf::Parser parser{extensions};

    constexpr fastgltf::Options gltfOptions = fastgltf::Options::DontRequireValidAssetMember
                                              | fastgltf::Options::AllowDouble
                                              | fastgltf::Options::LoadExternalBuffers;
    //                               | fastgltf::Options::LoadExternalImages;



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
        std:: cout << gltf.extensionsUsed[i] << std::endl;
      }

      gltf = std::move(load.get());
    }

    // we can estimate the descriptors we will need accurately
    // TODO FIXME descriptor clerk
    // std::vector<PoolSizeRatio> poolRatios = { {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
    //                                           {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1},
    //                                           {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3} };
    // //
    // mDescriptorClerk.initDescriptorPools(gltf.materials.size(), poolRatios);

    // Load samplers using helper functions that translate from OpenGL to vulkan specs
    for (fastgltf::Sampler& sampler : gltf.samplers)
    {
      VkSamplerCreateInfo samplerInfo{};
      samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
      samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
      samplerInfo.minLod = 0;

      // TODO simplify the following by just passing in the sampler and extracting details
      samplerInfo.magFilter = extractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
      samplerInfo.minFilter = extractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

      samplerInfo.mipmapMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

      VkSampler newSampler;
      vkCreateSampler(pDevice, &samplerInfo, nullptr, &newSampler);

      // TODO better to cache via hashing filters and just store globaly to reference
      // We are storing the samplers directly within the loadedGLTF class here
      mSamplers.push_back(newSampler);
    }

    // Temporary arrays for all the objects to use while creating the GLTF data
    // using this to first collect data then store in unordered maps later
    std::vector<std::shared_ptr<FcMesh>> meshes;
    std::vector<std::shared_ptr<Node>> nodes;
    std::vector<FcImage> images;
    //std::vector<std::shared_ptr<GLTFMaterial>> materials;

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD ALL TEXTURES   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // FIXME don't create 3 images!!
    uint32_t size = 0;
    for (fastgltf::Image& gltfImage : gltf.images)
    {
      FcImage fcImage;

      fcImage.loadTexture(parentPath, gltf, gltfImage);
      if (fcImage.isValid())
      {
        //images.push_back(fcImage);
        //mImages[gltfImage.name.c_str()] = fcImage;
        mImages.push_back(fcImage);// = fcImage;

      }
      else
      { // failed to load image so assign default image so we can continue loading scene
        //images.push_back(renderer->mCheckerboardTexture);
        mImages.push_back(renderer->mCheckerboardTexture);
        std::cout << "Failed to load texture: " << gltfImage.name << std::endl;
      }
    }

    // Create buffer to hold the material data
    mMaterialDataBuffer.allocateBuffer(
      sizeof(MaterialConstants) * gltf.materials.size()
      , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT);

    int dataIndex = 0;
    // ?? do we need to also duplicate here??
    MaterialConstants* sceneMaterialConstants =
      static_cast<MaterialConstants*>(mMaterialDataBuffer.getAddres());

    for (fastgltf::Material& material : gltf.materials)
    {
      std::shared_ptr<GLTFMaterial> newMaterial = std::make_shared<GLTFMaterial>();
      //materials.push_back(newMaterial);
      mMaterials.push_back(newMaterial);
      //mMaterials[material.name.c_str()] = newMaterial;

      // TODO could sort here by materials and also perhaps alphaMode/alphaCutoff

      MaterialConstants constants;
      constants.colorFactors.x = material.pbrData.baseColorFactor[0];
      constants.colorFactors.y = material.pbrData.baseColorFactor[1];
      constants.colorFactors.z = material.pbrData.baseColorFactor[2];
      constants.colorFactors.w = material.pbrData.baseColorFactor[3];


      // TODO look into specular colors... These checks fail w/ seg fault on material.specular
      // if (material.specular->specularTexture.has_value())
      // {
      //   fcLog("Has unused specular texture!");
      // }

      // if (material.specular->specularColorTexture.has_value())
      // {
      //   fcLog("Has unused specular color texture!");
      // }

      constants.metalRoughFactors.x = material.pbrData.metallicFactor;
      constants.metalRoughFactors.y = material.pbrData.roughnessFactor;

      // Index of Refraction
      constants.iorF0 = pow((1 - material.ior) / (1 + material.ior), 2);
      // emmisive factors
      constants.emmisiveFactors = glm::vec4(material.emissiveFactor.x(), material.emissiveFactor.y()
                                            , material.emissiveFactor.z(), material.emissiveStrength);

      MaterialPass passType = MaterialPass::MainColor;
      if (material.alphaMode == fastgltf::AlphaMode::Blend)
      {
        //std::cout << "Adding transparent material" << std::endl;
        passType = MaterialPass::Transparent;
      }

      // default the material textures
      // FIXME should have available locally and statically
      GLTFMetallicRoughness::MaterialResources materialResources;
      // Default Textures
      materialResources.colorImage = renderer->mWhiteTexture;
      materialResources.metalRoughImage = renderer->mWhiteTexture;
      materialResources.normalTexture = renderer->mWhiteTexture;
      materialResources.occlusionTexture = renderer->mWhiteTexture;
      materialResources.emissiveTexture = renderer->mBlackTexture;
      // Default Samplers
      materialResources.colorSampler = renderer->mDefaultSamplerLinear;
      materialResources.metalRoughSampler = renderer->mDefaultSamplerLinear;
      materialResources.normalSampler = renderer->mDefaultSamplerLinear;
      materialResources.occlusionSampler = renderer->mDefaultSamplerLinear;
      materialResources.emissiveSampler = renderer->mDefaultSamplerLinear;
      // Default Factors
      //TODO check what to set as
      constants.occlusionFactor = 1.0f;


      // Set the uniform buffer for the material data
      materialResources.dataBuffer = mMaterialDataBuffer;
      materialResources.dataBufferOffset = dataIndex * sizeof(MaterialConstants);

      // grab textures from the glTF file
      if (material.pbrData.baseColorTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasColorTexture;

        size_t index = material.pbrData.baseColorTexture.value().textureIndex;
        size_t fcImage = gltf.textures[index].imageIndex.value();
        size_t sampler = gltf.textures[index].samplerIndex.value();

        materialResources.colorImage = mImages[fcImage];
        materialResources.colorSampler = mSamplers[sampler];
      }

      if (material.pbrData.metallicRoughnessTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasRoughMetalTexture;

        //std::cout << "Model has metal/roughness texture..." << std::endl;

        size_t index = material.pbrData.metallicRoughnessTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();

        materialResources.metalRoughImage = mImages[imageIndex];
        materialResources.colorSampler = mSamplers[samplerIndex];
      }


      if (material.normalTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasNormalTexture;

        //std::cout << "Model has Normal Map Texture: (Scale = "
        //<< material.normalTexture->scale << ")" << std::endl;

        size_t index = material.normalTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();

        materialResources.normalTexture = mImages[imageIndex];
        materialResources.normalSampler = mSamplers[samplerIndex];
      }


      if (material.occlusionTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasOcclusionTexture;

        std::cout << "Model has Occlusion Map Texture: (Scale = "
                  << material.occlusionTexture->strength
                  << ", Default = 1)" << std::endl;

        size_t index = material.occlusionTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();
        materialResources.occlusionTexture = mImages[imageIndex];
        materialResources.occlusionSampler = mSamplers[samplerIndex];

        // Occlusion Factors
        constants.occlusionFactor = material.occlusionTexture.value().strength;
      }


      if (material.emissiveTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasEmissiveTexture;

        std::cout << "Model has Emmision Map Texture..." << std::endl;
        size_t index = material.emissiveTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();

        materialResources.emissiveTexture = mImages[imageIndex];
        materialResources.emissiveSampler = mSamplers[samplerIndex];
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
        fcLog("Sheen Data");
        if (material.sheen->sheenColorTexture.has_value())
        {
          fcLog("Sheen Color Texture");
        }
      }

      if (material.transmission != nullptr)
      {
        if (material.transmission->transmissionTexture.has_value())
        {
          fcLog("Transmission Data");
        }

        float transmission = material.transmission->transmissionFactor;
        std::cout << "transmission factor: " << transmission << std::endl;
      }
      if (material.clearcoat != nullptr)
      {
        fcLog("Clearcoat Data");
      }
      if (material.anisotropy != nullptr)
      {
        fcLog("Anisotropy Data");
      }
      if (material.iridescence != nullptr)
      {
        fcLog("Iridescence Data");
      }

      // Write material parameters to buffer.
      sceneMaterialConstants[dataIndex] = constants;

      // Build material
      // FIXME using the global descriptorClerk here, may want to use the local one instead
      newMaterial->data = renderer->mMetalRoughMaterial.writeMaterial(pDevice, passType, materialResources);

      dataIndex++;
    }

    // use the same vectors for all meshes so that memory doesnt reallocate as often
    // TODO std::move
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    for (fastgltf::Mesh& mesh : gltf.meshes)
    {
      std::shared_ptr<FcMesh> newMesh = std::make_shared<FcMesh>();
      newMesh->mName = mesh.name;
      meshes.push_back(newMesh);
      mMeshes[mesh.name.c_str()] = newMesh;

      //gltf.materials[primitive.materialIndex.value()].pbrData.;

      // clear the mesh arrays each mesh, we don't want to merge them by error
      // TODO change if can since these operations are O(N) (has to call destructor for each element)
      indices.clear();
      vertices.clear();

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD INDICES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      for (auto&& primitive : mesh.primitives)
      {
        Surface newSurface;
        newSurface.startIndex = static_cast<uint32_t>(indices.size());
        newSurface.count = static_cast
                           <uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

        size_t initialVertex = vertices.size();

        // ?? not sure why these need to have the following scope
        {
          fastgltf::Accessor& indexAcccessor = gltf.accessors[primitive.indicesAccessor.value()];
          indices.reserve(indices.size() + indexAcccessor.count);

          fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAcccessor
                                                   , [&](std::uint32_t index)
                                                    {
                                                      indices.push_back(initialVertex + index);
                                                    });
        }

        // *-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD VERTEX POSITIONS   *-*-*-*-*-*-*-*-*-*-*-*-*- //
        {
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
        }
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
                                                           vertices[initialVertex + index].tangent = vec;
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
          fcLog("Model has un-utilized colors per vertex");
          // fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).accessorIndex]
          //                                               , [&](glm::vec4 vec, size_t index)
          //                                                {
          //                                                  vertices[initialVertex + index].color = vec;
          //                                                });
        }

        if (primitive.materialIndex.has_value())
        {
          newSurface.material = mMaterials[primitive.materialIndex.value()];
        } else  {
          newSurface.material = mMaterials[0];

          // Signal flags as to which attributes this material can expect from the vertices
          // ?? not sure if we could have a material that associated with two different
          // meshes, where one has an attribute that the other does not...
          //gltf.materials[primitive.materialIndex.value()].
        }

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
        newSurface.bounds.origin = (maxPos + minPos) * 0.5f;
        newSurface.bounds.extents = (maxPos - minPos) * 0.5f;
        newSurface.bounds.sphereRadius = glm::length(newSurface.bounds.extents);


        // f
        newMesh->mSurfaces.push_back(newSurface);
      }

      // TODO check that we're not causing superfluous calls to copy constructor / destructors
      // TODO start here with optimizations, including a new constructor with name
      // TODO consider goin back to vector refererence from span since we can mandate that...
      // however, this limit future implementations that prefer arrays, etc.
      newMesh->uploadMesh(std::span(vertices), std::span(indices));
      //meshes.emplace_back(std::make_shared<FcMesh>(std::move(newMesh)));
      // TODO create constructor for mesh so we can emplace it in place

    }

    // -*-*-*-*-*-*-*-*-*-*-   LOAD ALL NODES AND THEIR MESHES   -*-*-*-*-*-*-*-*-*-*- //
    for (fastgltf::Node& node : gltf.nodes)
    {
      std::shared_ptr<Node> newNode;

      // find if the node has a mesh, if so, hook it it to the mesh pointer and allocate it with MeshNode class
      if (node.meshIndex.has_value())
      {
        newNode = std::make_shared<MeshNode>();
        static_cast<MeshNode*>(newNode.get())->mesh = meshes[*node.meshIndex];
      } else {
        newNode = std::make_shared<Node>();
      }

      nodes.push_back(newNode);
      mNodes[node.name.c_str()];

      // BUG ?? not sure if this is correct check with https://fastgltf.readthedocs.io/latest/guides.html#id8
      std::visit(fastgltf::visitor {
          [&] (fastgltf::math::fmat4x4 matrix)
           {
             memcpy(&newNode->localTransform, matrix.data(), sizeof(matrix));
           },
            [&](fastgltf::TRS transform)
             {
               glm::vec3 tl(transform.translation[0], transform.translation[1], transform.translation[2]);
               glm::quat rot(transform.rotation[3], transform.rotation[0]
                             , transform.rotation[1], transform.rotation[2]);
               glm::vec3 sc(transform.scale[0], transform.scale[1], transform.scale[2]);
               //
               glm::mat4 tm = glm::translate(glm::mat4(1.f), tl);
               glm::mat4 rm = glm::toMat4(rot);
               glm::mat4 sm = glm::scale(glm::mat4(1.f), sc);
               //
               newNode->localTransform = tm * rm * sm;
             } },
        node.transform);
    }

    // Run another loop over the nodes to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++)
    {
      fastgltf::Node& node = gltf.nodes[i];
      std::shared_ptr<Node>& sceneNode = nodes[i];

      for (auto& child : node.children)
      {
        sceneNode->children.push_back(nodes[child]);
        nodes[child]->parent = sceneNode;
      }
    }

    // Find the top nodes, with no parents
    for (auto& node : nodes)
    {
      if (node->parent.lock() == nullptr)
      {
        mTopNodes.push_back(node);
        // ??Not sure why we would need this?? I think it's just an initialization thing
        // if so, better done in class
        node->refreshTransform(glm::mat4{1.f});
      }
    }

    uint32_t transparentCount{0};
    for (auto& material : mMaterials)
    {
      if (material->data.passType == MaterialPass::Transparent)
        transparentCount++;
    }
    std::cout << "Transparent Objects: " << transparentCount << std::endl;
    std::cout << "Opaque Objects: " << mMaterials.size() - transparentCount << std::endl;
  }




  // glTF samplers use the numbers and properties of OpenGL, so create conversion functions
  VkFilter LoadedGLTF::extractFilter(fastgltf::Filter filter)
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

  // TODO combine with above filter function if possible
  VkSamplerMipmapMode LoadedGLTF::extractMipmapMode(fastgltf::Filter filter)
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

  //void LoadedGLTF::draw(const glm::mat4& topMatrix, DrawContext& ctx)
  void LoadedGLTF::draw(DrawContext& ctx)
  {
    // Only loop the topnodes which will in turn call draw on their child nodes
    for (std::shared_ptr<Node>& node : mTopNodes)
    {
      node->draw(ctx);
    }
  }


  void LoadedGLTF::update(const glm::mat4& topMatrix)
  {

    for (std::shared_ptr<Node>& node : mTopNodes)
    {
      node->update(topMatrix);
    }
  }






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
      FcMesh newMesh;
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
        Surface newSurface;
        newSurface.startIndex = static_cast<uint32_t>(indices.size());
        newSurface.count = static_cast<uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

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

      FcMesh* add2 = meshes.back().get();
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
    for (auto& mesh : meshes)
    {
      mesh->destroy();
    }


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
