#include "PhaseAlignmentAI.h"

PhaseAlignmentAI::PhaseAlignmentAI()
{
}

int PhaseAlignmentAI::alignBufferToReference(juce::AudioBuffer<float>& target, 
                                             const juce::AudioBuffer<float>& reference, 
                                             int maxOffsetSamples)
{
    // Simplified cross-correlation to find phase offset
    // In a real scenario, this would use FFT convolution for efficiency
    // and analyze transients / spectral phase.
    
    int numChannels = juce::jmin(target.getNumChannels(), reference.getNumChannels());
    int numSamples = juce::jmin(target.getNumSamples(), reference.getNumSamples());
    if (numSamples == 0 || numChannels == 0)
        return 0;

    int bestOffset = 0;
    float maxCorr = -1.0f;

    // Search range: -maxOffsetSamples to +maxOffsetSamples
    for (int offset = -maxOffsetSamples; offset <= maxOffsetSamples; ++offset)
    {
        float corr = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* refData = reference.getReadPointer(ch);
            const float* tgtData = target.getReadPointer(ch);
            
            for (int i = 0; i < numSamples; ++i)
            {
                int tgtIdx = i - offset;
                if (tgtIdx >= 0 && tgtIdx < numSamples)
                {
                    corr += refData[i] * tgtData[tgtIdx];
                }
            }
        }
        
        if (corr > maxCorr)
        {
            maxCorr = corr;
            bestOffset = offset;
        }
    }

    if (bestOffset != 0)
    {
        // Apply the delay
        juce::AudioBuffer<float> temp(target.getNumChannels(), target.getNumSamples());
        temp.clear();
        for (int ch = 0; ch < target.getNumChannels(); ++ch)
        {
            const float* src = target.getReadPointer(ch);
            float* dst = temp.getWritePointer(ch);
            for (int i = 0; i < target.getNumSamples(); ++i)
            {
                int srcIdx = i - bestOffset;
                if (srcIdx >= 0 && srcIdx < target.getNumSamples())
                    dst[i] = src[srcIdx];
            }
        }
        target.makeCopyOf(temp);
    }
    
    return bestOffset;
}

void PhaseAlignmentAI::alignAllTracks(std::vector<juce::AudioBuffer<float>*>& tracks, 
                                      int maxOffsetSamples)
{
    if (tracks.size() < 2) return;
    
    // Use track 0 as the reference
    const auto& reference = *tracks[0];
    
    for (size_t i = 1; i < tracks.size(); ++i)
    {
        if (tracks[i] != nullptr)
        {
            alignBufferToReference(*tracks[i], reference, maxOffsetSamples);
        }
    }
}
