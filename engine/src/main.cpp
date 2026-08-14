#include <juce_audio_devices/juce_audio_devices.h>

int main() {
    juce::ScopedJuceInitialiser_GUI gui;

    juce::AudioDeviceManager manager;
    manager.initialiseWithDefaultDevices(2, 2);

    auto* type = manager.getCurrentDeviceTypeObject();

    if (type != nullptr) {
        auto deviceNames = type->getDeviceNames();
        juce::Logger::writeToLog("--- Available Audio Devices ---");
        for (const auto& name : deviceNames) {
            juce::Logger::writeToLog(name);
        }
    }
    return 0;
}

//run the following to build toolchain and confirm compiler, CMake, and folder strucutre correctly wired before adding JUICE dependency
/*
cd engine/build
rm -rf *
cmake ..
cmake --build .

*/