cmake_minimum_required(VERSION 3.22)
project(SkellAfar VERSION 0.5.4 LANGUAGES C CXX)

include(FetchContent)
set(SKELLCORE_REV "3306ad495c93e062e39b8abbf079534b0b53da4e" CACHE STRING "Immutable SkellCore template revision")
option(SKELL_AFAR_BUILD_HEADLESS_TESTS "Build physics and scene metadata tests" ON)
set(SKELL_AFAR_REAL_SKELLCORE_DIR "" CACHE PATH "Optional: path to the SkellCore folder (containing include/SkellCore/Core.h); headless tests then run against the real FDN and limiter instead of tests/stub")
option(SKELL_AFAR_HEADLESS_ONLY "Only run standalone C++ tests, without JUCE" OFF)
if(SKELL_AFAR_BUILD_HEADLESS_TESTS)
  enable_testing()
  if(SKELL_AFAR_REAL_SKELLCORE_DIR AND EXISTS "${SKELL_AFAR_REAL_SKELLCORE_DIR}/include/SkellCore/Core.h")
    set(AFAR_CORE_INC "${SKELL_AFAR_REAL_SKELLCORE_DIR}/include")
    message(STATUS "Headless tests use the REAL SkellCore at ${AFAR_CORE_INC}")
  else()
    set(AFAR_CORE_INC "${CMAKE_CURRENT_SOURCE_DIR}/tests/stub")
    message(STATUS "Headless tests use the test STUB of SkellCore")
  endif()
  add_executable(SkellAfarPhysics tests/test_physics.cpp)
  target_compile_features(SkellAfarPhysics PRIVATE cxx_std_17)
  add_test(NAME SkellAfarPhysics COMMAND SkellAfarPhysics)
  add_executable(SkellAfarThroat tests/test_throat.cpp)
  target_compile_features(SkellAfarThroat PRIVATE cxx_std_17)
  add_test(NAME SkellAfarThroat COMMAND SkellAfarThroat)
  add_executable(SkellAfarSceneSmoke tests/test_audio.cpp)
  target_compile_features(SkellAfarSceneSmoke PRIVATE cxx_std_17)
  target_include_directories(SkellAfarSceneSmoke PRIVATE ${AFAR_CORE_INC})
  add_test(NAME SkellAfarSceneSmoke COMMAND SkellAfarSceneSmoke)
  add_executable(SkellAfarStress tests/test_stress.cpp)
  target_compile_features(SkellAfarStress PRIVATE cxx_std_17)
  target_include_directories(SkellAfarStress PRIVATE ${AFAR_CORE_INC})
  add_test(NAME SkellAfarStress COMMAND SkellAfarStress)
  add_executable(SkellAfarBedsPresence tests/test_beds_presence.cpp)
  target_compile_features(SkellAfarBedsPresence PRIVATE cxx_std_17)
  target_include_directories(SkellAfarBedsPresence PRIVATE ${AFAR_CORE_INC})
  add_test(NAME SkellAfarBedsPresence COMMAND SkellAfarBedsPresence)
  add_executable(SkellAfarLoudnessCharacter tests/test_loudness_character.cpp)
  target_compile_features(SkellAfarLoudnessCharacter PRIVATE cxx_std_17)
  target_include_directories(SkellAfarLoudnessCharacter PRIVATE ${AFAR_CORE_INC})
  add_test(NAME SkellAfarLoudnessCharacter COMMAND SkellAfarLoudnessCharacter)
  foreach(extra_test Onset Fuzz Latency_ceiling)
    string(TOLOWER ${extra_test} extra_lower)
    add_executable(SkellAfar${extra_test} tests/test_${extra_lower}.cpp)
    target_compile_features(SkellAfar${extra_test} PRIVATE cxx_std_17)
    target_include_directories(SkellAfar${extra_test} PRIVATE ${AFAR_CORE_INC})
    add_test(NAME SkellAfar${extra_test} COMMAND SkellAfar${extra_test})
  endforeach()
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
  # Preserve existing repository's header-only Guided Tour integration, without changing its UI API.
  FetchContent_Declare(SkellCoreTourStyle
    GIT_REPOSITORY https://github.com/skellyruno/skellcore-vst-template.git
    GIT_TAG eb56702153c4373799a4e8e2663e0f158c0115a6
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR _tour_headers_only)
  FetchContent_MakeAvailable(SkellCoreTourStyle)
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
    Assets/space_12.png Assets/space_13.png Assets/space_14.png Assets/space_15.png
    Assets/space_16.png Assets/space_17.png Assets/space_18.png Assets/space_19.png)
  juce_generate_juce_header(SkellAfar)
  target_sources(SkellAfar PRIVATE
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h
    Source/dsp/SceneModel.h Source/dsp/SceneDSP.h Source/dsp/ThroatTexture.h Source/dsp/RoomCharacter.h
    Source/ui/Theme.h Source/ui/SpaceBrowser.h Source/ui/SceneView.h Source/ui/Controls.h Source/ui/DetailPanel.h)
  target_include_directories(SkellAfar BEFORE PRIVATE
    "${skellcoretourstyle_SOURCE_DIR}/SkellCore/include")
  target_compile_features(SkellAfar PRIVATE cxx_std_17)
  target_compile_definitions(SkellAfar PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
  target_link_libraries(SkellAfar PRIVATE
    SkellCore::DSP SkellCore::UI SkellAfarAssets
    juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_extra juce::juce_dsp)
  option(SKELL_AFAR_TOUR_SCREENSHOTS "Build actual editor tour screenshot validation" OFF)
  if(SKELL_AFAR_TOUR_SCREENSHOTS)
    if(NOT TARGET SkellCore::UIRenderer)
      message(FATAL_ERROR "Pinned SkellCore UI renderer is required")
    endif()
    add_executable(SkellAfarTourScreenshots tests/ui_tour_renderer.cpp)
    target_include_directories(SkellAfarTourScreenshots BEFORE PRIVATE
      "${skellcoretourstyle_SOURCE_DIR}/SkellCore/include")
    target_include_directories(SkellAfarTourScreenshots PRIVATE
      "${CMAKE_CURRENT_BINARY_DIR}/SkellAfar_artefacts/JuceLibraryCode")
    target_link_libraries(SkellAfarTourScreenshots PRIVATE
      SkellAfar SkellCore::UIRenderer SkellCore::UI SkellCore::DSP
      SkellAfarAssets juce::juce_audio_utils juce::juce_gui_extra)
    target_compile_features(SkellAfarTourScreenshots PRIVATE cxx_std_17)
    target_compile_definitions(SkellAfarTourScreenshots PRIVATE
      JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
  endif()
endif()
