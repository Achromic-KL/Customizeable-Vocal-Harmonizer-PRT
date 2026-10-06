cmake_minimum_required(VERSION 3.22)
project(VocalHarmonizer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(VocalHarmonizer
    PRODUCT_NAME "Vocal Harmonizer"
)

target_sources(VocalHarmonizer PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
)

target_compile_definitions(VocalHarmonizer PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:VocalHarmonizer,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:VocalHarmonizer,JUCE_VERSION>"
)

target_link_libraries(VocalHarmonizer PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# --- Rubber Band (installed via vcpkg: vcpkg install rubberband:x64-windows) ---
<<<<<<< HEAD
# Rubber Band only exposes a pkg-config module, not a CMake config package,
# so we link directly against vcpkg's install tree rather than using
# find_package(). That install tree's location isn't something we can know
# in advance, so resolve it in order of preference:
#
#   1. An explicit -DRUBBERBAND_ROOT=... passed at configure time (highest
#      priority - always wins if given).
#   2. The VCPKG_ROOT environment variable, if set (the standard convention
#      vcpkg itself recommends - see https://learn.microsoft.com/vcpkg).
#   3. Otherwise, fail with a clear message instead of a confusing linker
#      error further down the build.
if(NOT RUBBERBAND_ROOT)
    if(DEFINED ENV{VCPKG_ROOT})
        set(RUBBERBAND_ROOT "$ENV{VCPKG_ROOT}/installed/x64-windows")
    endif()
endif()

set(RUBBERBAND_ROOT "${RUBBERBAND_ROOT}" CACHE PATH
    "Path to vcpkg's x64-windows install tree containing Rubber Band")

if(NOT EXISTS "${RUBBERBAND_ROOT}/include/rubberband/RubberBandStretcher.h")
    message(FATAL_ERROR
        "Could not find Rubber Band under RUBBERBAND_ROOT=\"${RUBBERBAND_ROOT}\".\n"
        "\n"
        "Fix this by either:\n"
        "  1. Setting a VCPKG_ROOT environment variable pointing at your vcpkg\n"
        "     folder (e.g. setx VCPKG_ROOT \"C:\\path\\to\\vcpkg\"), then opening a\n"
        "     new terminal and re-running cmake, or\n"
        "  2. Passing -DRUBBERBAND_ROOT=<path>/installed/x64-windows explicitly\n"
        "     when running cmake -B build ...\n"
        "\n"
        "Either way, make sure Rubber Band itself is installed first:\n"
        "  vcpkg install rubberband:x64-windows")
endif()
=======
set(RUBBERBAND_ROOT "C:/Users/danga/vcpkg/installed/x64-windows"
    CACHE PATH "Path to vcpkg's x64-windows install tree containing Rubber Band")
>>>>>>> f2a238db83cd08dca817459a72eda61a800a941e

target_include_directories(VocalHarmonizer PRIVATE "${RUBBERBAND_ROOT}/include")
target_link_libraries(VocalHarmonizer PRIVATE "${RUBBERBAND_ROOT}/lib/rubberband.lib")

file(GLOB RUBBERBAND_RUNTIME_DLLS "${RUBBERBAND_ROOT}/bin/*.dll")

add_custom_command(TARGET VocalHarmonizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RUBBERBAND_RUNTIME_DLLS}
        "$<TARGET_FILE_DIR:VocalHarmonizer>"
    COMMENT "Copying Rubber Band runtime DLLs next to the executable"
)
