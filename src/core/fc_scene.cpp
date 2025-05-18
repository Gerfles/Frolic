// fc_scene.cpp
#include "fc_scene.hpp"

// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   FROLIC   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_locator.hpp"
// TODO rename utilities to fc_utilities
#include "utilities.hpp"
// DELETE
#include "fc_renderer.hpp"
#include "fc_mesh.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   EXTERNAL *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-
// GLTF loading
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
// Matrix manipulation
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>


namespace fc
{
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

    for (auto& image : mTextures)
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

    // for (auto& sampler : mSamplers)
    // {
    //   vkDestroySampler(device, sampler, nullptr);
    // }
  }


  // TODO combine with below method (comments and such) then delete below
  // TODO brush up on the modern use of fastgltf -- seems vkguide is not the most up to date
  void FcScene::loadGltf(FcSceneRenderer& sceneRenderer, std::string_view filepath)
  {
    // TODO delete renderer and pDevice dependencies
    VkDevice pDevice = FcLocator::Device();

    std::cout << "Loading GLTF file: " << filepath << std::endl;

    constexpr fastgltf::Extensions extensions =
      fastgltf::Extensions::KHR_materials_clearcoat
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

    // TODO handle more cases with fastgltf samplers and handle mipmap and wrap.
    // Load samplers using helper functions that translate from OpenGL to vulkan specs
    for (fastgltf::Sampler& sampler : gltf.samplers)
    {
      if (sampler.minFilter.has_value() && sampler.magFilter.has_value())
      {
        // TODO not used yet
        VkSamplerMipmapMode mipMode;
        mipMode = extractMipmapMode(sampler.minFilter.value_or(fastgltf::Filter::Linear));

        if (sampler.minFilter.value() == fastgltf::Filter::Nearest
            && sampler.magFilter.value() == fastgltf::Filter::Nearest)
        {
          fcLog("USING NEAREST VKSAMPLER");
          // Get a pointer to default nearest sampler
          pSamplers.push_back(&FcDefaults::Samplers.Nearest);
        }
        else
        {
          if (mipMode == VK_SAMPLER_MIPMAP_MODE_NEAREST)
          {
            fcLog("USING BILINEAR VKSAMPLER");
            // Get a pointer to default linear sampler
            pSamplers.push_back(&FcDefaults::Samplers.Bilinear);
          }
          else if (mipMode == VK_SAMPLER_MIPMAP_MODE_LINEAR)
          {
            fcLog("USING TRILINEAR VKSAMPLER");
            // Get a pointer to default linear sampler
            pSamplers.push_back(&FcDefaults::Samplers.Trilinear);
          }
          else
          {
            fcLog("USING LINEAR VKSAMPLER");
            // Get a pointer to default linear sampler
            pSamplers.push_back(&FcDefaults::Samplers.Linear);
          }

        }
      }
      else
      {
        fcLog("USING DEFAULT VKSAMPLER");
        // Get a pointer to default linear sampler
        pSamplers.push_back(&FcDefaults::Samplers.Trilinear);
      }
    }

    // Temporary arrays for all the objects to use while creating the GLTF data
    // using this to first collect data then store in unordered maps later
    std::vector<std::shared_ptr<FcMesh>> meshes;
    std::vector<std::shared_ptr<FcNode>> nodes;
    std::vector<FcImage> images;
    //std::vector<std::shared_ptr<GLTFMaterial>> materials;

    // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD ALL TEXTURES   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    // FIXME don't create 3 images!!
    uint32_t size = 0;
    for (fastgltf::Image& gltfImage : gltf.images)
    {
      FcImage fcImage;

      fcImage.loadFromGltf(parentPath, gltf, gltfImage);
      if (fcImage.isValid())
      {
        //images.push_back(fcImage);
        //mImages[gltfImage.name.c_str()] = fcImage;
        mTextures.push_back(fcImage);// = fcImage;


      }
      else
      { // failed to load image so assign default image so we can continue loading scene
        mTextures.push_back(FcDefaults::Textures.checkerboard);
        std::cout << "Failed to load texture: " << gltfImage.name << std::endl;
      }
      size++;
    }

    std::cout << "Number of Textures Loaded: " << size << std::endl;

    // Create buffer to hold the material data
    mMaterialDataBuffer.allocate(sizeof(MaterialConstants) * gltf.materials.size()
                                 , FcBufferTypes::Uniform);


    // ?? do we need to also duplicate here??
    MaterialConstants* sceneMaterialConstants =
      static_cast<MaterialConstants*>(mMaterialDataBuffer.getAddress());



    // Preallocate material vector
    // TODO ?? eliminate materials in favor of two separate but equally sized vectors (vkDescriptorSet / type)
    mNumMaterials = gltf.materials.size();
    std::vector<std::shared_ptr<FcMaterial>> materials(mNumMaterials);
    std::cout << "Number of material in Scene: " << mNumMaterials << std::endl;

    /* materials.resize(gltf.materials.size()); */

    // -*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD ALL MATERIALS   -*-*-*-*-*-*-*-*-*-*-*-*-*- //
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
                             , mTextures[imageIndex]
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , *pSamplers[samplerIndex]);
      }
      else {  // set to defualt texture/sampler/values if none exist in material
        bindInfo.attachImage(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , FcDefaults::Textures.checkerboard
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , FcDefaults::Samplers.Linear);
      }

      // *-*-*-*-*-*-*-*-*-*-*-   MATERIAL METAL-ROUGH TEXTURE   *-*-*-*-*-*-*-*-*-*-*- //
      if (material.pbrData.metallicRoughnessTexture.has_value())
      {
        constants.flags |= MaterialFeatures::HasRoughMetalTexture;

        size_t index = material.pbrData.metallicRoughnessTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();

        bindInfo.attachImage(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , mTextures[imageIndex]
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , *pSamplers[samplerIndex]);
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

        bindInfo.attachImage(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , mTextures[imageIndex]
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , *pSamplers[samplerIndex]);
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

        size_t index = material.occlusionTexture.value().textureIndex;
        size_t imageIndex = gltf.textures[index].imageIndex.value();
        size_t samplerIndex = gltf.textures[index].samplerIndex.value();

        bindInfo.attachImage(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
                             , mTextures[imageIndex]
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , *pSamplers[samplerIndex]);

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
                             , mTextures[imageIndex]
                             , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                             , *pSamplers[samplerIndex]);
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
      sceneMaterialConstants[i] = constants;

      // ?? should this be outside of loop
      // Build descriptor sets for each material
      //createMaterialDescriptorSets(sceneRenderer);

      materials[i]->materialSet
        = FcLocator::DescriptorClerk().createDescriptorSet(mMaterialDescriptorLayout, bindInfo);

      /* newMaterial->data = renderer->mSceneRenderer.writeMaterial(pDevice, passType, materialResources); */
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

      //gltf.materials[primitive.materialIndex.value()].pbrData.;

      // clear the mesh arrays each mesh, we don't want to merge them by error
      // TODO change if can since these operations are O(N) (has to call destructor for each element)
      indices.clear();
      vertices.clear();

      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   LOAD INDICES   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      for (auto&& primitive : mesh.primitives)
      {
        FcSubMesh newSurface;
        newSurface.startIndex = static_cast<uint32_t>(indices.size());
        newSurface.indexCount = static_cast
                                <uint32_t>(gltf.accessors[primitive.indicesAccessor.value()].count);

        size_t initialVertex = vertices.size();

        fastgltf::Accessor& indexAcccessor = gltf.accessors[primitive.indicesAccessor.value()];
        indices.reserve(indices.size() + indexAcccessor.count);

        fastgltf::iterateAccessor<std::uint32_t>(gltf, indexAcccessor
                                                 , [&](std::uint32_t index)
                                                  {
                                                    indices.push_back(initialVertex + index);
                                                  });

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
          // NOTE: Left for reference
          // fastgltf::iterateAccessorWithIndex<glm::vec4>(gltf, gltf.accessors[(*colors).accessorIndex]
          //                                               , [&](glm::vec4 vec, size_t index)
          //                                                {
          //                                                  vertices[initialVertex + index].color = vec;
          //                                                });
        }


        if (primitive.materialIndex.has_value())
        {
          newSurface.material = materials[primitive.materialIndex.value()];
        } else  {
          newSurface.material = materials[0];

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
    std::cout << "Number of loaded Meshes: " << meshes.size() << std::endl;
    std::cout << "Number of Created FcMeshNodes: ";
    uint32_t meshNodeCount{0};

    for (fastgltf::Node& gltfNode : gltf.nodes)
    {
      std::shared_ptr<FcNode> newNode;

      // find if the gltfNode has a mesh, if so, hook it it to the mesh pointer and allocate it with MeshNode class
      if (gltfNode.meshIndex.has_value())
      {
        newNode = std::make_shared<FcMeshNode>();
        static_cast<FcMeshNode*>(newNode.get())->mMesh = meshes[*gltfNode.meshIndex];
        meshNodeCount++;
      } else {
        newNode = std::make_shared<FcNode>();
      }

      nodes.push_back(newNode);
      mNodes[gltfNode.name.c_str()];

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
        gltfNode.transform);
    }

    std::cout << meshNodeCount << std::endl;

    // Run another loop over the nodes to setup transform hierarchy
    for (int i = 0; i < gltf.nodes.size(); i++)
    {
      fastgltf::Node& gltfNode = gltf.nodes[i];
      std::shared_ptr<FcNode>& sceneNode = nodes[i];

      for (auto& child : gltfNode.children)
      {
        sceneNode->children.push_back(nodes[child]);
        nodes[child]->parent = sceneNode;
      }
    }

    // Find the top nodes, with no parents
    for (auto& gltfNode : nodes)
    {
      if (gltfNode->parent.lock() == nullptr)
      {
        mTopNodes.push_back(gltfNode);
        // ??Not sure why we would need this?? I think it's just an initialization thing
        // if so, better done in class
        gltfNode->update(glm::mat4{1.f});
      }
    }
  }



  // glTF samplers use the numbers and properties of OpenGL, so create conversion functions
  VkFilter FcScene::extractFilter(fastgltf::Filter filter)
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
  VkSamplerMipmapMode FcScene::extractMipmapMode(fastgltf::Filter filter)
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


  void FcScene::addToDrawCollection(FcDrawCollection& collection)
  {
    // probably best to just add a new pair as needed
    // size_t currentSize = collection.opaqueSurfaces.size();
    // collection.opaqueSurfaces.resize(currentSize + mNumMaterials);

    // Only loop the topnodes which will in turn call draw on their child nodes
    for (std::shared_ptr<FcNode>& node : mTopNodes)
    {
      node->addToDrawCollection(collection);
    }
  }


  void FcScene::update(const glm::mat4& topMatrix)
  {

    for (std::shared_ptr<FcNode>& node : mTopNodes)
    {
      node->update(topMatrix);
    }
  }


  // TODO rename most of these function to what they actually do - ie generateTextureList();
  std::vector<std::string> FcScene::LoadMaterials(const aiScene* scene)
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
        //materialNumToTexID[i] = loadTexture(textureNames[i]);
        materialNumToTexID[i] = loadTexture(textureNames[i], descriptorLayout);
        // TODO maybe save descriptorset to mesh
      }
    }

    // load in all our meshes
    loadAssimpNodes(scene->mRootNode, scene, materialNumToTexID);

  } // --- FcScene::FcScene (_) --- (END)



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



  uint32_t FcScene::loadTexture(std::string filename, VkDescriptorSetLayout layout)
  {
    // BUG from fcModel but should either be deleted or updated for scene
//     // TODO i think the texture is being copied here, don't do that !!
//     FcImage texture;
// //    texture.loadTexture(filename);
//     mTextures.emplace_back(std::move(texture));

//     FcDescriptorBindInfo bindInfo{};
//     bindInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, texture
//                          // TODO attach a default or passed in sampler
//                          , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL , nullptr);

//     VkDescriptorSet descriptorSet;
//     descriptorSet = FcLocator::DescriptorClerk().createDescriptorSet(layout, bindInfo);
//     mDescriptorSets.emplace_back(std::move(descriptorSet));
    return mTextures.size() - 1;
  }


  void FcScene::destroy()
  {
    // for (auto& mesh : mMeshList)
    // {
    //   mesh.destroy();
    // }

    // for (auto& texture : mTextures)
    // {
    //   texture.destroy();
    // }

    clearAll();

  }







}// --- namespace fc --- (END)
