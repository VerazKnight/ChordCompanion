cmake_minimum_required(VERSION 3.28)

set(VCPKG_ROOT "${CMAKE_SOURCE_DIR}/src/ext/vcpkg")

if(WIN32)
  set(VCPKG_EXE "${VCPKG_ROOT}/vcpkg.exe")
  set(BOOTSTRAP_SCRIPT "${VCPKG_ROOT}/bootstrap-vcpkg.bat")
else()
  set(VCPKG_EXE "${VCPKG_ROOT}/vcpkg")
  set(BOOTSTRAP_SCRIPT "${VCPKG_ROOT}/bootstrap-vcpkg.sh")
endif()

if(NOT EXISTS "${VCPKG_EXE}")
  message(STATUS "vcpkg not bootstrapped, running bootstrap script...")
  execute_process(
    COMMAND ${BOOTSTRAP_SCRIPT}
    WORKING_DIRECTORY "${VCPKG_ROOT}"
    RESULT_VARIABLE BOOTSTRAP_RESULT
  )
  if(NOT BOOTSTRAP_RESULT EQUAL 0)
    message(FATAL_ERROR "vcpkg bootstrap failed")
  endif()
endif()

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/src/ext/vcpkg/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

project(ChordCompanion LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW lib only")
set(GLFW_INSTALL OFF CACHE BOOL "GLFW lib only")

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

find_package(FluidSynth CONFIG REQUIRED)

set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/ext/imgui")

add_library(app_core
    src/mainApp.cpp
    src/ui.cpp
    src/pianoKeyboard.cpp
    src/pianoSound.cpp
    src/chordLogic.cpp
)

set(SOUNDFONT_FILE
    "${CMAKE_CURRENT_SOURCE_DIR}/resources/GeneralUser-GS.sf2"
)
set(SOUNDFONT_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedSoundFont.cmake"
)
set(GENERATED_INCLUDE_DIR
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
set(SOUNDFONT_HEADER
    "${GENERATED_INCLUDE_DIR}/embedded_sf2.h"
)

add_custom_command(
    OUTPUT "${SOUNDFONT_HEADER}"
    COMMAND "${CMAKE_COMMAND}"
        "-DINPUT_FILE=${SOUNDFONT_FILE}"
        "-DOUTPUT_FILE=${SOUNDFONT_HEADER}"
        -P "${SOUNDFONT_SCRIPT}"
    DEPENDS
        "${SOUNDFONT_FILE}"
        "${SOUNDFONT_SCRIPT}"
    COMMENT "Embedding GeneralUser-GS.sf2"
    VERBATIM
)

target_sources(app_core PRIVATE "${SOUNDFONT_HEADER}")

target_include_directories(app_core PRIVATE
    "${GENERATED_INCLUDE_DIR}"
    "${IMGUI_DIR}"
    "${IMGUI_DIR}/backends"
)

target_include_directories(app_core PUBLIC src)

target_link_libraries(app_core PUBLIC 
    glfw
    OpenGL::GL
    rtmidi
    FluidSynth::libfluidsynth
)

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
    app_core
)

include(CTest)

if(BUILD_TESTING)
    if(MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()

    add_subdirectory(src/ext/googletest EXCLUDE_FROM_ALL)
    add_subdirectory(tests)
endif()

if(MSVC)
    target_compile_options(ChordCompanion PRIVATE /W4)
else()
    target_compile_options(ChordCompanion PRIVATE
        -Wall
        -Wextra
        -Wformat
    )
endif()
