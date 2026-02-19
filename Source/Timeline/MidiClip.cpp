#include "MidiClip.h"

MidiClip::MidiClip(double start, double dur)
    : Clip(Type::Midi)
{
    startTime = start;
    duration  = dur;
    name      = "MIDI Clip";
}

MidiClip::~MidiClip()
{
}

void MidiClip::paint(juce::Graphics& g, juce::Rectangle<float> clipBounds, juce::Rectangle<int> clipArea)
{
    g.setColour(selected ? juce::Colour(0xffbb86fc) : juce::Colour(0xff7b2d8b));
    g.fillRoundedRectangle(clipBounds, 3.0f);

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(10.0f));
    g.drawText(name.isEmpty() ? "MIDI Clip" : name,
               clipBounds.toNearestInt().reduced(4, 2),
               juce::Justification::topLeft, true);

    // Draw mini piano roll preview
    if (midiData.getNumEvents() > 0)
    {
        int numNotes = midiData.getNumEvents();
        float noteH = juce::jmax(1.0f, clipBounds.getHeight() / 16.0f);

        for (int i = 0; i < numNotes; ++i)
        {
            auto* e = midiData.getEventPointer(i);
            if (e->message.isNoteOn())
            {
                int note = e->message.getNoteNumber();
                double noteStart = e->message.getTimeStamp() / duration; // Assuming relative logic
                // In a real app, timestamp might be absolute or relative. Assuming relative to clip start here.
                
                float nx = clipBounds.getX() + (float)noteStart * clipBounds.getWidth();
                float ny = clipBounds.getBottom() - (note / 127.0f) * clipBounds.getHeight();

                g.setColour(juce::Colours::white.withAlpha(0.7f));
                g.fillRect(nx, ny, 4.0f, noteH);
            }
        }
    }

    g.setColour(selected ? juce::Colours::white : juce::Colour(0xffbb86fc).withAlpha(0.5f));
    g.drawRoundedRectangle(clipBounds, 3.0f, 1.0f);
}
