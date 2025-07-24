#pragma once
/*
 * Stratum - minimal light-engine toolset
 * C++20 - CUDA 12.x compatible (host-only)
 *
 * Dependencies (header-only, ASCII-only -- add them anywhere in your include path):
 * - lodepng.h    - https://github.com/lvandeve/lodepng
 */

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <filesystem>
#include <limits>
#include <vector>
#include <utility>
#include <variant>
#include <functional>
#include <cmath>
#include <iterator>
#include <concepts>
#include <iomanip>
#include <cstdint>
#include <algorithm>
#include <map>
#include <list>

#include "lodepng.h"   // PNG encoder (header-only)

namespace Stratum
{
//
// Helper types
//
struct LCDConfig
{
    int   cols               = 0;      // pixel columns
    int   rows               = 0;      // pixel rows
    double led_radius        = 0.0;    // mm
    double optical_power     = 0.0;    // mW / cm^2 (metadata only)
    double layer_height      = 0.05;   // mm
    double exposure_s        = 8.0;    // s
    double padding_percentage= 10.0;   // %, border around auto-scaled model
    int   intensity_pct      = 100;    // 0-100 for M701... Ixxx
    std::filesystem::path png_dir = "layers";
    double final_lift_mm     = 5.0;    // mm, final lift height post-print. Set to 0 to disable.
};

struct SLAConfig
{
    double spot_radius       = 0.0;    // mm
    double optical_power     = 0.0;    // mW / cm^2 (metadata only)
    double layer_height      = 0.05;   // mm
    double padding_percentage= 10.0;   // %, for consistent auto-scaling (if used)
    double laser_power_pct   = 100.0;  // 0-100 -> M3 Sxxx
    double final_lift_mm     = 5.0;    // mm, final lift height post-print. Set to 0 to disable.
};

template<class T>
concept IsLCD = requires(T t)
{
    t.cols; t.rows; t.led_radius; t.padding_percentage;
};

template<class T>
concept IsSLA = requires(T t)
{
    t.spot_radius; t.padding_percentage;
};

// Full 3D bounds
struct Bounds3D
{
    double min_x, min_y, min_z;
    double max_x, max_y, max_z;
};

/*
 ************************************************************************
 * Geometry and Slicing Helpers
 ************************************************************************
 */
namespace Slicer
{
    struct Vec3 { double x{}, y{}, z{}; };
    struct Triangle { Vec3 v1, v2, v3; };
    struct Vec2 { double x{}, y{}; };
    struct Segment2D { Vec2 p1, p2; };

    // Reads all triangles from an ASCII STL file and computes the 3D bounds.
    inline std::vector<Triangle> readStl(const std::filesystem::path& p, Bounds3D& out_bounds) {
        std::ifstream f(p);
        if (!f) {
            throw std::runtime_error("cannot open " + p.string());
        }

        const double maxD = std::numeric_limits<double>::max();
        const double minD = std::numeric_limits<double>::lowest();
        out_bounds = { maxD, maxD, maxD, minD, minD, minD };

        std::vector<Triangle> triangles;
        std::string line;
        Vec3 vertices[3];
        int vertex_count = 0;

        while (std::getline(f, line)) {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            if (token == "vertex") {
                iss >> vertices[vertex_count].x >> vertices[vertex_count].y >> vertices[vertex_count].z;

                out_bounds.min_x = std::min(out_bounds.min_x, vertices[vertex_count].x);
                out_bounds.min_y = std::min(out_bounds.min_y, vertices[vertex_count].y);
                out_bounds.min_z = std::min(out_bounds.min_z, vertices[vertex_count].z);
                out_bounds.max_x = std::max(out_bounds.max_x, vertices[vertex_count].x);
                out_bounds.max_y = std::max(out_bounds.max_y, vertices[vertex_count].y);
                out_bounds.max_z = std::max(out_bounds.max_z, vertices[vertex_count].z);

                if (++vertex_count == 3) {
                    triangles.push_back({vertices[0], vertices[1], vertices[2]});
                    vertex_count = 0;
                }
            }
        }
        if (out_bounds.min_x == maxD) out_bounds = {0,0,0,0,0,0};
        return triangles;
    }

    // Calculates the intersection of a line segment (p1-p2) with a Z-plane.
    inline Vec3 intersectionPoint(const Vec3& p1, const Vec3& p2, double z) {
        if (std::abs(p2.z - p1.z) < 1e-9) return p1;
        double t = (z - p1.z) / (p2.z - p1.z);
        return { p1.x + t * (p2.x - p1.x), p1.y + t * (p2.y - p1.y), z };
    }

