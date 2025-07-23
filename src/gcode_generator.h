#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <limits>
#include <vector>
#include <utility>

namespace Stratum {

// Generates a very simple raster-style G-code toolpath from an ASCII STL file.
// The STL is treated as a flat 2D shape; the XY extents are used to scan the
// LED across a single layer. The feed rate is derived from the provided
// radiant exposure vs. cure depth mapping. Throws std::runtime_error if the
// file cannot be opened.
inline double interpolate_exposure(
    double depth,
    const std::vector<std::pair<double, double>>& curve) {
    if (curve.empty()) {
        return 0.0;
    }
    if (depth <= curve.front().first) {
        return curve.front().second;
    }
    for (std::size_t i = 1; i < curve.size(); ++i) {
        if (depth <= curve[i].first) {
            double d1 = curve[i - 1].first;
            double d2 = curve[i].first;
            double e1 = curve[i - 1].second;
            double e2 = curve[i].second;
            return e1 + (e2 - e1) * (depth - d1) / (d2 - d1);
        }
    }
    return curve.back().second;
}

template <typename OutputIt>
void generate_from_stl(const std::filesystem::path& stl_path,
                       double led_radius,
                       const std::vector<std::pair<double, double>>& exposure_curve,
                       const std::string& mode,
                       double power,
                       const std::string& bitmask_path,
                       OutputIt out) {
    std::ifstream file(stl_path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open " + stl_path.string());
    }

    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();

    std::string token;
    while (file >> token) {
        if (token == "vertex") {
            double x, y, z;
            file >> x >> y >> z;
            min_x = std::min(min_x, x);
            min_y = std::min(min_y, y);
            max_x = std::max(max_x, x);
            max_y = std::max(max_y, y);
        }
    }

    if (min_x == std::numeric_limits<double>::max()) {
        // No vertices found; default to origin
        min_x = min_y = max_x = max_y = 0.0;
    }

    double step = 2.0 * led_radius;
    double exposure = interpolate_exposure(led_radius, exposure_curve);
    double feed_rate = exposure > 0.0 ? 1000.0 / exposure : 1000.0;

    *out++ = "; Begin G-code generated from STL";
    *out++ = "; Photopolymerization toolpath";
    {
        std::ostringstream line;
        line << "; Mode: " << mode;
        *out++ = line.str();
    }
    {
        std::ostringstream line;
        line << "; Power: " << power;
        *out++ = line.str();
    }
    if (mode == "LCD") {
        std::ostringstream line;
        line << "; LED bitmask: " << bitmask_path;
        *out++ = line.str();
    }
    {
        std::ostringstream line;
        line << "; LED radius: " << led_radius << " mm";
        *out++ = line.str();
    }
    {
        std::ostringstream line;
        line << "; Step size: " << step << " mm";
        *out++ = line.str();
    }
    {
        std::ostringstream line;
        line << "; Feed rate: " << feed_rate << " mm/min";
        *out++ = line.str();
    }
    *out++ = "G21"; // millimeter units
    *out++ = "G90"; // absolute coordinates

    bool forward = true;
    for (double y = min_y; y <= max_y; y += step) {
        std::ostringstream start;
        std::ostringstream end;
        if (forward) {
            start << "G1 X" << min_x << " Y" << y << " F" << feed_rate;
            end << "G1 X" << max_x << " Y" << y;
        } else {
            start << "G1 X" << max_x << " Y" << y << " F" << feed_rate;
            end << "G1 X" << min_x << " Y" << y;
        }
        *out++ = start.str();
        *out++ = end.str();
        forward = !forward;
    }

    *out++ = "; End G-code";
}

} // namespace Stratum
