#pragma once
#include "canvas.hpp"
#include <vector>

struct LabelLayout
{
    V center{}, axis{1, 0};
    float span{};
    bool valid{};
};

LabelLayout measureLabel(const std::vector<V>& points);
