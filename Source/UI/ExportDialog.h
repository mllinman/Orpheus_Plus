#pragma once
#include <JuceHeader.h>
#include "../Export/AudioExportManager.h"
#include "../UI/OrpheusLookAndFeel.h"

class ExportDialog : public juce::Component
{
public:
    ExportDialog(AudioExportManager& exportManager);
    ~ExportDialog() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onCancel;
    std::function<void()> onExportStarted;

private:
    void triggerExport();
    void updatePreset(int presetId);

    AudioExportManager& manager;

    juce::Label titleLabel { "title", "EXPORT ENGINE" };
    juce::Label formatLabel { "fmt", "Format:" };
    juce::ComboBox formatBox;
    juce::Label sampleRateLabel { "sr", "Sample Rate:" };
    juce::ComboBox sampleRateBox;
    juce::Label bitDepthLabel { "bd", "Bit Depth:" };
    juce::ComboBox bitDepthBox;
    
    juce::Label modeLabel { "mode", "Source:" };
    juce::ComboBox modeBox;

    juce::Label presetLabel { "preset", "Streaming Preset:" };
    juce::ComboBox presetBox;

    juce::Label lufsLabel { "lufs", "Target LUFS:" };
    juce::Slider lufsSlider;
    juce::Label truePeakLabel { "tp", "True Peak (dB):" };
    juce::Slider truePeakSlider;

    juce::ToggleButton ditherToggle { "Apply Dithering" };
    juce::ToggleButton offlineToggle { "Offline Render (Max Speed)" };
    
    juce::TextButton exportButton { "RENDER" };
    juce::TextButton cancelButton { "Cancel" };

    juce::DropShadowEffect shadow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExportDialog)
};
