# CMakeList.txt : CMake project for RythmGameVideoEditor, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("RythmGameVideoEditor")


# Include directories for headers
include_directories(${CMAKE_SOURCE_DIR}/src 
                    ${CMAKE_SOURCE_DIR}/src/core
                    ${CMAKE_SOURCE_DIR}/src/window)

# Add the source files
set(SOURCES
    src/VideoDecoder.cpp
    src/VideoRenderer.cpp
    src/VideoPlayer.cpp
    src/main.cpp
)

# Add source to this project's executable.
add_executable (RythmGameVideoEditor  "src/main.cpp" 
    "src/Application.h" "src/Application.cpp" 
    "src/window/Window.h" "src/window/Window.cpp" 
    "src/window/WindowHSplit.h" "src/window/WindowHSplit.cpp" 
    "src/window/WindowVSplit.h" "src/window/WindowVSplit.cpp" 
    "src/window/AssetsListWindow.h" "src/window/AssetsListWindow.cpp" 
    "src/window/VideoPlayerWindow.h" "src/window/VideoPlayerWindow.cpp" 
    "src/window/TimelineWindow.h" "src/window/TimelineWindow.cpp" 
    "src/util.h" "src/util.cpp" 
    "src/core/VideoData.h" "src/window/WindowIncludes.h" "src/window/ContextMenu.h" "src/core/Timeline.h" "src/core/Timeline.cpp")

# Set a moderate warning level
target_compile_options(RythmGameVideoEditor PRIVATE /W3)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET RythmGameVideoEditor PROPERTY CXX_STANDARD 20)
endif()

set(SDL2_ROOT "${CMAKE_SOURCE_DIR}/libs/SDL2")
set(SDL2_INCLUDE_DIRS "${SDL2_ROOT}/include")
set(SDL2_LIBRARIES "${SDL2_ROOT}/lib/x64/")

# Locate FFmpeg
set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/libs/ffmpeg")
set(FFMPEG_INCLUDE_DIRS "${FFMPEG_ROOT}/include")
set(FFMPEG_LIBRARIES "${FFMPEG_ROOT}/lib")

# Include directories (header files)
include_directories(${SDL2_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS})

# Link FFmpeg libraries (lib files)
target_link_libraries(RythmGameVideoEditor
    ${SDL2_LIBRARIES}/SDL2.lib
    ${SDL2_LIBRARIES}/SDL2_ttf.lib
    ${SDL2_LIBRARIES}/SDL2main.lib
    ${FFMPEG_LIBRARIES}/avcodec.lib
    ${FFMPEG_LIBRARIES}/avformat.lib
    ${FFMPEG_LIBRARIES}/avutil.lib
    ${FFMPEG_LIBRARIES}/swresample.lib
    ${FFMPEG_LIBRARIES}/swscale.lib
)

# Set bin dirs
set(FFMPEG_BIN_DIR "${FFMPEG_ROOT}/bin")

# Copy library DLLs to the output directory
add_custom_command(TARGET RythmGameVideoEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${SDL2_LIBRARIES}/SDL2.dll"
    "${SDL2_LIBRARIES}/SDL2_ttf.dll"
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

# Set asset dirs
set(ASSETS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/assets")
set(ASSETS_DEST_DIR "$<TARGET_FILE_DIR:RythmGameVideoEditor>/assets")

# Copy assets to the output directory
add_custom_command(
    TARGET RythmGameVideoEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${ASSETS_DEST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSETS_SOURCE_DIR}" "${ASSETS_DEST_DIR}"
    COMMENT "Copying updated or new assets to build directory"
)
