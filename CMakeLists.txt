cmake_minimum_required(VERSION 3.22)
project(ZojoRig VERSION 1.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static MSVC runtime so the .exe runs without installing extra Microsoft redistributables
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(ZOJO_BUILD_TESTS "Build the offline test runner" OFF)
set(ZOJO_JUCE_PATH "" CACHE PATH "Use a local JUCE checkout instead of downloading it")

if(ZOJO_JUCE_PATH)
    add_subdirectory(${ZOJO_JUCE_PATH} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.15
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_binary_data(ZojoAssets
    SOURCES
        Resources/fonts/SpaceMono-Regular.ttf
        Resources/fonts/SpaceMono-Bold.ttf
        Resources/fonts/SpaceGrotesk.ttf)

juce_add_plugin(ZojoRig
    COMPANY_NAME "Zojo"
    PRODUCT_NAME "ZOJO RIG"
    BUNDLE_ID "com.zojo.zojorig"
    PLUGIN_MANUFACTURER_CODE Zojo
    PLUGIN_CODE Zrg1
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx Distortion
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "ZOJO RIG needs audio input to hear your guitar.")

set(ZOJO_SOURCES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Presets.cpp
    Source/UI/ZojoLookAndFeel.cpp
    Source/UI/Widgets.cpp)

target_sources(ZojoRig PRIVATE ${ZOJO_SOURCES})
target_include_directories(ZojoRig PRIVATE Source)

set(ZOJO_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_ASIO=1
    JUCE_WASAPI=1
    JUCE_DIRECTSOUND=1)
if(UNIX AND NOT APPLE)
    # Linux is only used for automated testing here; no ALSA/JACK headers needed
    list(APPEND ZOJO_DEFS JUCE_ALSA=0 JUCE_JACK=0)
endif()
target_compile_definitions(ZojoRig PUBLIC ${ZOJO_DEFS})

target_link_libraries(ZojoRig
    PRIVATE
        ZojoAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(ZOJO_BUILD_TESTS)
    juce_add_console_app(ZojoTests PRODUCT_NAME "ZojoTests")
    target_sources(ZojoTests PRIVATE Tests/TestMain.cpp ${ZOJO_SOURCES})
    target_include_directories(ZojoTests PRIVATE Source)
    target_compile_definitions(ZojoTests PRIVATE ${ZOJO_DEFS} JUCE_STANDALONE_APPLICATION=1 ZOJO_TESTS=1)
    target_link_libraries(ZojoTests PRIVATE ZojoAssets juce::juce_audio_utils juce::juce_dsp
        juce::juce_recommended_config_flags)
endif()
