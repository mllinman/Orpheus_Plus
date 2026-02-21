#include <JuceHeader.h>
#include "MixerPanel.h"
#include "../Audio/PluginManager.h"
#include "../Audio/MidiLearnManager.h"
#include "../Audio/ClipGeneratorProcessor.h"
#include "../Audio/TrackFaderProcessor.h"

//==============================================================================
MixerPanel::ChannelStrip::ChannelStrip(int idx, AudioEngine& e) : trackIndex(idx), engine(e)
{
    auto& info = engine.getTrackInfo(trackIndex);
    
    // Plugin Slots
    for (int i = 0; i < OrpheusTrackInfo::MAX_PLUGINS; ++i)
    {
        auto* b = pluginSlots.add(new PluginSlot(trackIndex, i, engine));
        addAndMakeVisible(b);
        b->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2d2d3e));
        
        b->onClick = [this, i] {
            // ... (keep existing click logic, but maybe update text update logic elsewhere usually)
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
                
                m.showMenuAsync(juce::PopupMenu::Options{}, [this, &pm, i](int result) {
                    if (result > 0)
                    {
                        auto& list = pm.getKnownPluginList();
                        if (result - 1 < list.getTypes().size())
                        {
                            const auto& desc = list.getTypes()[result - 1]; // unsafe if list changes?
                            pm.addPluginToTrack(trackIndex, desc); // Note: this defaults to first empty slot. 
                            // We need addPluginToTrack(trackIndex, desc, slotIndex)
                            // But for now, let's just stick to "add to track" and refactor if needed. 
                            // Wait, if I click slot 2 but slot 0 is empty, where should it go?
                            // Ideally slot 2. 
                            // Let's defer that refinement.
                        }
                    }
                });
            }
            else
            {
                // Right click handled? No this is Click.
                // Opens editor.
                if (juce::ModifierKeys::getCurrentModifiers().isPopupMenu())
                {
                     // Remove menu
                     juce::PopupMenu m;
                     m.addItem(1, "Remove Plugin");
                     m.showMenuAsync(juce::PopupMenu::Options{}, [this, i](int result) {
                         if (result == 1) engine.getPluginManager().removePluginFromTrack(trackIndex, i);
                     });
                }
                else
                {
                    engine.getPluginManager().openPluginEditor(trackIndex, i);
                }
            }
        };
    }




    nameLabel.setText(info.name, juce::dontSendNotification);
    nameLabel.setFont(juce::Font(10.0f));
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(nameLabel);

    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setRange(0.0001, 2.0, 0.001); // Avoid 0 for log ease, visually mapped to -inf to +6dB
    fader.setSkewFactorFromMidPoint(0.25); // Logarithmic feel centered around unity gain (1.0)
    fader.setValue(1.0, juce::dontSendNotification);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.onValueChange = [this] { engine.setTrackVolume(trackIndex, (float)fader.getValue()); };
    fader.setDoubleClickReturnValue(true, 1.0);
    fader.onMidiLearn = [this] {
        engine.getMidiLearn().setLearnMode(true, { ParameterTarget::Type::TrackVolume, trackIndex });
    };
    addAndMakeVisible(fader);

    panKnob.setSliderStyle(juce::Slider::Rotary);
    panKnob.setRange(-1.0, 1.0, 0.001);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panKnob.onValueChange = [this] { engine.setTrackPan(trackIndex, (float)panKnob.getValue()); };
    panKnob.onMidiLearn = [this] {
        engine.getMidiLearn().setLearnMode(true, { ParameterTarget::Type::TrackPan, trackIndex });
    };
    addAndMakeVisible(panKnob);

    sweetenerKnob.setSliderStyle(juce::Slider::Rotary);
    sweetenerKnob.setRange(0.0, 1.0, 0.001);
    sweetenerKnob.setValue(0.0, juce::dontSendNotification);
    sweetenerKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sweetenerKnob.onValueChange = [this] { engine.setTrackSweetener(trackIndex, (float)sweetenerKnob.getValue()); };
    sweetenerKnob.setDoubleClickReturnValue(true, 0.0);
    sweetenerKnob.onMidiLearn = [this] {
        engine.getMidiLearn().setLearnMode(true, { ParameterTarget::Type::TrackSweet, trackIndex });
    };
    addAndMakeVisible(sweetenerKnob);

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
    sweetenerKnob.setBounds(b.removeFromTop(36).reduced(2));
    panKnob.setBounds(b.removeFromTop(36).reduced(2));
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

    // Grab peak from graph via node -> processor
    if (auto* node = engine.getGraph().getNodeForId(juce::AudioProcessorGraph::NodeID(info.faderNodeID)))
    {
        if (auto* faderProc = dynamic_cast<TrackFaderProcessor*>(node->getProcessor()))
        {
             peakL = faderProc->getPeakL();
             peakR = faderProc->getPeakR();
        }
    }

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

bool MixerPanel::ChannelStrip::PluginSlot::isInterestedInDragSource(const SourceDetails& dragSourceDetails)
{
    auto desc = dragSourceDetails.description.toString();
    return desc.startsWith("PluginDesc:") || desc.startsWith("PluginSlot:");
}

void MixerPanel::ChannelStrip::PluginSlot::itemDropped(const SourceDetails& dragSourceDetails)
{
    auto desc = dragSourceDetails.description.toString();
    
    if (desc.startsWith("PluginDesc:"))
    {
        juce::String xmlString = desc.substring(11);
        auto xml = juce::parseXML(xmlString);
        if (xml)
        {
            juce::PluginDescription pd;
            if (pd.loadFromXml(*xml))
                engine.getPluginManager().addPluginToTrack(trackIndex, pd);
        }
    }
    else if (desc.startsWith("PluginSlot:"))
    {
        // Format: PluginSlot:trackIndex:slotIndex
        auto tokens = juce::StringArray::fromTokens(desc.substring(11), ":", "");
        if (tokens.size() == 2)
        {
            int sourceTrack = tokens[0].getIntValue();
            int sourceSlot = tokens[1].getIntValue();
            
            if (sourceTrack == trackIndex)
            {
                engine.getPluginManager().movePlugin(trackIndex, sourceSlot, slotIndex);
            }
        }
    }
}

void MixerPanel::ChannelStrip::PluginSlot::mouseDrag(const juce::MouseEvent& e)
{
    if (e.mouseWasDraggedSinceMouseDown())
    {
        auto& info = engine.getTrackInfo(trackIndex);
        if (info.pluginSlots[slotIndex] != -1)
        {
            if (auto* container = juce::DragAndDropContainer::findParentDragContainerFor(this))
            {
                if (!container->isDragAndDropActive())
                {
                    container->startDragging("PluginSlot:" + juce::String(trackIndex) + ":" + juce::String(slotIndex), this);
                }
            }
        }
    }
}
