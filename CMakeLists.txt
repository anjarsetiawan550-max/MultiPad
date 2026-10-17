cmake_minimum_required(VERSION 3.22)

# Runtime statis di Windows supaya tidak perlu install VC++ Redistributable
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "" FORCE)
endif()

project(MultiPadLR VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_binary_data(MultiPadAssets
    SOURCES
        Assets/bg.png
        Assets/pad1.png
        Assets/pad2.png
        Assets/pad3.png
        Assets/pad4.png
        Assets/pad5.png
        Assets/pad6.png)

juce_add_plugin(MultiPadLR
    COMPANY_NAME "LR PRO"
    PLUGIN_MANUFACTURER_CODE LrPr
    PLUGIN_CODE MpDr
    PLUGIN_NAME "MULTI PAD LR PRO"
    PRODUCT_NAME "MULTI PAD LR PRO"
    DESCRIPTION "Multi Pad Sampler - Doraemon"
    BUNDLE_ID com.lrpro.multipad
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Sampler
    ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon.png"
    ICON_SMALL "${CMAKE_CURRENT_SOURCE_DIR}/Assets/icon.png")

target_sources(MultiPadLR PRIVATE
    Source/GlobalKeys.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(MultiPadLR PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_STRICT_REFCOUNTEDPOINTER=1)

target_link_libraries(MultiPadLR
    PRIVATE
        MultiPadAssets
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags)

if(APPLE)
    target_link_libraries(MultiPadLR PRIVATE "-framework ApplicationServices")
endif()
