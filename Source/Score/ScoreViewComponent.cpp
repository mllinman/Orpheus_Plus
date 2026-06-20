#include "ScoreViewComponent.h"
#include "../UI/OrpheusLookAndFeel.h"

ScoreViewComponent::ScoreViewComponent()
{
    addAndMakeVisible(exportXmlBtn);
    exportXmlBtn.onClick = [this] { exportToMusicXML(); };
}

ScoreViewComponent::~ScoreViewComponent()
{
}

void ScoreViewComponent::setMidiClip(MidiClip* clip)
{
    activeClip = clip;
    repaint();
}

void ScoreViewComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xfff5f5f5)); // Standard sheet music off-white

    auto bounds = getLocalBounds();
    int staffY = bounds.getHeight() / 2;

    drawStaff(g, staffY);

    if (activeClip)
    {
        // Simple mapping just to verify render
        int xPos = 50;
        for (auto* event : activeClip->midiData)
        {
            if (event->message.isNoteOn())
            {
                drawNote(g, event->message.getNoteNumber(), staffY, xPos);
                xPos += 40; // Spacing
            }
        }
    }
}

void ScoreViewComponent::resized()
{
    exportXmlBtn.setBounds(10, 10, 120, 30);
}

void ScoreViewComponent::drawStaff(juce::Graphics& g, int yCenter)
{
    g.setColour(juce::Colours::black);
    // Draw 5 staff lines
    for (int i = -2; i <= 2; ++i)
    {
        g.drawHorizontalLine(yCenter + (i * 10), 20.0f, (float)getWidth() - 20.0f);
    }
}

void ScoreViewComponent::drawNote(juce::Graphics& g, int pitch, int staffY, int xPos)
{
    g.setColour(juce::Colours::black);
    
    // Very naive mapping of pitch to y offset
    int pitchOffset = (60 - pitch) * 5; 
    
    // Draw note head
    g.fillEllipse((float)xPos, (float)(staffY + pitchOffset), 12.0f, 10.0f);
    
    // Draw stem
    g.drawLine((float)(xPos + 11), (float)(staffY + pitchOffset + 5), (float)(xPos + 11), (float)(staffY + pitchOffset - 30), 1.5f);
}

void ScoreViewComponent::exportToMusicXML()
{
    if (!activeClip || activeClip->midiData.getNumEvents() == 0)
        return;

    fileChooser = std::make_unique<juce::FileChooser>("Export MusicXML",
                                                      juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
                                                      "*.xml");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this] (const juce::FileChooser& fc)
    {
        juce::File file = fc.getResult();
        if (file != juce::File{})
        {
            juce::String xmlHeader = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
            xmlHeader += "<!DOCTYPE score-partwise PUBLIC \"-//Recordare//DTD MusicXML 3.1 Partwise//EN\" \"http://www.musicxml.org/dtds/partwise.dtd\">\n";
            xmlHeader += "<score-partwise version=\"3.1\">\n";
            xmlHeader += "  <part-list>\n    <score-part id=\"P1\">\n      <part-name>Piano</part-name>\n    </score-part>\n  </part-list>\n";
            xmlHeader += "  <part id=\"P1\">\n";
            xmlHeader += "    <measure number=\"1\">\n";
            
            // Mocking note entries
            for (auto* event : activeClip->midiData)
            {
                if (event->message.isNoteOn())
                {
                    xmlHeader += "      <note>\n";
                    xmlHeader += "        <pitch>\n";
                    // Super naive conversion
                    int p = event->message.getNoteNumber();
                    juce::String step = "C"; 
                    if (p % 12 == 2) step = "D";
                    else if (p % 12 == 4) step = "E";
                    else if (p % 12 == 5) step = "F";
                    else if (p % 12 == 7) step = "G";
                    else if (p % 12 == 9) step = "A";
                    else if (p % 12 == 11) step = "B";
                    
                    xmlHeader += "          <step>" + step + "</step>\n";
                    xmlHeader += "          <octave>" + juce::String((p / 12) - 1) + "</octave>\n";
                    xmlHeader += "        </pitch>\n";
                    xmlHeader += "        <duration>1</duration>\n";
                    xmlHeader += "        <type>quarter</type>\n";
                    xmlHeader += "      </note>\n";
                }
            }
            
            xmlHeader += "    </measure>\n";
            xmlHeader += "  </part>\n";
            xmlHeader += "</score-partwise>\n";

            file.replaceWithText(xmlHeader);
        }
    });
}
