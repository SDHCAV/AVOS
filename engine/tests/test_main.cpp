#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>

class ConsoleLogger : public juce::Logger {
public:
    void logMessage(const juce::String& message) override {
        std::cout << message.toStdString() << std::endl;
    }
};

int main() {
    juce::ScopedJuceInitialiser_GUI gui;

    ConsoleLogger logger;
    juce::Logger::setCurrentLogger(&logger);

    std::cout << "========================================" << std::endl;
    std::cout << "       Running AVOS Unit Tests          " << std::endl;
    std::cout << "========================================" << std::endl;

    juce::UnitTestRunner runner;
    runner.runAllTests();

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}