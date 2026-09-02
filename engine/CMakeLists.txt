cmake_minimum_required(VERSION 3.15)

project(AVFlowEngine VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

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
    src/MixMinusBus.cpp          
    src/MixMinusBus.h 
    src/EngineWebSocketServer.cpp   
    src/EngineWebSocketServer.h  
)

target_link_libraries(av_flow_engine PRIVATE 
    juce::juce_audio_devices 
    juce::juce_audio_processors
    simple-websocket-server

)

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

#External Dependencies
include(FetchContent)

FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-12-2
)

FetchContent_Declare(
    simple_websocket_server
    GIT_REPOSITORY https://github.com/eidheim/Simple-WebSocket-Server.git
    GIT_TAG 1c4b98e7f22875c7e0280bc8c852c1a1be250a14  
)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_MakeAvailable(asio)

set(USE_STANDALONE_ASIO ON CACHE BOOL "" FORCE)
set(BUILD_WITHOUT_BOOST ON CACHE BOOL "" FORCE)

set(ASIO_INCLUDE_DIR "${asio_SOURCE_DIR}/asio/include" CACHE PATH "" FORCE)
list(APPEND CMAKE_REQUIRED_INCLUDES "${asio_SOURCE_DIR}/asio/include")
list(APPEND CMAKE_REQUIRED_DEFINITIONS "-DASIO_STANDALONE" "-D_WIN32_WINNT=0x0A00")

FetchContent_MakeAvailable(simple_websocket_server)

target_include_directories(av_flow_engine PRIVATE
    ${asio_SOURCE_DIR}/asio/include
)

target_compile_definitions(av_flow_engine PRIVATE 
    JUCE_WEB_BROWSER=0 
    JUCE_USE_CURL=0 
    ASIO_STANDALONE
    _WIN32_WINNT=0x0A00
)

if (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(av_flow_engine PRIVATE Threads::Threads)
endif()