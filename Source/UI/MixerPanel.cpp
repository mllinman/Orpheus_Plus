#include <JuceHeader.h>
#include "MixerPanel.h"
#include "../Audio/PluginManager.h"
#include "../Audio/MidiLearnManager.h"
#include "../Audio/ClipGeneratorProcessor.h"
#include "../Util/OrpheusLogger.h"
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
            auto& trackInfo = engine.getTrackInfo(trackIndex);
            int nodeID = trackInfo.pluginSlots[i];
            
            if (nodeID == -1)
            {
                // Add Plugin Menu
                juce::PopupMenu m;
                m.addSectionHeader("Add Plugin");
                
                auto& pm = engine.getPluginManager();
                const auto& list = pm.getKnownPluginList();
                
                // Copy descriptions for async-safe access
                std::vector<juce::PluginDescription> descs;
                int id = 1;
                for (const auto& desc : list.getTypes())
                {
                    m.addItem(id++, desc.name + " (" + desc.manufacturerName + ")");
                    descs.push_back(desc);
                }
                
                m.showMenuAsync(juce::PopupMenu::Options{},
                    [this, i, descs = std::move(descs)](int result)
                    {
                        if (result > 0 && result - 1 < (int)descs.size())
                        {
                            engine.getPluginManager().addPluginToTrack(trackIndex, descs[(size_t)(result - 1)]);
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
        engine.getMidiLearn().startLearning("vol", trackIndex);
    };
    addAndMakeVisible(fader);

    panKnob.setSliderStyle(juce::Slider::Rotary);
    panKnob.setRange(-1.0, 1.0, 0.001);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panKnob.onValueChange = [this] { engine.setTrackPan(trackIndex, (float)panKnob.getValue()); };
    panKnob.onMidiLearn = [this] {
        engine.getMidiLearn().startLearning("pan", trackIndex);
    };
    addAndMakeVisible(panKnob);

    sweetenerKnob.setSliderStyle(juce::Slider::Rotary);
    sweetenerKnob.setRange(0.0, 1.0, 0.001);
    sweetenerKnob.setValue(0.0, juce::dontSendNotification);
    sweetenerKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    sweetenerKnob.onValueChange = [this] { engine.setTrackSweetener(trackIndex, (float)sweetenerKnob.getValue()); };
    sweetenerKnob.setDoubleClickReturnValue(true, 0.0);
    sweetenerKnob.onMidiLearn = [this] {
        engine.getMidiLearn().startLearning("sweet", trackIndex);
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
    
    // Header (Track Name)
    nameLabel.setBounds(b.removeFromTop(18));
    
    // Plugin Slots AREA (Inset Rack Bay)
    auto slotArea = b.removeFromTop(70).reduced(2);
    int slotH = 16;
    for (auto* btn : pluginSlots)
        btn->setBounds(slotArea.removeFromTop(slotH).reduced(0, 1));
    
    // Middle section (Knobs and Buttons)
    b.removeFromTop(4);
    muteBtn.setBounds(b.removeFromTop(18).reduced(4, 0));
    b.removeFromTop(2);
    soloBtn.setBounds(b.removeFromTop(18).reduced(4, 0));
    b.removeFromTop(4);
    
    // Knobs side by side or stacked
    sweetenerKnob.setBounds(b.removeFromTop(36).reduced(2));
    panKnob.setBounds(b.removeFromTop(36).reduced(2));
    
    b.removeFromTop(4);
    // Fader Area (bottom half)
    fader.setBounds(b.reduced(4, 0));
}

void MixerPanel::ChannelStrip::paint(juce::Graphics& g)
{
    auto& info = engine.getTrackInfo(trackIndex);
    
    // Update Plugin Names 
    for (int i=0; i < pluginSlots.size(); ++i)
    {
        int nodeID = info.pluginSlots[i];
        if (nodeID != -1)
            pluginSlots[i]->setButtonText(engine.getPluginManager().getPluginName(nodeID));
        else
            pluginSlots[i]->setButtonText("+");
    }
    
    // Hardware Module Body
    g.fillAll(juce::Colour(0xff1f1f2e));
    
    // Left/Right Bevels to simulate individual hardware strips
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawVerticalLine(0, 0, (float)getHeight());
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawVerticalLine(getWidth() - 1, 0, (float)getHeight());

    // Track Name Header Background (Track Color)
    juce::Rectangle<float> headerBg(0, 0, (float)getWidth(), 20);
    juce::ColourGradient headerGrad(info.colour.withAlpha(0.8f), 0, 0,
                                    info.colour.withAlpha(0.3f), 0, 20, false);
    g.setGradientFill(headerGrad);
    g.fillRect(headerBg);
    
    // Rack Bay Inset for Plugins
    auto slotArea = juce::Rectangle<float>(2, 22, (float)getWidth() - 4, 70);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(slotArea, 2.0f);
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawRoundedRectangle(slotArea, 2.0f, 1.0f);

    // Fader Area Inset
    auto faderArea = fader.getBounds().toFloat().expanded(2);
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.fillRoundedRectangle(faderArea, 4.0f);

    // Level meter (drawn alongside fader)
    float meterW = 6.0f;
    float meterX = faderArea.getRight() - meterW - 2;
    float meterY = faderArea.getY() + 10; 
    float meterH = faderArea.getHeight() - 20;

    g.setColour(juce::Colour(0xff0d0d1a)); // Empty LED track
    g.fillRect(meterX, meterY, meterW * 2 + 2, meterH);

    // Grab peak from graph
    if (auto* node = engine.getGraph().getNodeForId(juce::AudioProcessorGraph::NodeID(info.faderNodeID)))
    {
        if (auto* faderProc = dynamic_cast<TrackFaderProcessor*>(node->getProcessor()))
        {
             peakL = faderProc->getPeakL();
             peakR = faderProc->getPeakR();
        }
    }

    // Segmented LED Drawing
    auto drawMeter = [&](juce::Rectangle<float> bounds, float peak)
    {
        int numSegments = 30;
        float segmentH = bounds.getHeight() / (float)numSegments;
        float gap = 1.0f;
        int activeSegments = (int)(peak * numSegments);

        for (int i = 0; i < numSegments; ++i)
        {
            float segY = bounds.getBottom() - (i + 1) * segmentH;
            juce::Rectangle<float> seg(bounds.getX(), segY + gap, bounds.getWidth(), segmentH - gap);

            if (i < activeSegments)
            {
                juce::Colour c = (i > 24) ? juce::Colour(0xffe94560) :
                                 (i > 20) ? juce::Colour(0xffffca28) :
                                            juce::Colour(0xff4caf50);
                g.setColour(c);
                g.fillRect(seg);
            }
            else
            {
                g.setColour(juce::Colours::white.withAlpha(0.03f));
                g.fillRect(seg);
            }
        }
    };

    drawMeter(juce::Rectangle<float>(meterX, meterY, meterW, meterH), peakL);
    drawMeter(juce::Rectangle<float>(meterX + meterW + 2, meterY, meterW, meterH), peakR);
}

//==============================================================================
// MasterStrip
//==============================================================================

MixerPanel::MasterStrip::MasterStrip(AudioEngine& e)
    : engine(e)
{
    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
    fader.setRange(-60.0, 6.0);
    fader.setValue(0.0);
    fader.setDoubleClickReturnValue(true, 0.0);
    fader.setNumDecimalPlacesToDisplay(1);
    fader.setTextValueSuffix(" dB");
    fader.onValueChange = [this]() {
        // Here we would tell AudioEngine to set master volume
        // engine.setMasterVolume(fader.getValue());
    };
    addAndMakeVisible(fader);

    nameLabel.setText("MASTER", juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setColour(juce::Label::textColourId, juce::Colour(0xffe94560));
    addAndMakeVisible(nameLabel);
}

MixerPanel::MasterStrip::~MasterStrip() {}

void MixerPanel::MasterStrip::resized()
{
    auto b = getLocalBounds();
    nameLabel.setBounds(b.removeFromBottom(24));
    b.removeFromBottom(8); // spacing
    
    // Fader
    fader.setBounds(b.removeFromBottom(200));
}

void MixerPanel::MasterStrip::paint(juce::Graphics& g)
{
    // Master Strip Body (Darker than normal tracks)
    g.fillAll(juce::Colour(0xff14141e));
    
    // Bevels
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawVerticalLine(0, 0, (float)getHeight());
    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.drawVerticalLine(getWidth() - 1, 0, (float)getHeight());

    // Red Header for Master
    juce::Rectangle<float> headerBg(0, 0, (float)getWidth(), 20);
    juce::ColourGradient headerGrad(juce::Colour(0xffe94560).withAlpha(0.5f), 0, 0,
                                    juce::Colour(0xff14141e), 0, 20, false);
    g.setGradientFill(headerGrad);
    g.fillRect(headerBg);

    // Master meters (Segmented LED style)
    auto b = fader.getBounds().toFloat().expanded(4);
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(b, 4.0f);

    float meterW = 10.0f;
    float meterX = b.getRight() - meterW * 2 - 6;
    float meterY = b.getY() + 10;
    float meterH = b.getHeight() - 20;

    g.setColour(juce::Colour(0xff0d0d1a));
    g.fillRect(meterX, meterY, meterW * 2 + 2, meterH);

    peakL = audioEngine.getMasterPeakLeft();
    peakR = audioEngine.getMasterPeakRight();

    auto drawMeter = [&](juce::Rectangle<float> bounds, float peak)
    {
        int numSegments = 40;
        float segmentH = bounds.getHeight() / (float)numSegments;
        float gap = 1.0f;
        int activeSegments = (int)(peak * numSegments);

        for (int i = 0; i < numSegments; ++i)
        {
            float segY = bounds.getBottom() - (i + 1) * segmentH;
            juce::Rectangle<float> seg(bounds.getX(), segY + gap, bounds.getWidth(), segmentH - gap);

            if (i < activeSegments)
            {
                juce::Colour c = (i > 32) ? juce::Colour(0xffe94560) :
                                 (i > 26) ? juce::Colour(0xffffca28) :
                                            juce::Colour(0xff4caf50);
                g.setColour(c);
                g.fillRect(seg);
            }
            else
            {
                g.setColour(juce::Colours::white.withAlpha(0.03f));
                g.fillRect(seg);
            }
        }
    };

    drawMeter(juce::Rectangle<float>(meterX, meterY, meterW, meterH), peakL);
    drawMeter(juce::Rectangle<float>(meterX + meterW + 2, meterY, meterW, meterH), peakR);
}

//==============================================================================
MixerPanel::MixerPanel(AudioEngine& e, AppState& s)
    : audioEngine(e), appState(s)
{
    audioEngine.addListener(this);
    channelViewport.setViewedComponent(&channelContainer, false);
    channelViewport.setScrollBarsShown(false, true);
    addAndMakeVisible(channelViewport);

    masterStrip = std::make_unique<MasterStrip>(audioEngine);
    addAndMakeVisible(masterStrip.get());

    horizontalLayout.setItemLayout(0, -0.1, -1.0, -0.7); // Channel viewport (flexible)
    horizontalLayout.setItemLayout(1, 8, 8, 8);          // Resizer bar (fixed 8px)
    horizontalLayout.setItemLayout(2, 60, 200, masterStripWidth); // Master strip

    resizerBar = std::make_unique<juce::StretchableLayoutResizerBar>(&horizontalLayout, 1, false);
    addAndMakeVisible(resizerBar.get());

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

    juce::Component dummyLeft, dummyRight;
    juce::Component* hComps[] = { &dummyLeft, resizerBar.get(), &dummyRight };
    horizontalLayout.layOutComponents(hComps, 3, bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(), false, true);
    
    channelViewport.setBounds(dummyLeft.getBounds());
    masterStrip->setBounds(dummyRight.getBounds());

    const int stripW = 72;
    channelContainer.setSize(juce::jmax(channelViewport.getWidth(),
        strips.size() * stripW), channelViewport.getHeight());

    for (int i = 0; i < strips.size(); ++i)
        strips[i]->setBounds(i * stripW, 0, stripW, channelViewport.getHeight());
}

void MixerPanel::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12121e));
    g.setColour(juce::Colour(0xff533483).withAlpha(0.5f));
    g.drawHorizontalLine(0, 0, (float)getWidth());
}

void MixerPanel::trackListChanged()
{
    OrpheusLogger::logInfo("MixerPanel::trackListChanged — rebuilding strips...");
    rebuildStrips();
    OrpheusLogger::logInfo("MixerPanel::trackListChanged — rebuild done.");
}

void MixerPanel::timerCallback()
{
    repaint();
}

void MixerPanel::rebuildStrips()
{
    OrpheusLogger::logInfo("MixerPanel::rebuildStrips — clearing " + juce::String(strips.size()) + " strips.");
    strips.clear();
    channelContainer.removeAllChildren();

    int numTracks = audioEngine.getNumTracks();
    OrpheusLogger::logInfo("MixerPanel::rebuildStrips — creating " + juce::String(numTracks) + " strips.");

    for (int i = 0; i < numTracks; ++i)
    {
        auto* strip = strips.add(new ChannelStrip(i, audioEngine));
        channelContainer.addAndMakeVisible(strip);
    }

    OrpheusLogger::logInfo("MixerPanel::rebuildStrips — calling resized.");
    resized();
    OrpheusLogger::logInfo("MixerPanel::rebuildStrips — complete.");
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
