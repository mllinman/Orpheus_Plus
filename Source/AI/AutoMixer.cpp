#include "AutoMixer.h"
#include "../Timeline/AudioClip.h"

void AutoMixer::analyzeSession(AudioEngine* engine)
{
    calculatedTargets.clear();
    lastClassifications.clear();

    if (engine == nullptr)
        return;

    int numTracks = engine->getNumTracks();

    for (int i = 0; i < numTracks; ++i)
    {
        auto* track = engine->getTrack(i);
        if (track == nullptr) continue;

        // ── Classify the track's instrument type ──
        TrackElementClassifier::ClassificationResult classification;

        bool classified = false;
        for (auto* clip : track->clips)
        {
            if (auto* audioClip = dynamic_cast<AudioClip*>(clip))
            {
                juce::File file(audioClip->sourceFile);
                if (file.existsAsFile())
                {
                    juce::AudioFormatManager fmtMgr;
                    fmtMgr.registerBasicFormats();
                    if (auto reader = std::unique_ptr<juce::AudioFormatReader>(
                            fmtMgr.createReaderFor(file)))
                    {
                        int samplesToRead = juce::jmin((int)reader->lengthInSamples,
                                                       (int)(reader->sampleRate * 5.0));
                        juce::AudioBuffer<float> analysisBuffer(1, samplesToRead);
                        reader->read(&analysisBuffer, 0, samplesToRead, 0, true, false);

                        classification = classifier.classify(
                            track->name, analysisBuffer, reader->sampleRate);
                        classified = true;
                    }
                }
                break;
            }
        }

        if (!classified)
        {
            // Classify by track name alone
            juce::AudioBuffer<float> emptyBuf;
            classification = classifier.classify(track->name, emptyBuf, 44100.0);
        }

        lastClassifications.push_back(classification);

        // ── Calculate instrument-aware target levels ──
        TargetLevels target;
        target.trackIndex    = i;
        target.instrumentType = classification.type;
        target.instrumentLabel = classification.subType.isNotEmpty()
            ? classification.subType
            : TrackElementClassifier::instrumentTypeToString(classification.type);
        target.targetVolumeDb = getInstrumentTargetDb(classification.type);
        target.targetPan      = getInstrumentTargetPan(classification.type, i);

        calculatedTargets.push_back(target);
    }
}

void AutoMixer::applyBalancing(AudioEngine* engine)
{
    if (engine == nullptr)
        return;

    for (const auto& target : calculatedTargets)
    {
        if (auto* track = engine->getTrack(target.trackIndex))
        {
            float linearGain = juce::Decibels::decibelsToGain(target.targetVolumeDb);
            engine->setTrackVolume(target.trackIndex, linearGain);
            engine->setTrackPan(target.trackIndex, target.targetPan);
        }
    }
}

float AutoMixer::getInstrumentTargetDb(TrackElementClassifier::InstrumentType type)
{
    using IT = TrackElementClassifier::InstrumentType;
    switch (type)
    {
        case IT::Vocals:          return -6.0f;   // Vocals sit on top
        case IT::Drums:           return -8.0f;   // Drums are the backbone
        case IT::Bass:            return -10.0f;  // Bass fills the low end
        case IT::ElectricGuitar:  return -12.0f;  // Guitars support the arrangement
        case IT::AcousticGuitar:  return -12.0f;
        case IT::Synth:           return -14.0f;  // Synths fill space
        case IT::Piano:           return -12.0f;  // Piano provides harmonic foundation
        case IT::Percussion:      return -14.0f;  // Percussion adds flavor
        case IT::Other:
        default:                  return -12.0f;  // Safe default
    }
}

float AutoMixer::getInstrumentTargetPan(TrackElementClassifier::InstrumentType type, int trackIndex)
{
    using IT = TrackElementClassifier::InstrumentType;

    // Use trackIndex to alternate pan direction for same-type instruments
    float direction = (trackIndex % 2 == 0) ? -1.0f : 1.0f;

    switch (type)
    {
        case IT::Vocals:          return 0.0f;                // Always centered
        case IT::Drums:           return 0.0f;                // Centered (overheads would be handled by sub-stems)
        case IT::Bass:            return 0.0f;                // Always centered
        case IT::ElectricGuitar:  return 0.4f * direction;    // Hard-ish pan for separation
        case IT::AcousticGuitar:  return 0.3f * direction;    // Moderate pan
        case IT::Synth:           return 0.25f * direction;   // Slight spread
        case IT::Piano:           return 0.15f * direction;   // Slight offset
        case IT::Percussion:      return 0.35f * direction;   // Good stereo variety
        case IT::Other:
        default:                  return 0.0f;
    }
}
