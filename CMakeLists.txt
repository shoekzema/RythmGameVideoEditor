# CMakeList.txt : CMake project for RythmGameVideoEditor, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

project(RythmGameVideoEditor LANGUAGES C CXX)

# Prefer a consistent C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Source files
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/util.cpp

    src/core/AssetsList.cpp
    src/core/Timeline.cpp

    src/window/Window.cpp
    src/window/WindowHSplit.cpp
    src/window/WindowVSplit.cpp
    src/window/AssetsListWindow.cpp
    src/window/VideoPlayerWindow.cpp
    src/window/TimelineWindow.cpp
    src/window/TimelineRenderer.cpp
    src/window/TimelineController.cpp
)

# Add the executable
add_executable(RythmGameVideoEditor ${SOURCES})

# Target include directories
target_include_directories(RythmGameVideoEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/window
)

# Set MSVC-only compile options and debug format after the target exists
if(MSVC)
  target_compile_options(RythmGameVideoEditor PRIVATE /W3)
  # Enable Edit-and-Continue for suitable MSVC configurations
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "EditAndContinue")
endif()

# Set target C++ standard in a target-centric way
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RythmGameVideoEditor PROPERTY CXX_STANDARD 20)
endif()

# External library layout (local vendored libraries)
set(SDL2_ROOT "${CMAKE_SOURCE_DIR}/libs/SDL2")
set(SDL2_INCLUDE_DIRS "${SDL2_ROOT}/include")
set(SDL2_LIB_DIR "${SDL2_ROOT}/lib/x64")

set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/libs/ffmpeg")
set(FFMPEG_INCLUDE_DIRS "${FFMPEG_ROOT}/include")
set(FFMPEG_LIB_DIR "${FFMPEG_ROOT}/lib")
set(FFMPEG_BIN_DIR "${FFMPEG_ROOT}/bin")

# Add vendor include dirs to target
target_include_directories(RythmGameVideoEditor PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)

# Link libraries (using full paths to .lib files)
# These assume the lib files exist at the expected locations; TODO: consider switching to find_package/find_library later.
target_link_libraries(RythmGameVideoEditor PRIVATE
    "${SDL2_LIB_DIR}/SDL2.lib"
    "${SDL2_LIB_DIR}/SDL2_ttf.lib"
    "${SDL2_LIB_DIR}/SDL2main.lib"
    "${FFMPEG_LIB_DIR}/avcodec.lib"
    "${FFMPEG_LIB_DIR}/avformat.lib"
    "${FFMPEG_LIB_DIR}/avutil.lib"
    "${FFMPEG_LIB_DIR}/swresample.lib"
    "${FFMPEG_LIB_DIR}/swscale.lib"
)

# Post-build: copy required DLLs to the executable directory
add_custom_command(TARGET RythmGameVideoEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL2_LIB_DIR}/SDL2.dll"
    "${SDL2_LIB_DIR}/SDL2_ttf.dll"
    "${FFMPEG_BIN_DIR}/avformat-60.dll"
    "${FFMPEG_BIN_DIR}/avcodec-60.dll"
    "${FFMPEG_BIN_DIR}/avutil-58.dll"
    "${FFMPEG_BIN_DIR}/swscale-7.dll"
    "${FFMPEG_BIN_DIR}/swresample-4.dll"
    "${FFMPEG_BIN_DIR}/avfilter-9.dll"
    "${FFMPEG_BIN_DIR}/avdevice-60.dll"
    $<TARGET_FILE_DIR:RythmGameVideoEditor>
    COMMENT "Copying updated or new libraries to build directory"
)

# Copy assets to the output directory
set(ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")
set(ASSETS_DEST_DIR "$<TARGET_FILE_DIR:RythmGameVideoEditor>/assets")

add_custom_command(TARGET RythmGameVideoEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSETS_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_SOURCE_DIR}" "${ASSETS_DEST_DIR}"
    COMMENT "Copying updated or new assets to build directory"
)
