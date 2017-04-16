set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${Vulkan_INCLUDE_DIRS})

add_executable(vktest
    debug.cpp
    global.cpp
    main.cpp
    renderer.cpp
    renderer_init.cpp
    renderer_release.cpp
    swapchain.cpp
    timer.cpp
    utility.cpp
)

link_directories(${SDL2_LIBRARY_DIRS})
link_directories(${Vulkan_LIBRARY_DIRS})

target_link_libraries(vktest ${SDL2_LIBRARIES} ${Vulkan_LIBRARIES})
