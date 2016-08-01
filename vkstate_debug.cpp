#include "vkstate.h"

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

void VkState::_assert(VkResult condition, std::string message)
{
    if (condition != VK_SUCCESS) {
        std::stringstream out;

        out << "[[";
        switch (condition) {
        case VK_NOT_READY:
            out << "VK_NOT_READY";
            break;
        case VK_TIMEOUT:
            out << "VK_TIMEOUT";
            break;
        case VK_EVENT_SET:
            out << "VK_EVENT_SET";
            break;
        case VK_EVENT_RESET:
            out << "VK_EVENT_RESET";
            break;
        case VK_INCOMPLETE:
            out << "VK_INCOMPLETE";
            break;
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            out << "VK_ERROR_OUT_OF_HOST_MEMORY";
            break;
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            out << "VK_ERROR_OUT_OF_DEVICE_MEMORY";
            break;
        case VK_ERROR_INITIALIZATION_FAILED:
            out << "VK_ERROR_INITIALIZATION_FAILED";
            break;
        case VK_ERROR_DEVICE_LOST:
            out << "VK_ERROR_DEVICE_LOST";
            break;
        case VK_ERROR_MEMORY_MAP_FAILED:
            out << "VK_ERROR_MEMORY_MAP_FAILED";
            break;
        case VK_ERROR_LAYER_NOT_PRESENT:
            out << "VK_ERROR_LAYER_NOT_PRESENT";
            break;
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            out << "VK_ERROR_EXTENSION_NOT_PRESENT";
            break;
        case VK_ERROR_FEATURE_NOT_PRESENT:
            out << "VK_ERROR_FEATURE_NOT_PRESENT";
            break;
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            out << "VK_ERROR_INCOMPATIBLE_DRIVER";
            break;
        case VK_ERROR_TOO_MANY_OBJECTS:
            out << "VK_ERROR_TOO_MANY_OBJECTS";
            break;
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            out << "VK_ERROR_FORMAT_NOT_SUPPORTED";
            break;
        case VK_ERROR_SURFACE_LOST_KHR:
            out << "VK_ERROR_SURFACE_LOST_KHR";
            break;
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            out << "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
            break;
        case VK_SUBOPTIMAL_KHR:
            out << "VK_ERROR_SUBOPTIMAL_KHR";
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            out << "VK_ERROR_OUT_OF_DATE_KHR";
            break;
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            out << "VK_ERROR_OUT_OF_DATE_KHR";
            break;
        default:
            out << "* UNKNOWN (" << static_cast<int>(condition) << ")";
        }
        out << "]]\n\n";

        /* 
         * This loop attempts to wrap the message if the character count is
         * greater than 'x' characters wide.  However, if you're in the middle
         * of a word, it'll wait until the next space is found. Seems to work.
         *
         * UTF-8 Technical debt?
         */
        int count = 0;
        for (size_t i = 0; i < message.length(); i++) {
            count++;
            if (count >= VKTEST_DEBUG_BREAK_CHAR_COUNT && message[i] == ' ') {
                out << "\n";
                count = 0;
                continue;
            }

            out << message[i];
        }

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Vulkan Library Error",
          out.str().c_str(), this->window);
        exit(-1);
    }
}

void VkState::_info(std::string message)
{
#ifdef VKTEST_DEBUG
    std::stringstream out;
    int count = 0;
    for (size_t i = 0; i < message.length(); i++) {
        count++;
        if (count >= VKTEST_DEBUG_BREAK_CHAR_COUNT && message[i] == ' ') {
            out << "\n";
            count = 0;
            continue;
        }

        out << message[i];
    }
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "VkTest Information",
      out.str().c_str(), this->window);
#else
    return;
#endif
}

VkResult VkState::init_debug(void)
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
        this->_assert(VK_INCOMPLETE, "Failed to open log file.");
    }

    std::stringstream out;
    out << "Log opened at " << SDL_GetTicks() << "ms." << std::endl;
    log << out.str();

    return result;
#else
    return VK_SUCCESS;
#endif
}

VkResult VkState::release_debug(void)
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
std::fstream* VkState::_get_log_file(void)
{
    return &this->log;
}

VKAPI_ATTR VkBool32 VKAPI_CALL VkTestDebugReportCallback(
  VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
  uint64_t object, size_t location, int32_t messageCode,
  const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
    VkState* state = (VkState*)pUserData;
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

