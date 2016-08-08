#include "renderer.h"

#define VKTEST_DEBUG_BREAK_CHAR_COUNT       (60)

#ifdef VKTEST_DEBUG
static PFN_vkCreateDebugReportCallbackEXT
  pfnCreateDebugReportCallback = nullptr;
static PFN_vkDestroyDebugReportCallbackEXT
  pfnDestroyDebugReportCallback = nullptr;
static PFN_vkDebugReportMessageEXT
  pfnDebugReportMessage = nullptr;

VKAPI_ATTR VkBool32 VKAPI_CALL VkTestDebugReportCallback(
  VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
  uint64_t object, size_t location, int32_t messageCode,
  const char* pLayerPrefix, const char* pMessage, void* pUserData);
#endif

/* need this function definition, regardless of build type */
VkResult Renderer::init_debug(void)
{
#ifdef VKTEST_DEBUG
    VkResult result = VK_SUCCESS;

    VkInstance temp = this->instance;
    pfnCreateDebugReportCallback = (PFN_vkCreateDebugReportCallbackEXT)
      vkGetInstanceProcAddr(temp, "vkCreateDebugReportCallbackEXT");
    pfnDestroyDebugReportCallback = (PFN_vkDestroyDebugReportCallbackEXT)
      vkGetInstanceProcAddr(temp, "vkDestroyDebugReportCallbackEXT");
    pfnDebugReportMessage = (PFN_vkDebugReportMessageEXT)
      vkGetInstanceProcAddr(temp, "vkDebugReportMessageEXT");

    VkDebugReportCallbackCreateInfoEXT cci = {};
    cci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    cci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT |
                VK_DEBUG_REPORT_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                // these last two are *very* chatty.
                VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    cci.pfnCallback = &VkTestDebugReportCallback;
    cci.pUserData = this;

    result = pfnCreateDebugReportCallback(this->instance, &cci, nullptr,
      &this->callback);

    log.open("log.txt", std::fstream::out);
    if (!log.is_open()) {
        Assert(VK_INCOMPLETE, "Failed to open log file.");
    }

    std::stringstream out;
    out << "Log opened at " << SDL_GetTicks() << "ms." << std::endl;
    log << out.str();

    return result;
#else
    return VK_SUCCESS;
#endif
}

VkResult Renderer::release_debug(void)
{
#ifdef VKTEST_DEBUG
    std::stringstream out;
    out << "Log closed at " << SDL_GetTicks() << "ms." << std::endl;
    this->log << out.str();
    this->log.close();

    pfnDestroyDebugReportCallback(this->instance, this->callback, nullptr);

    return VK_SUCCESS;
#else
    return VK_SUCCESS;
#endif
}


#ifdef VKTEST_DEBUG
std::fstream* Renderer::_get_log_file(void)
{
    return &this->log;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkTestDebugReportCallback(
  VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
  uint64_t object, size_t location, int32_t messageCode,
  const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    Renderer* state = (Renderer*)pUserData;
    std::stringstream out;

    switch (flags) {
    case VK_DEBUG_REPORT_ERROR_BIT_EXT:
        out << "ERROR: ";
        break;
    case VK_DEBUG_REPORT_WARNING_BIT_EXT:
        out << "WARNING: ";
        break;
    case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
        out << "PERFORMANCE: ";
        break;
    case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
        out << "INFORMATION: ";
        break;
    case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
        out << "DEBUG: ";
        break;
    default:
        out << "!! UNKNOWN !!: ";
    }

    out << pMessage;

    /* If it's an error, ensure that it's noticed via MessageBox */
    if (flags == VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        out << " | " << pLayerPrefix << std::endl;
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Vulkan Error",
          out.str().c_str(), nullptr);
    } else {
        out << std::endl;
    }

    /* Output everything to the log file. */
    std::fstream* file = state->_get_log_file();
    if (file->is_open()) {
        *file << out.str().c_str();
        file->flush();
    }

    return VK_FALSE;
}
#endif
