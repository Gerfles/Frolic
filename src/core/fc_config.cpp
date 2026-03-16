//>--- fc_config.cpp ---<//
#include "fc_config.hpp"
// *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   CORE   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include "log.hpp"
#include "fc_window.hpp"
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-   STL   *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*- //
#include <stdexcept>
/* #include <string> */
#include <unordered_set>
// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* //


namespace fc
{
  // TEST that we can add to instance extensions before SDL_Vulkan retrieves data()
  void FcConfig::enableExtenedSwapchainColorSpace()
  {
    mInstanceExtensions.push_back(VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    enableExtendedSwapchainColorSpace = true;
  }

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

  void FcConfig::enableValidationLayers() noexcept
  {
    validationLayersEnabled = true;
    validationLayers.push_back("VK_LAYER_KHRONOS_validation");
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


  // TODO templatize and move to utilities
  const bool availableContainsRequired(std::vector<const char*>& availableElements,
                                       std::vector<const char*>& requiredElements )
  {
    if (availableElements.size() == 0) {
      return false;
    }

    std::unordered_set<std::string> requiredSet(requiredElements.begin(), requiredElements.end());

    for (auto& availableElement : availableElements)
    {
      /* fcPrintEndl("available Element: %s", availableElement); */
      requiredSet.erase(availableElement);
    }

    // if the set is empty, all the requiredElements elements are also in availableElements
    if (requiredSet.empty())
    {
      return true;
    }

    for (const std::string& element : requiredSet)
    {
      fcPrintEndl("Missing Element: %s", element.c_str());
    }

    return false;
  }


  //
  const bool FcConfig::areDeviceExtensionsSupported(VkPhysicalDevice device) noexcept
  {
    // acquire all availabe Device Extensions and add their name to availableLayersOrExtensions
    u32 extensionsCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionsCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr
                                         , &extensionsCount, availableExtensions.data());

    // add all the extension names to a vector for processing
    std::vector<const char*> availableExtensionNames;
    for (VkExtensionProperties& extension : availableExtensions)
    {
      /* fcPrintEndl("Available Vulkan Device Extension: %s", extension.extensionName); */
      availableExtensionNames.push_back(extension.extensionName);
    }

    return availableContainsRequired(availableExtensionNames, mDeviceExtensions);
  }


  //
  const bool FcConfig::areValidationLayersSupported() noexcept
  {
    // acquire all available validation layers and add their name to availableLayersOrExtensions
    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    // add all the layers names to a vector for processing
    std::vector<const char*> availableLayerNames;
    for (VkLayerProperties& layer : availableLayers)
    {
      /* fcPrintEndl("Available Vulkan Layer: %s", layer.layerName); */
      availableLayerNames.push_back(layer.layerName);
    }

    return availableContainsRequired(availableLayerNames, validationLayers);
  }


  //
  const bool FcConfig::areInstanceExtensionsSupported() noexcept
  {
    // acquire all available Instance extensions and add their name to availableLayersOrExtensions
    u32 extensionsCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionsCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionsCount, availableExtensions.data());

    // add all the extension names to a vector for processing
    std::vector<const char*> availableExtensionNames;
    for (VkExtensionProperties& extension : availableExtensions)
    {
      /* fcPrintEndl("Available Vulkan Instance Extension: %s", extension.extensionName); */
      availableExtensionNames.push_back(extension.extensionName);
    }

    return availableContainsRequired(availableExtensionNames, mInstanceExtensions);
  }


  //
  VkLayerSettingsCreateInfoEXT* FcConfig::getValidationLayerSettings()
  {
    if ( !validationLayersEnabled) {
      fcPrintEndl("No Validation Layers enabled");
      return VK_NULL_HANDLE;
    }

    // make sure our Vulkan drivers support these validation layers
    if (!areValidationLayersSupported())
    {
      throw std::runtime_error("Validation layers requested but not available!");
    }

    // TODO may not be available on linux so should probably provide alternate path
    // enable the best practices layer extension to warn about possible efficiency mistakes
    featureEnables[0] = VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT;
    // Massively slows things down ( TODO enable after implementing)
    featureEnables[1] = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT;
    featureEnables[2] = VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_RESERVE_BINDING_SLOT_EXT;

    features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    features.enabledValidationFeatureCount = static_cast<uint32_t>(ARRAY_SIZE(featureEnables));
    features.pEnabledValidationFeatures = featureEnables;

    // Sometimes we need to disable specific Vulkan validation checks for performance or known
    // bugs in the validation layers
    VkBool32 gpuav_descriptor_checks = VK_FALSE;
    VkBool32 gpuav_indirect_draws_buffers = VK_FALSE;
    VkBool32 gpuav_post_proces_descriptor_indexing = VK_FALSE;

    // TODO use lambda instead
#define LAYER_SETTINGS_BOOL32(name, var)                \
    VkLayerSettingEXT {                                 \
      .pLayerName = validationLayers[0],                \
    .pSettingName = name,                               \
            .type = VK_LAYER_SETTING_TYPE_BOOL32_EXT,   \
      .valueCount = 1,                                  \
         .pValues = var }

    layerSettings[0] = LAYER_SETTINGS_BOOL32("gpuav_descriptor_checks", &gpuav_descriptor_checks);
    layerSettings[1] = LAYER_SETTINGS_BOOL32("gpuav_indirect_draws_buffers", &gpuav_indirect_draws_buffers);
    layerSettings[2] =LAYER_SETTINGS_BOOL32("gpuav_post_process_descriptor_indexing",
                                       &gpuav_post_proces_descriptor_indexing);
#undef LAYER_SETTINGS_BOOL32

    //
    layerSettingsCreate.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
    layerSettingsCreate.settingCount = static_cast<u32>(ARRAY_SIZE(layerSettings));
    layerSettingsCreate.pSettings = layerSettings;
    layerSettingsCreate.pNext = &features;

    fcPrintEndl("INFO: Validation Layers added!");
    return (&layerSettingsCreate);
  }

} // --- namespace fc --- (END)