    // Slices a mesh of triangles at a given Z height, returning 2D line segments.
    inline std::vector<Segment2D> sliceTriangles(const std::vector<Triangle>& triangles, double z) {
        std::vector<Segment2D> segments;
        for (const auto& tri : triangles) {
            const Vec3* V[] = {&tri.v1, &tri.v2, &tri.v3};
            double z_coords[] = {V[0]->z, V[1]->z, V[2]->z};

            int below_count = 0;
            for(double v_z : z_coords) {
                if (v_z < z) below_count++;
            }

            if (below_count == 0 || below_count == 3) continue;

            Vec3 intersection_points[2];
            int intersection_count = 0;

            for (int i = 0; i < 3; ++i) {
                int j = (i + 1) % 3;
                bool v1_below = z_coords[i] < z;
                bool v2_below = z_coords[j] < z;

                if (v1_below != v2_below) {
                    if (intersection_count < 2) {
                       intersection_points[intersection_count++] = intersectionPoint(*V[i], *V[j], z);
                    }
                }
            }

            if (intersection_count == 2) {
                segments.push_back({{intersection_points[0].x, intersection_points[0].y},
                                    {intersection_points[1].x, intersection_points[1].y}});
            }
        }
        return segments;
    }

    // Fills a pixel mask by rasterizing 2D line segments, applying an offset to center the model.
    inline void rasterizeCenteredSegments(std::vector<uint8_t>& mask, int w, int h, double pitch,
                                          const std::vector<Segment2D>& segments, double offset_x, double offset_y) {
        if (segments.empty()) return;

        for (int py = 0; py < h; ++py) {
            double y_coord = (py + 0.5) * pitch;
            std::vector<double> intersections;

            for (const auto& seg : segments) {
                const Vec2 p1 = { seg.p1.x + offset_x, seg.p1.y + offset_y };
                const Vec2 p2 = { seg.p2.x + offset_x, seg.p2.y + offset_y };

                if ((p1.y <= y_coord && p2.y > y_coord) || (p2.y <= y_coord && p1.y > y_coord)) {
                    if (std::abs(p2.y - p1.y) > 1e-9) {
                        double x_intersect = p1.x + (p2.x - p1.x) * (y_coord - p1.y) / (p2.y - p1.y);
                        intersections.push_back(x_intersect);
                    }
                }
            }

            std::sort(intersections.begin(), intersections.end());

            for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
                int px_start = static_cast<int>(std::round(intersections[i] / pitch));
                int px_end = static_cast<int>(std::round(intersections[i+1] / pitch));

                for (int px = std::max(0, px_start); px < std::min(w, px_end); ++px) {
                    mask[static_cast<size_t>(py) * w + px] = 1;
                }
            }
        }
    }
} // namespace Slicer

/*
 ************************************************************************
 * Generic helpers
 ************************************************************************
 */

// Write a monochrome (1-bit) PNG mask: white = expose, black = off.
inline void writeMonoPNG(const std::filesystem::path& file,
                         int w, int h,
                         const std::vector<uint8_t>& mask)
{
    if (static_cast<std::size_t>(w) * static_cast<std::size_t>(h) != mask.size())
        throw std::runtime_error("mask dimension mismatch while writing " + file.string());

    std::vector<uint8_t> rgba(static_cast<std::size_t>(w) *
                              static_cast<std::size_t>(h) * 4, 0);

    for (std::size_t i = 0; i < mask.size(); ++i)
    {
        const uint8_t v = mask[i] ? 255u : 0u;
        rgba[i * 4 + 0] = v;
        rgba[i * 4 + 1] = v;
        rgba[i * 4 + 2] = v;
        rgba[i * 4 + 3] = 255u;
    }

    const unsigned err = lodepng::encode(file.string(), rgba, w, h);
    if (err)
        throw std::runtime_error("PNG encode error: " +
                                 std::string(lodepng_error_text(err)));
}

// Very small helpers to emit G-code or comments through an output iterator.
template<typename Out>
inline void comment(Out& o, const std::string& t) { *o++ = "; " + t; }

template<typename Out>
inline void cmd(Out& o, const std::string& g)     { *o++ = g; }

