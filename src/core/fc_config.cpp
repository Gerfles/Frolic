//>--- fc_config.cpp ---<//
#include "fc_config.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "log.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <string>
#include <unordered_set>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  //
  void FcConfig::populateVulkanPhysicalDeviceFeatures(VkPhysicalDeviceFeatures2& features_1_0) noexcept
  {
    features_1_0.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features_1_0.features.samplerAnisotropy = samplerAnisotropy;
    features_1_0.features.shaderStorageImageMultisample = shaderStorageImageMultisample;
    features_1_0.features.sampleRateShading = sampleRateShading;
    features_1_0.features.geometryShader = geometryShader;
    features_1_0.features.tessellationShader = tessellationShader;
    features_1_0.features.fillModeNonSolid = fillModeNonSolid;

    // First find the additional required features since we only want to pass the first set from Vulkan1.0
    VkPhysicalDeviceVulkan11Features* features_1_1 = VK_NULL_HANDLE;
    if (features_1_0.pNext != VK_NULL_HANDLE)
    {
      features_1_1 = reinterpret_cast<VkPhysicalDeviceVulkan11Features*>(features_1_0.pNext);
      //
      features_1_1->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    }

    //
    VkPhysicalDeviceVulkan12Features* features_1_2 = VK_NULL_HANDLE;
    if (features_1_1 != VK_NULL_HANDLE && features_1_1->pNext != VK_NULL_HANDLE)
    {
      features_1_2 = reinterpret_cast<VkPhysicalDeviceVulkan12Features*>(features_1_1->pNext);
      //
      features_1_2->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
      features_1_2->descriptorBindingSampledImageUpdateAfterBind = descriptorBindingSampledImageUpdateAfterBind;
      features_1_2->descriptorBindingStorageImageUpdateAfterBind = descriptorBindingStorageImageUpdateAfterBind;
      features_1_2->shaderSampledImageArrayNonUniformIndexing = shaderSampledImageArrayNonUniformIndexing;
      features_1_2->bufferDeviceAddress = bufferDeviceAddress;
      features_1_2->descriptorIndexing = descriptorIndexing;
      features_1_2->descriptorBindingPartiallyBound = descriptorBindingPartiallyBound;
      features_1_2->runtimeDescriptorArray = runtimeDescriptorArray;
    }

    //
    VkPhysicalDeviceVulkan13Features* features_1_3;
    if (features_1_2 != VK_NULL_HANDLE && features_1_2->pNext != VK_NULL_HANDLE)
    {
      features_1_3 = reinterpret_cast<VkPhysicalDeviceVulkan13Features*>(features_1_2->pNext);
      //
      features_1_3->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
      features_1_3->dynamicRendering = dynamicRendering;
      features_1_3->synchronization2 = synchronization2;
    }
  }


  //
  const bool FcConfig::areDeviceFeaturesSupported(VkPhysicalDevice device,
                                                  VkPhysicalDeviceFeatures2& requiredFeatures_1_0) noexcept
  {
    // Initialize all potential requiredFeatures versions even though some may not be available
    VkPhysicalDeviceVulkan11Features* requiredFeatures_1_1 = VK_NULL_HANDLE;
    VkPhysicalDeviceVulkan12Features* requiredFeatures_1_2 = VK_NULL_HANDLE;
    VkPhysicalDeviceVulkan13Features* requiredFeatures_1_3 = VK_NULL_HANDLE;

    // Incrementally check what's available and only reinterpret pointer to it if we determine it exists
    if (requiredFeatures_1_0.pNext != VK_NULL_HANDLE)
    {
      requiredFeatures_1_1 = reinterpret_cast<VkPhysicalDeviceVulkan11Features*>(requiredFeatures_1_0.pNext);

      if (requiredFeatures_1_1->pNext != VK_NULL_HANDLE)
      {
        requiredFeatures_1_2 = reinterpret_cast<VkPhysicalDeviceVulkan12Features*>(requiredFeatures_1_1->pNext);

        if (requiredFeatures_1_2->pNext != VK_NULL_HANDLE)
        {
          requiredFeatures_1_3 = reinterpret_cast<VkPhysicalDeviceVulkan13Features*>(requiredFeatures_1_2->pNext);
        }
      }
    }

    // Used to check if we have any required features that were unavailable and print those later
    std::string missingFeatures;

    // Next figure out what all the features available to us are
    VkPhysicalDeviceVulkan13Features availableFeatures_1_3 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES
    };

    VkPhysicalDeviceVulkan12Features availableFeatures_1_2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
    , .pNext = &availableFeatures_1_3
    };

    VkPhysicalDeviceVulkan11Features availableFeatures_1_1 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES
    , .pNext = &availableFeatures_1_2
    };

    VkPhysicalDeviceFeatures2 availableFeatures_1_0 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2
    , .pNext = &availableFeatures_1_1
    };

    vkGetPhysicalDeviceFeatures2(device, &availableFeatures_1_0);

    // Macros for checking support of features and printing missing features for debugging
