#include "MultiBreakpointEnvelope.h"
#include <algorithm>
#include <cmath>

MultiBreakpointEnvelope::MultiBreakpointEnvelope()
{
    // Default linear envelope
    points.push_back({0.0f, 0.0f, 0.0f});
    points.push_back({1.0f, 1.0f, 0.0f});
}

void MultiBreakpointEnvelope::addPoint(float time, float value, float curve)
{
    points.push_back({time, value, curve});
    sortPoints();
}

void MultiBreakpointEnvelope::removePoint(int index)
{
    if (index > 0 && index < static_cast<int>(points.size()) - 1)
    {
        points.erase(points.begin() + index);
    }
}

void MultiBreakpointEnvelope::setPoint(int index, float time, float value, float curve)
{
    if (index >= 0 && index < static_cast<int>(points.size()))
    {
        // Don't allow ends to move on the time axis
        if (index == 0) time = 0.0f;
        if (index == static_cast<int>(points.size()) - 1) time = 1.0f;

        points[index] = {time, value, curve};
        sortPoints();
    }
}

void MultiBreakpointEnvelope::sortPoints()
{
    std::sort(points.begin(), points.end(), [](const Point& a, const Point& b) {
        return a.time < b.time;
    });
}

float MultiBreakpointEnvelope::getValueAt(float time) const
{
    if (points.empty()) return 0.0f;
    if (time <= points.front().time) return points.front().value;
    if (time >= points.back().time) return points.back().value;

    for (size_t i = 0; i < points.size() - 1; ++i)
    {
        if (time >= points[i].time && time <= points[i+1].time)
        {
            const auto& p1 = points[i];
            const auto& p2 = points[i+1];
            
            float t = (time - p1.time) / (p2.time - p1.time);
            
            if (p1.curve == 0.0f)
            {
                // Linear
                return p1.value + t * (p2.value - p1.value);
            }
            else
            {
                // Exponential/Logarithmic approximation
                float curveAmount = std::pow(2.0f, p1.curve);
                float ct = (std::pow(curveAmount, t) - 1.0f) / (curveAmount - 1.0f);
                return p1.value + ct * (p2.value - p1.value);
            }
        }
    }
    return 0.0f;
}
