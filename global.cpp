#include "global.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define VKTEST_DEBUG_BREAK_CHAR_COUNT       (60)

void Assert(VkResult test, std::string message, SDL_Window* win)
{
    if (test != VK_SUCCESS) {
        std::stringstream out;

        out << "[[";
        switch (test) {
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
            out << "* UNKNOWN (" << static_cast<int>(test) << ")";
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

        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error",
          out.str().c_str(), win);
        exit(-1);
    }
}

void Info(std::string message, SDL_Window* win)
{
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
#if defined(VKTEST_DEBUG)
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Information",
      out.str().c_str(), win);
#else
    std::cerr << out.str() << std::endl;
    return;
#endif
}

std::fstream Log::m_file;
Log::Level Log::m_verbosity;

void Log::Close(void)
{
    std::stringstream out;
    out << "Log closed at " << SDL_GetTicks() << std::endl;
    if (m_file.is_open()) {
        m_file << out.str();
        m_file.flush();
        m_file.close();
    } else {
        std::cout << out.str();
    }
}

bool Log::Init(Log::Level verbosity)
{
    m_verbosity = verbosity;
    m_file.open("log.txt", std::fstream::out | std::fstream::binary);
    if (!m_file.is_open()) {
        return false;
    }

    std::stringstream out;
    out << "Log opened at " << SDL_GetTicks() << std::endl;
    m_file << out.str();
    m_file.flush();

    return true;
}

void Log::Write(Log::Level level, std::string message)
{
    std::stringstream out;

    switch (level) {
    case SEVERE:
        out << "SEVERE:  ";
        break;
    case WARNING:
        out << "WARNING: ";
        break;
    case ROUTINE:
        out << "ROUTINE: ";
        break;
    }

    out << "[" << SDL_GetTicks() << "] " << message << std::endl;
    if (m_file.is_open()) {
        m_file << out.str();
        m_file.flush();
    } else {
        std::cout << out.str() << std::endl;
    }
}

std::vector<char> ReadFile(std::string path)
{
    std::fstream f;

    f.open(path.c_str(), std::fstream::in | std::fstream::binary);
    if (!f.is_open()) {
        std::stringstream out;
        out << "Could not open file at: " << path << std::endl;
        Assert(VK_INCOMPLETE, out.str());
    }

    f.seekg(0L, f.end);
    size_t len = f.tellg();
    f.seekg(0L, f.beg);

    std::vector<char> data(len);
    f.read(data.data(), len);
    f.close();

    return data;
}

bool ReadImage(struct STBImage* out, std::string path)
{
    if (out == nullptr || path.empty()) {
        return false;
    }

    out->data = stbi_load(path.c_str(), &out->width, &out->height, &out->comp,
      STBI_rgb_alpha);
    if (out->data == nullptr) {
        return false;
    }

    return true;
}

void ReleaseImage(struct STBImage* out)
{
    stbi_image_free(out->data);
    out->width = 0;
    out->height = 0;
    out->comp = 0;
}