// --- Auto-scaling and centering helper ---
template <typename Cfg>
inline double calculateScaleFactor(const Cfg& cfg, const Bounds3D& bb, double pitch, double& build_w, double& build_h) {
    build_w = cfg.cols * pitch;
    build_h = cfg.rows * pitch;

    const double model_w = bb.max_x - bb.min_x;
    const double model_h = bb.max_y - bb.min_y;
    if (model_w < 1e-9 || model_h < 1e-9) return 1.0;

    const double padding_factor = cfg.padding_percentage / 100.0;
    const double printable_w = build_w * (1.0 - 2.0 * padding_factor);
    const double printable_h = build_h * (1.0 - 2.0 * padding_factor);
    if (printable_w <= 0 || printable_h <= 0) return 1.0;

    const double scale_x = printable_w / model_w;
    const double scale_y = printable_h / model_h;

    return std::min(scale_x, scale_y);
}

/*
 ************************************************************************
 * LCD / MSLA specialization
 ************************************************************************
 */

template<IsLCD Cfg, typename Out>
void generateGCode(const std::filesystem::path& stl,
                   const Cfg&                    cfg,
                   Out                           out)
{
    if (cfg.cols <= 0 || cfg.rows <= 0)
        throw std::invalid_argument("LCDConfig.cols/rows must be positive");

    if (cfg.layer_height <= 0)
        throw std::invalid_argument("LCDConfig.layer_height must be positive");

    Bounds3D initial_bb;
    auto triangles = Slicer::readStl(stl, initial_bb);

    const double pitch = 2.0 * cfg.led_radius;

    double build_w, build_h;
    double scale_factor = calculateScaleFactor(cfg, initial_bb, pitch, build_w, build_h);

    Bounds3D scaled_bb = initial_bb;
    if (std::abs(scale_factor - 1.0) > 1e-9) {
        Slicer::Vec3 center = {
            initial_bb.min_x + (initial_bb.max_x - initial_bb.min_x) / 2.0,
            initial_bb.min_y + (initial_bb.max_y - initial_bb.min_y) / 2.0,
            initial_bb.min_z
        };
        const double maxD = std::numeric_limits<double>::max();
        const double minD = std::numeric_limits<double>::lowest();
        scaled_bb = { maxD, maxD, maxD, minD, minD, minD };

        for (auto& tri : triangles) {
            for (auto* v : {&tri.v1, &tri.v2, &tri.v3}) {
                v->x = center.x + (v->x - center.x) * scale_factor;
                v->y = center.y + (v->y - center.y) * scale_factor;

                scaled_bb.min_x = std::min(scaled_bb.min_x, v->x);
                scaled_bb.min_y = std::min(scaled_bb.min_y, v->y);
                scaled_bb.min_z = std::min(scaled_bb.min_z, v->z);
                scaled_bb.max_x = std::max(scaled_bb.max_x, v->x);
                scaled_bb.max_y = std::max(scaled_bb.max_y, v->y);
                scaled_bb.max_z = std::max(scaled_bb.max_z, v->z);
            }
        }
    }

    const int total_layers = static_cast<int>(std::ceil((scaled_bb.max_z - scaled_bb.min_z) / cfg.layer_height));
    const double offset_x = (build_w - (scaled_bb.max_x - scaled_bb.min_x)) / 2.0 - scaled_bb.min_x;
    const double offset_y = (build_h - (scaled_bb.max_y - scaled_bb.min_y)) / 2.0 - scaled_bb.min_y;

    std::filesystem::create_directories(cfg.png_dir);

    // Header
    comment(out, "**** MSLA Print ****");
    cmd(out, "G28");
    cmd(out, "G90");

    const auto layer_png = [&](int idx) -> std::filesystem::path
    {
        std::ostringstream n;
        n << "layer" << std::setw(4) << std::setfill('0') << (idx + 1) << ".png";
        return cfg.png_dir / n.str();
    };

    // Layer loop
    for (int l = 0; l < total_layers; ++l)
    {
        const double z_mm = scaled_bb.min_z + (l + 0.5) * cfg.layer_height;

        const auto segments = Slicer::sliceTriangles(triangles, z_mm);
        if (segments.empty()) {
            comment(out, "Layer " + std::to_string(l + 1) + " is empty, skipping.");
            continue;
        }

        std::ostringstream s_g1;
        s_g1 << "G1 Z" << std::fixed << std::setprecision(4) << (scaled_bb.min_z + (l + 1) * cfg.layer_height) << " F50";
        cmd(out, s_g1.str());

        std::vector<uint8_t> mask(static_cast<size_t>(cfg.cols) * cfg.rows, 0);
        Slicer::rasterizeCenteredSegments(mask, cfg.cols, cfg.rows, pitch, segments, offset_x, offset_y);

        const auto png_path = layer_png(l);
        writeMonoPNG(png_path, cfg.cols, cfg.rows, mask);

        std::ostringstream s_m701;
        s_m701 << "M701 P\"" << png_path.filename().string() << "\" S" << cfg.exposure_s << " I" << cfg.intensity_pct;
        cmd(out, s_m701.str());
    }

    // End / post-lift
    if (cfg.final_lift_mm > 1e-9) {
        const double final_z = scaled_bb.min_z + total_layers * cfg.layer_height;
        std::ostringstream s_lift;
        s_lift << "G1 Z" << (final_z + cfg.final_lift_mm) << " F100";
        cmd(out, s_lift.str());
    }
    cmd(out, "M702");
    cmd(out, "M84");
    cmd(out, "M30");
    comment(out, "PNG layers stored in " + cfg.png_dir.string());
}


