#include "MixerPanel.h"
#include "../Audio/PluginManager.h"

//==============================================================================
MixerPanel::ChannelStrip::ChannelStrip(int idx, AudioEngine& e) : trackIndex(idx), engine(e)
{
    auto& info = engine.getTrackInfo(trackIndex);
    
    // Plugin Slots
    for (int i = 0; i < OrpheusTrackInfo::MAX_PLUGINS; ++i)
    {
        auto* b = pluginSlots.add(new juce::TextButton());
        addAndMakeVisible(b);
        b->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d3e));
        
        b->onClick = [this, i] {
            auto& trackInfo = engine.getTrackInfo(trackIndex);
            int nodeID = trackInfo.pluginSlots[i];
            
            if (nodeID == -1)
            {
                // Add Plugin Menu
                juce::PopupMenu m;
                m.addSectionHeader("Add Plugin");
                
                auto& pm = engine.getPluginManager();
                int id = 1;

                const auto& list = pm.getKnownPluginList();
                for (const auto& desc : list.getTypes())
                {
                    m.addItem(id++, desc.name + " (" + desc.manufacturerName + ")");
                }
                
                m.showMenuAsync(juce::PopupMenu::Options{}, [this, &pm](int result) {
                    if (result > 0)
                    {
                        auto& list = pm.getKnownPluginList();
                        if (result - 1 < list.getTypes().size())
                        {
                            const auto& desc = list.getTypes()[result - 1]; // unsafe if list changes?
                            pm.addPluginToTrack(trackIndex, desc);
                        }
                    }
                });
            }
            else
            {
                engine.getPluginManager().openPluginEditor(trackIndex, i);
            }
        };
    }

    nameLabel.setText(info.name, juce::dontSendNotification);
    nameLabel.setFont(juce::Font(10.0f));
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(nameLabel);

    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setRange(0.0, 1.5, 0.001);
    fader.setValue(1.0, juce::dontSendNotification);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.onValueChange = [this] { engine.setTrackVolume(trackIndex, (float)fader.getValue()); };
    addAndMakeVisible(fader);

    panKnob.setSliderStyle(juce::Slider::Rotary);
    panKnob.setRange(-1.0, 1.0, 0.001);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panKnob.onValueChange = [this] { engine.setTrackPan(trackIndex, (float)panKnob.getValue()); };
    addAndMakeVisible(panKnob);

    muteBtn.setToggleable(true);
    muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffFFB300));
    muteBtn.onClick = [this] { engine.setTrackMute(trackIndex, muteBtn.getToggleState()); };
    addAndMakeVisible(muteBtn);

    soloBtn.setToggleable(true);
    soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff00BCD4));
    soloBtn.onClick = [this] { engine.setTrackSolo(trackIndex, soloBtn.getToggleState()); };
    addAndMakeVisible(soloBtn);
}

MixerPanel::ChannelStrip::~ChannelStrip()
{
}

void MixerPanel::ChannelStrip::resized()
{
    auto b = getLocalBounds().reduced(2);
    nameLabel.setBounds(b.removeFromTop(14));
    
    // Plugin Slots AREA
    auto slotArea = b.removeFromTop(b.getHeight() / 2); // Top half for plugins
    int slotH = 18;
    for (auto* btn : pluginSlots)
        btn->setBounds(slotArea.removeFromTop(slotH).reduced(0, 1));
    
    auto btnRow = b.removeFromBottom(24);
    muteBtn.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(1));
    soloBtn.setBounds(btnRow.reduced(1));
    panKnob.setBounds(b.removeFromTop(40).reduced(4));
    fader.setBounds(b.reduced(4, 0));
}

void MixerPanel::ChannelStrip::paint(juce::Graphics& g)
{
    auto& info = engine.getTrackInfo(trackIndex);
    
    // Update Plugin Names 
    // (In paint? usually separate update, but fine for prototype)
    for (int i=0; i < pluginSlots.size(); ++i)
    {
        int nodeID = info.pluginSlots[i];
        if (nodeID != -1)
        {
            pluginSlots[i]->setButtonText(engine.getPluginManager().getPluginName(nodeID)); // Optimization needed
        }
        else
        {
            pluginSlots[i]->setButtonText("+");
        }
    }
    
    g.fillAll(juce::Colour(0xff1a1a2e));
    g.setColour(info.colour.withAlpha(0.6f));
    g.fillRect(0, 0, getWidth(), 3);
    g.setColour(juce::Colour(0xff0d0d1a));
    g.drawRect(getLocalBounds());

    // Level meter
    float meterW = 6.0f;
    float meterX = (float)(getWidth() - 16);
    float meterH = (float)(getHeight() - 50);
    float meterY = 14.0f; 
   
    meterY = 14.0f + (float)(pluginSlots.size() * 18); // Simple shift
    meterH -= (float)(pluginSlots.size() * 18);

    g.setColour(juce::Colour(0xff0d0d1a));
    g.fillRect(meterX, meterY, meterW * 2 + 2, meterH);

    auto meterColour = [](float peak) {
        return peak > 0.9f ? juce::Colour(0xffe94560) :
               peak > 0.7f ? juce::Colour(0xffffd54f) :
                             juce::Colour(0xff4caf50);
    };

    g.setColour(meterColour(peakL));
    g.fillRect(meterX, meterY + meterH * (1.0f - peakL), meterW, meterH * peakL);
    g.setColour(meterColour(peakR));
    g.fillRect(meterX + meterW + 2, meterY + meterH * (1.0f - peakR), meterW, meterH * peakR);
}

//==============================================================================
MixerPanel::MixerPanel(AudioEngine& e, AppState& s)
    : audioEngine(e), appState(s)
{
    audioEngine.addListener(this);
    channelViewport.setViewedComponent(&channelContainer, false);
    channelViewport.setScrollBarsShown(false, true);
    addAndMakeVisible(channelViewport);
    startTimerHz(30);
    rebuildStrips();
}

MixerPanel::~MixerPanel()
{
    audioEngine.removeListener(this);
    stopTimer();
}

void MixerPanel::resized()
{
    auto bounds = getLocalBounds();
    channelViewport.setBounds(bounds);

    const int stripW = 72;
    channelContainer.setSize(juce::jmax(bounds.getWidth(),
        strips.size() * stripW), bounds.getHeight());

    for (int i = 0; i < strips.size(); ++i)
        strips[i]->setBounds(i * stripW, 0, stripW, bounds.getHeight());
}

void MixerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12121e));
    g.setColour(juce::Colour(0xff533483).withAlpha(0.5f));
    g.drawHorizontalLine(0, 0, (float)getWidth());
}

void MixerPanel::trackListChanged()
{
    rebuildStrips();
}

void MixerPanel::timerCallback()
{
    repaint();
}

void MixerPanel::rebuildStrips()
{
    strips.clear();
    channelContainer.removeAllChildren();

    for (int i = 0; i < audioEngine.getNumTracks(); ++i)
    {
        auto* strip = strips.add(new ChannelStrip(i, audioEngine));
        channelContainer.addAndMakeVisible(strip);
    }

    resized();
}
