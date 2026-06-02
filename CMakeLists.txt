cmake_minimum_required(VERSION 3.22)
project(SpectraFlow VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─── FetchContent ────────────────────────────────────────────────────────────
include(FetchContent)

# JUCE 7.0.12
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
)

# KFR DSP
FetchContent_Declare(
    kfr
    GIT_REPOSITORY https://github.com/kfrlib/kfr.git
    GIT_TAG        5.1.0
    GIT_SHALLOW    TRUE
)

# chowdsp_utils
FetchContent_Declare(
    chowdsp_utils
    GIT_REPOSITORY https://github.com/Chowdhury-DSP/chowdsp_utils.git
    GIT_TAG        v2.3.0
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(JUCE kfr chowdsp_utils)

# ─── KFR options (CPU-only, no GPU) ─────────────────────────────────────────
set(KFR_ENABLE_DFT ON)
set(KFR_ENABLE_CAPI OFF)
set(KFR_ARCH auto)   # auto-selects best SIMD at compile time

# ─── Plugin target ───────────────────────────────────────────────────────────
juce_add_plugin(SpectraFlow
    COMPANY_NAME            "SpectraFlow Audio"
    PLUGIN_MANUFACTURER_CODE SFAu
    PLUGIN_CODE             SFv1
    FORMATS                 VST3
    PRODUCT_NAME            "SpectraFlow"
    IS_SYNTH                FALSE
    NEEDS_MIDI_INPUT        FALSE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES         "Analyzer" "Fx"
    PLUGIN_MANUFACTURER_CODE SFAu
    VST3_AUTO_MANIFEST      FALSE
    # Allow up to 12 sidechain buses in VST3
    NEEDS_CURL              FALSE
    NEEDS_WEB_BROWSER       FALSE
)

# ─── Sources ─────────────────────────────────────────────────────────────────
target_sources(SpectraFlow PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/DSP/FFTAnalyzer.cpp
    Source/DSP/WaveformAnalyzer.cpp
    Source/DSP/BusManager.cpp
    Source/DSP/FIFOManager.cpp
    Source/GUI/SpectrumRenderer.cpp
    Source/GUI/WaveformRenderer.cpp
    Source/GUI/ColorManager.cpp
    Source/GUI/UIController.cpp
    Source/Utils/ParameterManager.cpp
    Source/Utils/LockFreeQueue.cpp
)

# ─── Include dirs ────────────────────────────────────────────────────────────
target_include_directories(SpectraFlow PRIVATE
    Source
    Source/DSP
    Source/GUI
    Source/Utils
    ${kfr_SOURCE_DIR}/include
)

# ─── JUCE modules ────────────────────────────────────────────────────────────
target_link_libraries(SpectraFlow PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_audio_devices
    juce::juce_dsp
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_core
    chowdsp::chowdsp_dsp_utils
    chowdsp::chowdsp_parameters
    chowdsp::chowdsp_data_structures
    kfr_dft
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ─── Definitions ─────────────────────────────────────────────────────────────
target_compile_definitions(SpectraFlow PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
    KFR_NO_C_COMPLEX_TYPES=1
)

# ─── MSVC-specific flags ──────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(SpectraFlow PRIVATE /W3 /MP /arch:AVX2)
endif()

# ─── GCC/Clang flags ─────────────────────────────────────────────────────────
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(SpectraFlow PRIVATE
        -O3 -march=native -ffast-math
        -Wall -Wextra -Wno-unused-parameter
    )
endif()