/*
 ************************************************************************
 * Laser SLA specialization
 ************************************************************************
 */
namespace { // Anonymous namespace for internal helpers

    // A comparator for map keys to handle floating point inaccuracies for Vec2
    struct Vec2Comparator {
        bool operator()(const Slicer::Vec2& a, const Slicer::Vec2& b) const {
            constexpr double tol = 1e-6;
            if (std::abs(a.x - b.x) > tol) {
                return a.x < b.x;
            }
            if (std::abs(a.y - b.y) > tol) {
                return a.y < b.y;
            }
            return false;
        }
    };

    // Stitches unordered segments into continuous polygons and determines if they are closed.
    inline std::vector<std::pair<std::vector<Slicer::Vec2>, bool>> stitchSegments(const std::vector<Slicer::Segment2D>& segments) {
        if (segments.empty()) return {};

        using AdjacencyList = std::map<Slicer::Vec2, std::list<Slicer::Vec2>, Vec2Comparator>;
        AdjacencyList adj;
        for (const auto& seg : segments) {
            adj[seg.p1].push_back(seg.p2);
            adj[seg.p2].push_back(seg.p1);
        }

        std::vector<std::pair<std::vector<Slicer::Vec2>, bool>> polygons;

        while (!adj.empty()) {
            std::vector<Slicer::Vec2> path;
            Slicer::Vec2 start_node = adj.begin()->first;
            Slicer::Vec2 current_node = start_node;
            path.push_back(current_node);

            bool is_closed = false;
            while(true) {
                if (adj.find(current_node) == adj.end() || adj.at(current_node).empty()) {
                    break; // Path is open
                }

                Slicer::Vec2 next_node = adj.at(current_node).front();

                adj.at(current_node).pop_front();
                auto& neighbors_of_next = adj.at(next_node);
                for (auto it = neighbors_of_next.begin(); it != neighbors_of_next.end(); ++it) {
                    if (Vec2Comparator{}(*it, current_node) == false && Vec2Comparator{}(current_node, *it) == false) {
                        neighbors_of_next.erase(it);
                        break;
                    }
                }

                if (adj.at(current_node).empty()) {
                    adj.erase(current_node);
                }
                if (adj.count(next_node) > 0 && adj.at(next_node).empty()) {
                    adj.erase(next_node);
                }

                if (Vec2Comparator{}(next_node, start_node) == false && Vec2Comparator{}(start_node, next_node) == false) {
                    is_closed = true; // Closed the loop
                    break;
                }

                path.push_back(next_node);
                current_node = next_node;
            }
            polygons.push_back({path, is_closed});
        }
        return polygons;
    }

    // ** CORRECTED HATCHING ALGORITHM **
    // This function now uses the raw segments from the slicer, which is the most
    // robust way to perform a scanline fill.
    template<typename Out>
    void generateSlaHatching(Out& out,
                             const std::vector<Slicer::Segment2D>& segments, // Takes raw segments
                             const SLAConfig& cfg,
                             double rapid_feed,
                             double expose_feed)
    {
        if (segments.empty()) return;

        double hatch_dist = 2.0 * cfg.spot_radius;
        if (hatch_dist <= 1e-9) return;

        double min_y = std::numeric_limits<double>::max();
        double max_y = std::numeric_limits<double>::lowest();
        for (const auto& seg : segments) {
            min_y = std::min({min_y, seg.p1.y, seg.p2.y});
            max_y = std::max({max_y, seg.p1.y, seg.p2.y});
        }

        bool forward = true;
        for (double y = min_y; y <= max_y; y += hatch_dist) {
            std::vector<double> intersections;
            for (const auto& seg : segments) {
                 const Slicer::Vec2& p1 = seg.p1;
                 const Slicer::Vec2& p2 = seg.p2;
                // Robust check for scanline intersection
                if ((p1.y < y && p2.y >= y) || (p2.y < y && p1.y >= y)) {
                    if (std::abs(p2.y - p1.y) > 1e-9) {
                        double x_intersect = p1.x + (p2.x - p1.x) * (y - p1.y) / (p2.y - p1.y);
                        intersections.push_back(x_intersect);
                    }
                }
            }

            std::sort(intersections.begin(), intersections.end());

            if (intersections.size() % 2 != 0) {
                // Odd number of intersections means the scanline is likely grazing a
                // vertex or a meshing error occurred. Skip this line for safety.
                continue;
            }

            auto generate_gcode_line = [&](double x1, double x2) {
                std::ostringstream ss;
                ss << std::fixed << std::setprecision(4);
                ss << "G0 X" << x1 << " Y" << y << " F" << rapid_feed;
                cmd(out, ss.str());
                ss.str(""); ss.clear();
                ss << "G1 X" << x2 << " Y" << y << " F" << expose_feed;
                cmd(out, ss.str());
            };

            if (forward) {
                for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
                    generate_gcode_line(intersections[i], intersections[i+1]);
                }
            } else {
                for (int i = intersections.size() - 2; i >= 0; i -= 2) {
                    generate_gcode_line(intersections[i+1], intersections[i]);
                }
            }
            forward = !forward;
        }
    }
}

