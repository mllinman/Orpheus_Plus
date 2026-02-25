#pragma once
#include <JuceHeader.h>
#include <cmath>

/**
 * Object-based 3D panner for surround/immersive audio.
 * Supports 7.1.4 bed channels (scaffold for Dolby Atmos Production Suite).
 */
class SurroundPanner
{
public:
    SurroundPanner() = default;

    struct Position
    {
        float azimuth   = 0.0f;   // -180 to 180 degrees (0 = front center)
        float elevation = 0.0f;   // -90 to 90 degrees (0 = ear level)
        float distance  = 1.0f;   // 0..1, where 0 = closest, 1 = farthest
    };

    void setPosition(Position pos) { position_ = pos; }
    Position getPosition() const { return position_; }

    /** Calculate gain coefficients for a 7.1.4 bed layout.
     *  Channels: L, R, C, LFE, Ls, Rs, Lss, Rss, Ltf, Rtf, Ltr, Rtr
     */
    std::array<float, 12> calculateGains() const
    {
        std::array<float, 12> gains{};

        float azRad = position_.azimuth * juce::MathConstants<float>::pi / 180.0f;
        float elRad = position_.elevation * juce::MathConstants<float>::pi / 180.0f;

        // Distance attenuation
        float distGain = 1.0f / juce::jmax(0.1f, position_.distance + 0.5f);

        // Horizontal panning (simplified VBAP-like)
        float cosAz = std::cos(azRad);
        float sinAz = std::sin(azRad);
        float cosEl = std::cos(elRad);
        float sinEl = std::sin(elRad);

        // Ear-level channels (0=L, 1=R, 2=C, 4=Ls, 5=Rs, 6=Lss, 7=Rss)
        float earLevel = cosEl * distGain;
        float frontBack = (cosAz + 1.0f) * 0.5f; // 0=back, 1=front

        gains[0] = earLevel * juce::jmax(0.0f, -sinAz) * frontBack;        // L
        gains[1] = earLevel * juce::jmax(0.0f,  sinAz) * frontBack;        // R
        gains[2] = earLevel * juce::jmax(0.0f,  cosAz) * 0.7f;              // C
        gains[3] = 0.1f * distGain;                                          // LFE (constant low level)
        gains[4] = earLevel * juce::jmax(0.0f, -sinAz) * (1.0f - frontBack); // Ls
        gains[5] = earLevel * juce::jmax(0.0f,  sinAz) * (1.0f - frontBack); // Rs
        gains[6] = earLevel * juce::jmax(0.0f, -sinAz) * 0.5f;              // Lss
        gains[7] = earLevel * juce::jmax(0.0f,  sinAz) * 0.5f;              // Rss

        // Height channels
        float heightLevel = juce::jmax(0.0f, sinEl) * distGain;
        gains[8]  = heightLevel * juce::jmax(0.0f, -sinAz) * frontBack;       // Ltf
        gains[9]  = heightLevel * juce::jmax(0.0f,  sinAz) * frontBack;       // Rtf
        gains[10] = heightLevel * juce::jmax(0.0f, -sinAz) * (1.0f - frontBack); // Ltr
        gains[11] = heightLevel * juce::jmax(0.0f,  sinAz) * (1.0f - frontBack); // Rtr

        return gains;
    }

private:
    Position position_;
};
