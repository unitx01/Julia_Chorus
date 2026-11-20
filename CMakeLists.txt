cmake_minimum_required(VERSION 3.22)
project(CaesarChorus VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Downloads JUCE automatically the first time you configure
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(CaesarChorus
    COMPANY_NAME             "DIY"
    PLUGIN_MANUFACTURER_CODE Diyp
    PLUGIN_CODE              Csch
    FORMATS                  VST3 AU Standalone   # AU is only built on macOS
    PRODUCT_NAME             "Caesar Chorus"
    IS_SYNTH                 FALSE
    NEEDS_MIDI_INPUT         FALSE
    NEEDS_MIDI_OUTPUT        FALSE
    IS_MIDI_EFFECT           FALSE
    COPY_PLUGIN_AFTER_BUILD  FALSE)

# Find the plugin's source files anywhere in the repository
file(GLOB_RECURSE CAESAR_CPP "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE CAESAR_H   "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
list(FILTER CAESAR_CPP EXCLUDE REGEX "/(build|_deps)/")
list(FILTER CAESAR_H   EXCLUDE REGEX "/(build|_deps)/")

message(STATUS "Source files found: ${CAESAR_CPP}")
message(STATUS "Header files found: ${CAESAR_H}")

if(NOT CAESAR_CPP)
    message(FATAL_ERROR "No .cpp files found in the repository - upload the Source files.")
endif()

target_sources(CaesarChorus PRIVATE ${CAESAR_CPP})

foreach(header ${CAESAR_H})
    get_filename_component(header_dir "${header}" DIRECTORY)
    target_include_directories(CaesarChorus PRIVATE "${header_dir}")
endforeach()

target_compile_definitions(CaesarChorus PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(CaesarChorus
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
