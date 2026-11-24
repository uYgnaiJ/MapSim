#include "label_layout.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

LabelLayout measureLabel(const std::vector<V>& points)
{
    if (points.empty())
        return {};
    V centroid{};
    for (auto p : points)
        centroid = centroid + p;
    centroid = centroid * (1.f / points.size());
    float xx = 0, xy = 0, yy = 0;
    for (auto p : points)
    {
        V d = p - centroid;
        xx += d.x * d.x;
        xy += d.x * d.y;
        yy += d.y * d.y;
    }
    xx /= points.size();
    xy /= points.size();
    yy /= points.size();
    float angle = .5f * std::atan2(2 * xy, xx - yy);
    V axis{std::cos(angle), std::sin(angle)};
    if (axis.x < 0 || (std::abs(axis.x) < .0001f && axis.y < 0))
        axis = axis * -1;
    V center = points.front();
    float best = std::numeric_limits<float>::max(), low = 0, high = 0;
    bool first = true;
    for (auto p : points)
    {
        V d = p - centroid;
        float distance = d.x * d.x + d.y * d.y;
        if (distance < best)
        {
            best = distance;
            center = p;
        }
        float projection = (p - centroid).x * axis.x + (p - centroid).y * axis.y;
        if (first)
            low = high = projection;
        first = false;
        low = std::min(low, projection);
        high = std::max(high, projection);
    }
    return {center, axis, high - low, true};
}
