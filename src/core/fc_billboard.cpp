//>_ fc_billboard.cpp _<//
#include "fc_billboard.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_renderer.hpp"
#include "fc_defaults.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
  //
  //
  void FcBillboard::loadTexture(FcRenderer& renderer, float width
                                , float height, std::string_view filename)
  {
    mPush.width = width;
    mPush.height = height;

    FcDrawCollection& drawCollection = renderer.DrawCollection();
    FcImage* newTexture = drawCollection.mTextures.getNextFree();

    // TODO check that we only need to throw error in one place and the other make noexcept
    if (newTexture == nullptr)
    {
      throw std::runtime_error("Invalid texture index!");
    }

    std::filesystem::path file{filename};
    newTexture->loadStbi(file, FcImageTypes::Texture);

    if ( ! newTexture->isValid())
    {
      // failed to load image so assign default image so we can continue loading scene
      *newTexture = FcDefaults::Textures.checkerboard;
      std::cout << "Failed to load texture: " << filename << std::endl;
    }

    // TODO provide alternate path
    bool isBindlessSupported = true;
    if (isBindlessSupported)
    {
      mPush.textureIndex = newTexture->Handle();

      ResourceUpdate resourceUpdate{ResourceDeletionType::Texture
                                  , newTexture->Handle()
                                  , drawCollection.stats.frame};

      drawCollection.bindlessTextureUpdates.push_back(resourceUpdate);
    }
    else
    {
      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NON-BINDLESS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      // mTexture.loadStbi(filename, FcImageTypes::Texture);
      // FcDescriptorBindInfo descriptorInfo;
      // descriptorInfo.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      //                           VK_SHADER_STAGE_FRAGMENT_BIT);

      // descriptorInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      //                            mTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      //                            FcDefaults::Samplers.Linear);

      // FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

      // // TODO create a single image descriptor layout for all types of textures, etc.
      // VkDescriptorSetLayout layout = descClerk.createDescriptorSetLayout(descriptorInfo);
      // mDescriptor = descClerk.createDescriptorSet(layout, descriptorInfo);
      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NON-BINDLESS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    }
  }

}// --- namespace fc --- (END)
