#pragma once
#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdint>
#include <vector>

// One measured point from the lidar.
// Angle in degrees [0, 360), distance in millimetres, quality 0–255.
struct ScanPoint {
    float   angle_deg  = 0.0f;
    float   distance_mm= 0.0f;
    uint8_t quality    = 0;

    // Cartesian helpers (mm)
    float x() const { return distance_mm * std::cos(angle_deg * M_PI / 180.0f); }
    float y() const { return distance_mm * std::sin(angle_deg * M_PI / 180.0f); }

    bool isValid() const { return quality > 0 && distance_mm > 0.0f; }
};

// One full 360° revolution
using ScanFrame = std::vector<ScanPoint>;
