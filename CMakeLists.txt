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
set(RUBBERBAND_ROOT "C:/Users/danga/vcpkg/installed/x64-windows"
    CACHE PATH "Path to vcpkg's x64-windows install tree containing Rubber Band")

target_include_directories(VocalHarmonizer PRIVATE "${RUBBERBAND_ROOT}/include")
target_link_libraries(VocalHarmonizer PRIVATE "${RUBBERBAND_ROOT}/lib/rubberband.lib")

file(GLOB RUBBERBAND_RUNTIME_DLLS "${RUBBERBAND_ROOT}/bin/*.dll")

add_custom_command(TARGET VocalHarmonizer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${RUBBERBAND_RUNTIME_DLLS}
        "$<TARGET_FILE_DIR:VocalHarmonizer>"
    COMMENT "Copying Rubber Band runtime DLLs next to the executable"
)
