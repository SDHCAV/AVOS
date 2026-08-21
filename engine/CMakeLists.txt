cmake_minimum_required(VERSION 3.25)

project(AVFlowEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/JUCE)

juce_add_console_app(av_flow_engine 
    PRODUCT_NAME "AV Flow Engine"
)

target_sources(av_flow_engine PRIVATE 
    src/main.cpp
    src/RoutingGraph.cpp
    src/RoutingGraph.h
    src/EngineAudioCallback.h
    src/GainProcessor.h
    src/PannerProcessor.h
    src/LevelMeter.h
)

target_link_libraries(av_flow_engine PRIVATE 
    juce::juce_audio_devices 
    juce::juce_audio_processors
)

target_compile_definitions(av_flow_engine PRIVATE 
    JUCE_WEB_BROWSER=0 
    JUCE_USE_CURL=0
)

add_executable(av_flow_engine_tests
    tests/test_main.cpp
    tests/PannerProcessorTests.cpp
    tests/GainProcessorTests.cpp
    src/RoutingGraph.cpp
    src/RoutingGraph.h
    src/GainProcessor.h
    src/PannerProcessor.h
    src/LevelMeter.h
)

target_link_libraries(av_flow_engine_tests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
)

target_compile_definitions(av_flow_engine_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)