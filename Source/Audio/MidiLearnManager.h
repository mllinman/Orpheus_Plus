#pragma once
#include <JuceHeader.h>
#include <vector>
#include <map>

/**
 * Global MIDI Learn manager.
 * Click any control → move a CC → mapping saved.
 */
class MidiLearnManager
{
public:
    struct MidiMapping
    {
        int          channel;       // MIDI channel (1-16, 0=omni)
        int          cc;            // CC number (0-127)
        juce::String targetParam;   // Parameter ID (e.g., "vol", "pan")
        int          trackIndex;    // Track this mapping applies to (-1 = global)
    };

    MidiLearnManager() = default;

    // ── Learn Mode ──────────────────────────────────────────────────────
    void startLearning(const juce::String& targetParam, int trackIndex)
    {
        learningTarget_ = targetParam;
        learningTrack_  = trackIndex;
        isLearning_     = true;
    }

    void cancelLearning()
    {
        isLearning_ = false;
        learningTarget_ = {};
    }

    bool isLearning() const { return isLearning_; }

    /** Call this when a CC message arrives. Returns true if a mapping was created. */
    bool handleCC(int channel, int cc)
    {
        if (!isLearning_) return false;

        // Create mapping
        MidiMapping m;
        m.channel     = channel;
        m.cc          = cc;
        m.targetParam = learningTarget_;
        m.trackIndex  = learningTrack_;

        // Remove any existing mapping for this CC
        mappings_.erase(
            std::remove_if(mappings_.begin(), mappings_.end(),
                [&](const MidiMapping& existing) {
                    return existing.channel == channel && existing.cc == cc;
                }),
            mappings_.end());

        mappings_.push_back(m);
        isLearning_ = false;
        learningTarget_ = {};
        return true;
    }

    /** Look up the value for a given param+track from current CC states. */
    float getValueForParam(const juce::String& param, int trackIndex) const
    {
        for (auto& m : mappings_)
        {
            if (m.targetParam == param && m.trackIndex == trackIndex)
            {
                auto it = ccValues_.find(makeCCKey(m.channel, m.cc));
                if (it != ccValues_.end())
                    return it->second;
            }
        }
        return -1.0f; // No mapping
    }

    /** Store the latest CC value (called from MIDI input callback). */
    void updateCCValue(int channel, int cc, float normalizedValue)
    {
        ccValues_[makeCCKey(channel, cc)] = normalizedValue;
    }

    const std::vector<MidiMapping>& getMappings() const { return mappings_; }

    void removeMapping(int index)
    {
        if (index >= 0 && index < (int)mappings_.size())
            mappings_.erase(mappings_.begin() + index);
    }

    void clearAllMappings() { mappings_.clear(); }

private:
    static int makeCCKey(int channel, int cc) { return channel * 128 + cc; }

    std::vector<MidiMapping> mappings_;
    std::map<int, float>     ccValues_;  // key = channel*128+cc → normalized value

    bool         isLearning_     = false;
    juce::String learningTarget_;
    int          learningTrack_  = -1;
};
