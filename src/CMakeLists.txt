#TODO: toolchain independant way to do this
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("Turning on debug flags...")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVKTEST_DEBUG")
endif()

include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${Vulkan_INCLUDE_DIRS})
include_directories(${XCB_INCLUDE_DIRS})
include_directories(${X11XCB_INCLUDE_DIRS})

link_directories(${SDL2_LIBRARY_DIRS})
link_directories(${X11XCB_LIBRARY_DIRS})
link_directories(${XCB_LIBRARY_DIRS})
link_directories(${Vulkan_LIBRARY_DIRS})

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

target_link_libraries(vktest
    ${SDL2_LIBRARIES}
    ${X11XCB_LIBRARIES}
    ${XCB_LIBRARIES}
    ${Vulkan_LIBRARIES})

file(COPY textures DESTINATION .)
file(COPY shaders DESTINATION .)