template<IsSLA Cfg, typename Out>
void generateGCode(const std::filesystem::path& stl,
                   const Cfg&                    cfg,
                   Out                           out)
{
    if (cfg.layer_height <= 0)
        throw std::invalid_argument("SLAConfig.layer_height must be positive");

    Bounds3D bb;
    auto triangles = Slicer::readStl(stl, bb);
    const int total_layers = static_cast<int>(std::ceil((bb.max_z - bb.min_z) / cfg.layer_height));

    // Header
    comment(out, "**** Laser SLA Print ****");
    cmd(out, "G28");
    cmd(out, "G90");

    const double expose_feed = 150.0;
    const double rapid_feed  = 200.0;

    // Layer loop
    for (int l = 0; l < total_layers; ++l)
    {
        const double z_mm = bb.min_z + (l + 0.5) * cfg.layer_height;

        const auto segments = Slicer::sliceTriangles(triangles, z_mm);
        if (segments.empty()) {
            comment(out, "Layer " + std::to_string(l + 1) + " is empty, skipping.");
            continue;
        }

        const double current_z = bb.min_z + (l + 1) * cfg.layer_height;
        comment(out, "Layer " + std::to_string(l + 1) + " (Z = " + std::to_string(current_z) + " mm)");
        std::ostringstream s_g1;
        s_g1 << "G1 Z" << std::fixed << std::setprecision(4) << current_z << " F60";
        cmd(out, s_g1.str());

        std::ostringstream s_m3;
        s_m3 << "M3 S" << cfg.laser_power_pct;
        cmd(out, s_m3.str());

        // --- Contour Pass ---
        // Use stitched polygons for a clean, continuous contour toolpath.
        auto polygon_data = stitchSegments(segments);
        comment(out, "--- Contour Pass ---");
        for (const auto& data_pair : polygon_data) {
            const auto& poly = data_pair.first;
            if (poly.size() < 2) continue;

            std::ostringstream ss_g0;
            ss_g0 << std::fixed << std::setprecision(4) << "G0 X" << poly.front().x << " Y" << poly.front().y << " F" << rapid_feed;
            cmd(out, ss_g0.str());

            for (size_t i = 1; i < poly.size(); ++i) {
                std::ostringstream ss_g1;
                ss_g1 << std::fixed << std::setprecision(4) << "G1 X" << poly[i].x << " Y" << poly[i].y << " F" << expose_feed;
                cmd(out, ss_g1.str());
            }

            if (data_pair.second) { // is_closed
                std::ostringstream ss_g1_close;
                ss_g1_close << std::fixed << std::setprecision(4) << "G1 X" << poly.front().x << " Y" << poly.front().y << " F" << expose_feed;
                cmd(out, ss_g1_close.str());
            }
        }

        // --- Hatch Pass ---
        // Use the original raw segments for a robust fill, avoiding errors from complex polygons.
        comment(out, "--- Hatch Pass ---");
        generateSlaHatching(out, segments, cfg, rapid_feed, expose_feed);

        cmd(out, "M5"); // laser off
    }

    // End
    if (cfg.final_lift_mm > 1e-9) {
        const double final_z = bb.min_z + total_layers * cfg.layer_height;
        std::ostringstream s_lift;
        s_lift << "G1 Z" << (final_z + cfg.final_lift_mm) << " F200";
        cmd(out, s_lift.str());
    }
    cmd(out, "M84");
    cmd(out, "M30");
}

} // namespace Stratum