#define CHECK_DEVICE_FEATURES(requiredFeature, availableFeature, feature, version) \
    if ((requiredFeature) && ( !availableFeature))                      \
    {                                                                   \
      missingFeatures.append("\n\t" "VkPhysicalDeviceFeatures_" version "_" #feature); \
    }

#define CHECK_VULKAN_1_0_FEATURE(feature)                               \
    CHECK_DEVICE_FEATURES(requiredFeatures_1_0.features.feature, availableFeatures_1_0.features.feature, feature, "1.0")
#define CHECK_VULKAN_1_1_FEATURE(feature)                               \
    CHECK_DEVICE_FEATURES(requiredFeatures_1_1->feature, availableFeatures_1_1.feature, feature, "1.1")
#define CHECK_VULKAN_1_2_FEATURE(feature)                               \
    CHECK_DEVICE_FEATURES(requiredFeatures_1_2->feature, availableFeatures_1_2.feature, feature, "1.2")
#define CHECK_VULKAN_1_3_FEATURE(feature)                               \
    CHECK_DEVICE_FEATURES(requiredFeatures_1_3-feature, availableFeatures_1_3.feature, feature, "1.3")

    // Verify Vulkan 1.0 Features
    CHECK_VULKAN_1_0_FEATURE(samplerAnisotropy);
    CHECK_VULKAN_1_0_FEATURE(shaderStorageImageMultisample);
    CHECK_VULKAN_1_0_FEATURE(sampleRateShading);
    CHECK_VULKAN_1_0_FEATURE(geometryShader);
    CHECK_VULKAN_1_0_FEATURE(tessellationShader);
    CHECK_VULKAN_1_0_FEATURE(fillModeNonSolid);

    // Verify Vulkan 1.2 Features
    CHECK_VULKAN_1_2_FEATURE(descriptorBindingSampledImageUpdateAfterBind);
    CHECK_VULKAN_1_2_FEATURE(descriptorBindingStorageImageUpdateAfterBind);
    CHECK_VULKAN_1_2_FEATURE(shaderSampledImageArrayNonUniformIndexing);
    CHECK_VULKAN_1_2_FEATURE(bufferDeviceAddress);
    CHECK_VULKAN_1_2_FEATURE(descriptorIndexing);
    CHECK_VULKAN_1_2_FEATURE(descriptorBindingPartiallyBound);
    CHECK_VULKAN_1_2_FEATURE(runtimeDescriptorArray);

    // Verify Vulkan 1.3 Features
    CHECK_VULKAN_1_3_FEATURE(dynamicRendering);
    CHECK_VULKAN_1_3_FEATURE(synchronization2);

#undef CHECK_VULKAN_1_3_FEATURE
#undef CHECK_VULKAN_1_2_FEATURE
#undef CHECK_VULKAN_1_1_FEATURE
#undef CHECK_VULKAN_1_0_FEATURE
#undef CHECK_DEVICE_FEATURES

    if ( !missingFeatures.empty())
    {
      fcPrintEndl("Missing Vulkan features: %s\n", missingFeatures.c_str());
      return false;
    }

    return true;
  }


  //
  const bool FcConfig::areExtensionsSupported(std::vector<const char*>& extensionsOrLayers,
                                              FeatureType type, VkPhysicalDevice device) noexcept
  {
    u32 extensionsCount = 0;
    std::vector<const char*> availablelayersOrExtensions;

    // acquire all availabe Device Extensions and add their name to availableLayersOrExtensions
    if (type == FeatureType::DeviceExtension)
    {
      vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);

      std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
      vkEnumerateDeviceExtensionProperties(device, nullptr
                                           , &extensionsCount, availableExtensions.data());

      for (VkExtensionProperties extension : availableExtensions)
      {
        /* fcPrintEndl("Available Vulkan Device Extension: %s", extension.extensionName); */
        availablelayersOrExtensions.push_back(extension.extensionName);
      }
    }
    // acquire all available validation layers and add their name to availableLayersOrExtensions
    else if (type == FeatureType::ValidationLayer)
    {
      vkEnumerateInstanceLayerProperties(&extensionsCount, nullptr);
      std::vector<VkLayerProperties> availableLayers(extensionsCount);
      vkEnumerateInstanceLayerProperties(&extensionsCount, availableLayers.data());

      for (VkLayerProperties& layer : availableLayers)
      {
        /* fcPrintEndl("Available Vulkan Layer: %s", layer.layerName); */
        availablelayersOrExtensions.push_back(layer.layerName);
      }
    }
    // acquire all available Instance extensions and add their name to availableLayersOrExtensions
    else if (type == FeatureType::InstanceExtension)
    {
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
      std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
      vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.data());

      for (VkExtensionProperties& extension : availableExtensions)
      {
        /* fcPrintEndl("Available Vulkan Instance Extension: %s", extension.extensionName); */
        availablelayersOrExtensions.push_back(extension.extensionName);
      }
    }

    if (extensionsCount == 0) {
      return false;
    }

    // add all the required extentions to an unordered set for easy removal
    std::unordered_set<std::string> requiredExtensionsOrLayers(extensionsOrLayers.begin(),
                                                               extensionsOrLayers.end());

    for (const char* extensionOrLayer : availablelayersOrExtensions)
    {
      requiredExtensionsOrLayers.erase(extensionOrLayer);
    }

    return requiredExtensionsOrLayers.empty();
  }


      } // --- namespace fc --- (END)
