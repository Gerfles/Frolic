//>_ fc_billboard.cpp _<//
#include "fc_billboard.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "fc_assert.hpp"
#include "fc_handle.hpp"
#include "fc_draw_collection.hpp"
#include "fc_defaults.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //


namespace fc
{
void FcBillboard::loadTexture(FcDrawCollection& drawCollection, float width,
                                   float height, std::string_view filename) noexcept
  {
    mWidth = width;
    mHeight = height;

    std::filesystem::path file{filename};

    FcImage billboardTexture;
    billboardTexture.loadStbi(file, FcImageTypes::Texture);

    if (! billboardTexture.isValid())
    {
      // failed to load image so assign default image so we can continue loading scene
      billboardTexture = FcDefaults::Textures.checkerboard;
      FC_DEBUG_LOG_FORMAT("Failed to load texture: %s", filename);
    }

    // Get the handle assigned via resource pool
    /* FcHandle<FcImage> imgHandle = drawCollection.mBillboards.createElement(std::move(billboardTexture)); */
    FcHandle<FcImage> imgHandle = drawCollection.mTextures.createElement(std::move(billboardTexture));
    FC_ASSERT(imgHandle.isValid());

    // TODO check via configurations
    bool isBindlessSupported = true;
    if (isBindlessSupported)
    {
      ResourceUpdate resourceUpdate{ResourceDeletionType::Billboard
                                  , drawCollection.stats.frame
                                  , imgHandle};

      drawCollection.bindlessTextureUpdates.push_back(resourceUpdate);
    }
    else
    {
      // TODO Implement
      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   NON-BINDLESS   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
      // newTexture->loadStbi(filename, FcImageTypes::imgHandle);
      // FcDescriptorBindInfo descriptorInfo;
      // descriptorInfo.addBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      //                           VK_SHADER_STAGE_FRAGMENT_BIT);

      // descriptorInfo.attachImage(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
      //                            *newTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      //                            FcDefaults::Samplers.Linear);

      // FcDescriptorClerk& descClerk = FcLocator::DescriptorClerk();

      // // TODO create a single image descriptor layout for all types of textures, etc.
      // VkDescriptorSetLayout layout = descClerk.createDescriptorSetLayout(descriptorInfo);
      // mDescriptor = descClerk.createDescriptorSet(layout, descriptorInfo);
      // *-*-*-*-*-*-*-*-*-*-*-*-*-*-   END NON-BINDLESS   *-*-*-*-*-*-*-*-*-*-*-*-*-*- //
    }
  }

}// --- namespace fc --- (END)
