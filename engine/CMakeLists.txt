cmake_minimum_required(VERSION 3.15)

project(AVFlowEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(external/JUCE)

juce_add_console_app(av_flow_engine 
    PRODUCT_NAME "AV Flow Engine"
)

find_package(Boost REQUIRED COMPONENTS system)
find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

target_sources(av_flow_engine PRIVATE 
    src/main.cpp
    src/RoutingGraph.cpp
    src/RoutingGraph.h
    src/EngineAudioCallback.h
    src/GainProcessor.h
    src/PannerProcessor.h
    src/LevelMeter.h
    src/MixMinusBus.cpp          
    src/MixMinusBus.h 
    src/EngineWebSocketServer.cpp   
    src/EngineWebSocketServer.h  
)

target_link_libraries(av_flow_engine PRIVATE 
    juce::juce_audio_devices 
    juce::juce_audio_processors
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto

)

target_compile_definitions(av_flow_engine PRIVATE 
    JUCE_WEB_BROWSER=0 
    JUCE_USE_CURL=0 
    _WIN32_WINNT=0x0A00
)

#tests
add_executable(av_flow_engine_tests
    tests/test_main.cpp
    tests/PannerProcessorTests.cpp
    tests/GainProcessorTests.cpp
    tests/MixMinusBusTests.cpp

    src/RoutingGraph.cpp
    src/RoutingGraph.h
    src/GainProcessor.h
    src/PannerProcessor.h
    src/LevelMeter.h
    src/MixMinusBus.cpp          
    src/MixMinusBus.h
)

target_link_libraries(av_flow_engine_tests PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
)

target_compile_definitions(av_flow_engine_tests PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)