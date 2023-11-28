cmake_minimum_required(VERSION 3.23.2)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG")

project(VulkanEngine)

# add sources
file(GLOB_RECURSE SOURCES 
    ${PROJECT_SOURCE_DIR}/src/*.cpp
    ${PROJECT_SOURCE_DIR}/src/**/*.cpp
    ${PROJECT_SOURCE_DIR}/include/*.h
    ${PROJECT_SOURCE_DIR}/include/**/*.h
    ${PROJECT_SOURCE_DIR}/include/*.hpp
    ${PROJECT_SOURCE_DIR}/include/**/*.hpp)
add_executable(${PROJECT_NAME}
    ${SOURCES})

install(TARGETS ${PROJECT_NAME})

# DOWNLOAD ALL SUBMODULES
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")

    message(STATUS "Submodule update")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        RESULT_VARIABLE GIT_SUBMOD_RESULT
    )
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}.")
    endif()

endif()

# CHECK ALL SUBMODULES
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/glfw/CMakeLists.txt")
    message(FATAL_ERROR "GLFW not downloaded.")
elseif(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/glm/CMakeLists.txt")
    message(FATAL_ERROR "GLM not downloaded.")
elseif(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/stb/CMakeLists.txt")
    message(FATAL_ERROR "STB not downloaded.")
elseif(NOT EXISTS "${PROJECT_SOURCE_DIR}/external/vulkan/CMakeLists.txt")
    message(FATAL_ERROR "VULKAN not downloaded.")
endif()

# Find packages
find_package(Vulkan REQUIRED)

# include subdirectories
add_subdirectory(external/vulkan)
add_subdirectory(external/vulkan/glfw)
add_subdirectory(external/vulkan/glm)
add_subdirectory(external/stb)
add_subdirectory(external/tinyobjloader)

# include directories
target_include_directories(${PROJECT_NAME}
    PUBLIC
    include
    include/common
    include/containers
    external/vulkan/glfw/include
    external/vulkan/glm
    external/vulkan/vulkan
    external/stb
    external/tinyobjloader
)

# link
target_link_directories(${PROJECT_NAME}
    PRIVATE
    src
    external/vulkan/glfw/src
    external/vulkan/glm
    external/stb
    external/tinyobjloader
)

target_link_libraries(${PROJECT_NAME}
    glfw
    glm
    # stb
    tinyobjloader
    Vulkan::Vulkan
)

# file(GLOB_RECURSE sources ${PROJECT_SOURCE_DIR}/**/*.c)