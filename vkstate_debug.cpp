#include "vkstate.h"

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
        /* VK_ERROR_FRAGMENTED_POOL was added sometime after my version of
         * the Vulkan loader was built.  In the 1.0.10 spec, it doesn't exist.
        case VK_ERROR_FRAGMENTED_POOL:
            out << "VK_ERROR_FRAGMENTED_POOL";
            break;
         */
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
            out << "**UNKNOWN**";
        }
        out << "]]\n\n";

        /* 
         * This loop attempts to wrap the message if the character count is
         * greater than 80 characters wide.  However, if you're in the middle
         * of a word, it'll wait until the next space is found. Seems to work.
         *
         * UTF-8 Technical debt.
         */
        int count = 0;
        for (size_t i = 0; i < message.length(); i++) {
            count++;
            if (count >= 80 && message[i] == ' ') {
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

#ifdef VKTEST_DEBUG
void VkState::_info(std::string message)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Message",
      message.c_str(), this->window);
}
#else
void VkState::_info(std::string message) { return; }
#endif

/*
void VkState::_debug_callback(void)
{

}
*/

