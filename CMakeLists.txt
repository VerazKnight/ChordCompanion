cmake_minimum_required(VERSION 3.28)

project(ChordCompanion LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set( GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set( GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(ALSA REQUIRED)

    set(RTMIDI_API_ALSA ON CACHE BOOL "Enable ALSA MIDI backend" FORCE)
    set(RTMIDI_API_JACK OFF CACHE BOOL "Disable JACK MIDI backend" FORCE)
elseif(WIN32)
    set(RTMIDI_API_WINMM ON CACHE BOOL "Enable Windows MIDI backend" FORCE)
elseif(APPLE)
    set(RTMIDI_API_CORE ON CACHE BOOL "Enable CoreMIDI backend" FORCE)
endif()

add_subdirectory(src/ext/glfw)

add_subdirectory(src/ext/rtmidi)

find_package(OpenGL REQUIRED)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ext/imgui")

add_executable(ChordCompanion 
    src/main.cpp
    
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"

    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

target_include_directories(ChordCompanion PRIVATE
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

target_link_libraries(ChordCompanion PRIVATE
    glfw
    OpenGL::GL
    rtmidi
)

if(MSVC)
    target_compile_options(ChordCompanion PRIVATE /W4)
else()
    target_compile_options(ChordCompanion PRIVATE
        -Wall
        -Wextra
        -Wformat
    )
endif()