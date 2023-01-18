#include "HRenderer.h"
#include "../logging/HLogger.h"
#include <string>
#include <cassert>
#include <vector>

#ifndef NDEBUG
VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void* user_data)
{
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        std::cout << "Callback Warning: " << callback_data->messageIdNumber << ":" << callback_data->pMessageIdName
            << ":" << callback_data->pMessage << std::endl;
    }
    else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        std::cerr << "Callback Error: " << callback_data->messageIdNumber << ":" << callback_data->pMessageIdName
            << ":" << callback_data->pMessage << std::endl;
    }
    return VK_FALSE;
}
#endif // !NDEBUG



namespace Hedge
{
    // ================================================================================================================
    HRenderManager::HRenderManager()
    {
        // Create vulkan instance and possible debug initialization.
        CreateVulkanAppInstDebugger();

        // Create physical device and logical device
        CreateVulkanPhyLogicalDevice();
    }

    // ================================================================================================================
    HRenderManager::~HRenderManager()
    {


#ifndef NDEBUG
        // Destroy debug messenger
        auto fpVkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInst, "vkDestroyDebugUtilsMessengerEXT");
        if (fpVkDestroyDebugUtilsMessengerEXT == nullptr)
        {
            exit(1);
        }
        fpVkDestroyDebugUtilsMessengerEXT(m_vkInst, m_dbgMsger, nullptr);
#endif

        vkDestroyInstance(m_vkInst, nullptr);
    }

    // ================================================================================================================
    void HRenderManager::CreateVulkanAppInstDebugger()
    {
        // Initialize instance and application
        VkApplicationInfo appInfo{};
        {
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Hedge";
            appInfo.applicationVersion = 1;
            appInfo.pEngineName = "HedgeEngine";
            appInfo.engineVersion = 1;
            appInfo.apiVersion = VK_API_VERSION_1_3;
        }

        std::vector<const char*> extensions;
#ifndef NDEBUG
        ValidateDebugExtAndValidationLayer();
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        const char* validationLayerName = "VK_LAYER_KHRONOS_validation";

        // Create the debug callback messenger info for instance create and destroy check.
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        {
            debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
            debugCreateInfo.pfnUserCallback = debug_utils_messenger_callback;
        }
#endif

        VkInstanceCreateInfo instCreateInfo{};
        {
            instCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
#ifndef NDEBUG
            instCreateInfo.pNext = &debugCreateInfo;
#endif
            instCreateInfo.pApplicationInfo = &appInfo;
            instCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            instCreateInfo.ppEnabledExtensionNames = extensions.data();
            instCreateInfo.enabledLayerCount = 1;
            instCreateInfo.ppEnabledLayerNames = &validationLayerName;
        }
        VK_CHECK(vkCreateInstance(&instCreateInfo, nullptr, &m_vkInst));

#ifndef NDEBUG
        // Create debug messenger
        auto fpVkCreateDebugUtilsMessengerExt = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_vkInst, "vkCreateDebugUtilsMessengerEXT");
        if (fpVkCreateDebugUtilsMessengerExt == nullptr)
        {
            exit(1);
        }
        VK_CHECK(fpVkCreateDebugUtilsMessengerExt(m_vkInst, &debugCreateInfo, nullptr, &m_dbgMsger));
#endif
    }

    // ================================================================================================================
    void HRenderManager::CreateVulkanPhyLogicalDevice()
    {
        // Enumerate the physicalDevices, select the first one and display the name of it.
        uint32_t phyDeviceCount;
        VK_CHECK(vkEnumeratePhysicalDevices(m_vkInst, &phyDeviceCount, nullptr));
        assert(phyDeviceCount >= 1);
        std::vector<VkPhysicalDevice> phyDeviceVec(phyDeviceCount);
        VK_CHECK(vkEnumeratePhysicalDevices(m_vkInst, &phyDeviceCount, phyDeviceVec.data()));
        m_vkPhyDevice = phyDeviceVec[0];
        VkPhysicalDeviceProperties physicalDevProperties;
        vkGetPhysicalDeviceProperties(m_vkPhyDevice, &physicalDevProperties);
        HDG_CORE_INFO("Selected device name: {}", physicalDevProperties.deviceName);

        // Initialize the logical device with the queue family that supports both graphics and present on the physical device
        // Find the queue family indices that supports graphics, compute and present.
        uint32_t queueFamilyPropCount;
        vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhyDevice, &queueFamilyPropCount, nullptr);
        assert(queueFamilyPropCount >= 1);
        std::vector<VkQueueFamilyProperties> queueFamilyProps(queueFamilyPropCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhyDevice, &queueFamilyPropCount, queueFamilyProps.data());

    }

#ifndef NDEBUG

    // ================================================================================================================
    void HRenderManager::ValidateDebugExtAndValidationLayer()
    {
        // Verify that the debug extension for the callback messenger is supported.
        uint32_t propNum;
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &propNum, nullptr));
        assert(propNum >= 1);
        std::vector<VkExtensionProperties> props(propNum);
        VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &propNum, props.data()));
        for (int i = 0; i < props.size(); ++i)
        {
            if (strcmp(props[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0)
            {
                break;
            }
            if (i == propNum - 1)
            {
                std::cout << "Something went very wrong, cannot find " << VK_EXT_DEBUG_UTILS_EXTENSION_NAME << " extension"
                    << std::endl;
                exit(1);
            }
        }

        // Verify that the validation layer for Khronos validation is supported
        uint32_t layerNum;
        VK_CHECK(vkEnumerateInstanceLayerProperties(&layerNum, nullptr));
        assert(layerNum >= 1);
        std::vector<VkLayerProperties> layers(layerNum);
        VK_CHECK(vkEnumerateInstanceLayerProperties(&layerNum, layers.data()));
        for (uint32_t i = 0; i < layerNum; ++i)
        {
            if (strcmp("VK_LAYER_KHRONOS_validation", layers[i].layerName) == 0)
            {
                break;
            }
            if (i == layerNum - 1)
            {
                std::cout << "Something went very wrong, cannot find VK_LAYER_KHRONOS_validation extension" << std::endl;
                exit(1);
            }
        }
    }
#endif
}