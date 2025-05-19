#include "Vulkan/Vulkan.hpp"

void GfxCreateContext()
{
    GfxContext *ctx = Alloc<GfxContext>(heap);
}

void GfxDestroyContext()
{
}


VkInstance g_vk_instance;
VkDebugUtilsMessengerEXT g_vk_debug_messenger;
VkDevice g_vk_device;

static int GetPhysicalDeviceScore(VkPhysicalDevice device)
{
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(device, &props);

    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceFeatures(device, &features);

    int score = 1;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        score += 1000;
    if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
        score += 10;

    // For systems with multiple discrete GPU, we could look at individual features to account for the score
    // @Todo: maybe look at available queues

    const char *types[] = {
        "other",
        "integrated GPU",
        "discrete GPU",
        "virtual GPU",
        "CPU",
    };

    LogMessage(Log_Vulkan, "Device: %s '%s', score: %d", types[props.deviceType], props.deviceName, score);

    return score;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data
)
{
    if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        LogError(Log_Vulkan, "%s", callback_data->pMessage);
    else if (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
        LogWarning(Log_Vulkan, "%s", callback_data->pMessage);
    else
        LogMessage(Log_Vulkan, "%s", callback_data->pMessage);

    return VK_FALSE;
}

void InitVulkan()
{
    VkResult result{};

    // Create instance
    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vox";
    app_info.applicationVersion = VK_MAKE_VERSION(0,0,1);
    app_info.pEngineName = "VoxEngine";
    app_info.engineVersion = VK_MAKE_VERSION(0,0,1);
    app_info.apiVersion = VK_API_VERSION_1_0;

    uint num_extensions;
    SDL_Vulkan_GetInstanceExtensions(g_window, &num_extensions, null);

    const char **extension_names = Alloc<const char *>(num_extensions + 1, heap);
    SDL_Vulkan_GetInstanceExtensions(g_window, &num_extensions, extension_names);

    num_extensions += 1;
    extension_names[num_extensions - 1] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    const char *validation_layers[] = {
        // "VK_LAYER_KHRONOS_validation",
    };

    // Make sure validation layers are available
    uint num_available_layers;
    vkEnumerateInstanceLayerProperties(&num_available_layers, null);

    VkLayerProperties *available_layers = Alloc<VkLayerProperties>(num_available_layers, heap);
    vkEnumerateInstanceLayerProperties(&num_available_layers, available_layers);

    for (int i = 0; i < (int)StaticArraySize(validation_layers); i += 1)
    {
        bool found_layer = false;
        for (int j = 0; j < (int)num_available_layers; j += 1)
        {
            if (strcmp(validation_layers[i], available_layers[j].layerName) == 0)
            {
                found_layer = true;
                break;
            }
        }

        Assert(found_layer, "Could not find validation layer '%s'", validation_layers[i]);
    }

    VkInstanceCreateInfo inst_info{};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledLayerCount = StaticArraySize(validation_layers);
    inst_info.ppEnabledLayerNames = validation_layers;
    inst_info.enabledExtensionCount = num_extensions;
    inst_info.ppEnabledExtensionNames = extension_names;

    VkDebugUtilsMessengerCreateInfoEXT debug_info{};
    debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_info.pfnUserCallback = VulkanDebugCallback;

    inst_info.pNext = &debug_info;

    for (uint i = 0; i < num_extensions; i += 1)
        LogMessage(Log_Vulkan, "Extension '%s'", extension_names[i]);

    result = vkCreateInstance(&inst_info, null, &g_vk_instance);
    Assert(result == VK_SUCCESS, "vkCreateInstance failed");

    // Setup debug messenger
    auto vkCreateDebugUtilsMessengerEXT = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vk_instance, "vkCreateDebugUtilsMessengerEXT");
    if (vkCreateDebugUtilsMessengerEXT)
    {
        result = vkCreateDebugUtilsMessengerEXT(g_vk_instance, &debug_info, null, &g_vk_debug_messenger);
        Assert(result == VK_SUCCESS, "vkCreateDebugUtilsMessengerEXT failed");
    }

    // Choose physical device
    uint num_physical_devices;
    vkEnumeratePhysicalDevices(g_vk_instance, &num_physical_devices, null);
    Assert(num_physical_devices > 0, "No GPU");

    VkPhysicalDevice *physical_devices = Alloc<VkPhysicalDevice>(num_physical_devices, heap);
    vkEnumeratePhysicalDevices(g_vk_instance, &num_physical_devices, physical_devices);

    int best_score = -1;
    VkPhysicalDevice best_device = VK_NULL_HANDLE;
    for (uint i = 0; i < num_physical_devices; i += 1)
    {
        int score = GetPhysicalDeviceScore(physical_devices[i]);
        if (score > 0 && score > best_score)
        {
            best_device = physical_devices[i];
            best_score = score;
        }
    }

    Assert(best_score > 0, "Could not find a suitable graphics device");

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(best_device, &props);
    LogMessage(Log_Vulkan, "Using device '%s'", props.deviceName);

    // Create logical device and queues

}

void TerminateVulkan()
{
    auto vkDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(g_vk_instance, "vkDestroyDebugUtilsMessengerEXT");
    if (vkDestroyDebugUtilsMessengerEXT && g_vk_debug_messenger != VK_NULL_HANDLE)
        vkDestroyDebugUtilsMessengerEXT(g_vk_instance, g_vk_debug_messenger, null);

    vkDestroyInstance(g_vk_instance, null);
}


