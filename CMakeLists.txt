cmake_minimum_required(VERSION 3.22)
project(SkellAfar VERSION 0.4.0 LANGUAGES C CXX)

include(FetchContent)
set(SKELLCORE_REV "e701a6a1e05d923f61c17275609c95122d456223" CACHE STRING "Immutable SkellCore template revision")
option(SKELL_AFAR_BUILD_HEADLESS_TESTS "Build physics and scene metadata tests" ON)
option(SKELL_AFAR_HEADLESS_ONLY "Only run standalone C++ tests, without JUCE" OFF)
if(SKELL_AFAR_BUILD_HEADLESS_TESTS)
  enable_testing()
  add_executable(SkellAfarPhysics tests/test_physics.cpp)
  target_compile_features(SkellAfarPhysics PRIVATE cxx_std_17)
  add_test(NAME SkellAfarPhysics COMMAND SkellAfarPhysics)
  add_executable(SkellAfarThroat tests/test_throat.cpp)
  target_compile_features(SkellAfarThroat PRIVATE cxx_std_17)
  add_test(NAME SkellAfarThroat COMMAND SkellAfarThroat)
  add_executable(SkellAfarSceneSmoke tests/test_audio.cpp)
  target_compile_features(SkellAfarSceneSmoke PRIVATE cxx_std_17)
  target_include_directories(SkellAfarSceneSmoke PRIVATE tests/stub)
  add_test(NAME SkellAfarSceneSmoke COMMAND SkellAfarSceneSmoke)
  add_executable(SkellAfarStress tests/test_stress.cpp)
  target_compile_features(SkellAfarStress PRIVATE cxx_std_17)
  target_include_directories(SkellAfarStress PRIVATE tests/stub)
  add_test(NAME SkellAfarStress COMMAND SkellAfarStress)
  add_executable(SkellAfarBedsPresence tests/test_beds_presence.cpp)
  target_compile_features(SkellAfarBedsPresence PRIVATE cxx_std_17)
  target_include_directories(SkellAfarBedsPresence PRIVATE tests/stub)
  add_test(NAME SkellAfarBedsPresence COMMAND SkellAfarBedsPresence)
endif()
if(NOT SKELL_AFAR_HEADLESS_ONLY)
  if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("${JUCE_PATH}" JUCE)
  else()
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 8.0.12 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
  endif()
  FetchContent_Declare(SkellCore
    GIT_REPOSITORY https://github.com/skellyruno/skell-vst-template.git
    GIT_TAG ${SKELLCORE_REV}
    SOURCE_SUBDIR SkellCore)
  FetchContent_MakeAvailable(SkellCore)
  juce_add_plugin(SkellAfar
    COMPANY_NAME "Skell Audio"
    BUNDLE_ID "audio.skell.afar"
    PLUGIN_MANUFACTURER_CODE SkAu
    PLUGIN_CODE Afar
    FORMATS VST3 Standalone
    PRODUCT_NAME "Skell-Afar"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
  juce_add_binary_data(SkellAfarAssets SOURCES
    Assets/bg_panel.png Assets/logo_skull.png
    Assets/space_00.png Assets/space_01.png Assets/space_02.png Assets/space_03.png
    Assets/space_04.png Assets/space_05.png Assets/space_06.png Assets/space_07.png
    Assets/space_08.png Assets/space_09.png Assets/space_10.png Assets/space_11.png
    Assets/space_12.png Assets/space_13.png Assets/space_14.png Assets/space_15.png)
  juce_generate_juce_header(SkellAfar)
  target_sources(SkellAfar PRIVATE
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h
    Source/dsp/SceneModel.h Source/dsp/SceneDSP.h Source/dsp/ThroatTexture.h
    Source/ui/Theme.h Source/ui/SpaceBrowser.h Source/ui/SceneView.h Source/ui/Controls.h Source/ui/DetailPanel.h)
  target_compile_features(SkellAfar PRIVATE cxx_std_17)
  target_compile_definitions(SkellAfar PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(SkellAfar PRIVATE
    SkellCore::DSP SkellCore::UI SkellAfarAssets
    juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra juce::juce_dsp)
endif()
