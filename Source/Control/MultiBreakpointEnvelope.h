#pragma once
#include <JuceHeader.h>
#include <vector>

class MultiBreakpointEnvelope
{
public:
    struct Point
    {
        float time;  // 0.0 to 1.0 (normalized duration)
        float value; // 0.0 to 1.0
        float curve; // 0.0 is linear, negative is log, positive is exp
    };

    MultiBreakpointEnvelope();
    ~MultiBreakpointEnvelope() = default;

    void addPoint(float time, float value, float curve = 0.0f);
    void removePoint(int index);
    void setPoint(int index, float time, float value, float curve);

    float getValueAt(float time) const;

    const std::vector<Point>& getPoints() const { return points; }

private:
    std::vector<Point> points;
    void sortPoints();
};
