#pragma once
/*  Stratum – minimal light‑engine toolset
    C++20 ‑ CUDA 12.x compatible (host‑only)

    Dependencies (header‑only, ASCII‑only — add them anywhere in your include path):
      • lodepng.h   – https://github.com/lvandeve/lodepng
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

#include "lodepng.h"   // PNG encoder (header‑only)

namespace Stratum {

// ── types ──────────────────────────────────────────────────────────────
using WorkingCurve = std::variant<
        std::vector<std::pair<double, double>>,
        std::function<double(double)>>;

struct LCDConfig {
    int    cols;                 // pixels in X
    int    rows;                 // pixels in Y
    double led_radius;           // mm
    double optical_power;        // mW ⁄ cm²  (metadata only)
    double layer_height;         // mm
    double pitch_mm   = 0.0;     // centre‑to‑centre; 0 → 2·led_radius
    int    bottom_layers = 4;    // # of initial “burn‑in” layers
    double bottom_exposure = 25; // s
    double normal_exposure =  8; // s
    int    intensity_pct   =100; // 0‑100 for M701 … Ixxx
    double lift_mm         =150; // “out of vat” lift after print
    std::filesystem::path png_dir = "layers";   // where PNGs are stored
};

struct SLAConfig {
    double spot_radius;          // mm
    double optical_power;        // mW ⁄ cm² (metadata only)
    double layer_height;         // mm
    double hatch_mm  = 0.0;      // 0 → 2·spot_radius
    int    layers;               // total layers to generate
    double laser_power_pct =100; // 0‑100 → M3 Sxxx
    double dwell_ms        =1000;
    double lift_mm         = 50; // raise platform after print
};

template<class T>
concept IsLCD = requires(T t) { t.cols; t.rows; t.led_radius; };

template<class T>
concept IsSLA = requires(T t) { t.spot_radius; };

struct Bounds { double min_x, min_y, max_x, max_y; };

// ── helpers ────────────────────────────────────────────────────────────
inline Bounds stlBounds2D(const std::filesystem::path& p)
{
    std::ifstream f(p);
    if (!f) throw std::runtime_error("Cannot open " + p.string());

    Bounds b{  std::numeric_limits<double>::max(),
               std::numeric_limits<double>::max(),
               std::numeric_limits<double>::lowest(),
               std::numeric_limits<double>::lowest() };

    std::string token;
    while (f >> token) {
        if (token == "vertex") {
            double x,y,z; f >> x >> y >> z;
            b.min_x = std::min(b.min_x, x);
            b.min_y = std::min(b.min_y, y);
            b.max_x = std::max(b.max_x, x);
            b.max_y = std::max(b.max_y, y);
        }
    }
    if (b.min_x == std::numeric_limits<double>::max())
        b = {0,0,0,0};
    return b;
}

inline double exposureAt(double d, const WorkingCurve& wc)
{
    return std::visit([d](auto&& arg)->double {
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T,std::vector<std::pair<double,double>>>){
            const auto& v = arg;
            if (v.empty()) return 0.0;
            if (d<=v.front().first) return v.front().second;
            for(std::size_t i=1;i<v.size();++i){
                if (d<=v[i].first){
                    auto [d1,e1]=v[i-1]; auto [d2,e2]=v[i];
                    return e1+(e2-e1)*(d-d1)/(d2-d1);
                }
            }
            return v.back().second;
        } else { return arg(d); }
    }, wc);
}

// Write a 1‑bit mono PNG mask (white = expose, black = off)
inline void writeMonoPNG(const std::filesystem::path& file,
                         int w,int h,
                         const std::vector<uint8_t>& mask)       // mask.size()==w*h, 0|1
{
    std::vector<uint8_t> rgba(w*h*4, 0);
    for(int i=0;i<w*h;++i){
        uint8_t v = mask[i] ? 255 : 0;
        rgba[i*4+0]=v; rgba[i*4+1]=v; rgba[i*4+2]=v; rgba[i*4+3]=255;
    }
    unsigned err = lodepng::encode(file.string(), rgba, w, h);
    if(err) throw std::runtime_error("PNG encode error: "+std::string(lodepng_error_text(err)));
}

// ── generic G‑code helpers ─────────────────────────────────────────────
template<typename Out>
inline void comment(Out& o, const std::string& text) { *o++ = "; " + text; }

template<typename Out>
inline void cmd(Out& o, const std::string& g) { *o++ = g; }

// ── LCD / MSLA specialization ─────────────────────────────────────────
template<IsLCD Cfg, typename Out>
void generateGCode(const std::filesystem::path& stl,
                   const WorkingCurve& wc,
                   const Cfg& cfg,
                   Out out)
{
    const Bounds bb   = stlBounds2D(stl);
    const double pitch= cfg.pitch_mm>0 ? cfg.pitch_mm : 2.0*cfg.led_radius;
    const int    total_layers = cfg.bottom_layers +  // crude (Z‑extent unknown)
                                static_cast<int>(std::ceil((bb.max_y-bb.min_y) / cfg.layer_height));

    // Create directory for PNG masks
    std::filesystem::create_directories(cfg.png_dir);

    // ─ Header
    comment(out,"**** MSLA Print ****");
    {
        std::ostringstream s; s<<"Equal "<<cfg.layer_height<<" mm layer thickness, no heat commands"; comment(out,s.str());
    }
    cmd(out,"G28 Z");
    cmd(out,"G90");

    auto layer_png = [&](int idx)->std::filesystem::path{
        std::ostringstream n; n<<"layer"<<std::setw(3)<<std::setfill('0')<<(idx+1)<<".png";
        return cfg.png_dir / n.str();
    };

    // ─ Layer loop
    for(int l=0;l<total_layers;++l){
        double z = (l+1)*cfg.layer_height;
        {
            std::ostringstream s; s<<"G1 Z"<<std::fixed<<std::setprecision(2)<<z<<" F50";
            cmd(out,s.str());
        }

        // simple rectangular bitmap (white inside model’s XY bounds)
        std::vector<uint8_t> mask(cfg.cols*cfg.rows, 0);
        for(int y=0;y<cfg.rows;++y){
            double py = y * pitch;
            if(py<bb.min_y || py>bb.max_y) continue;
            for(int x=0;x<cfg.cols;++x){
                double px = x * pitch;
                if(px>=bb.min_x && px<=bb.max_x) mask[y*cfg.cols+x]=1;
            }
        }
        auto png_path = layer_png(l);
        writeMonoPNG(png_path, cfg.cols, cfg.rows, mask);

        double exp = (l<cfg.bottom_layers) ? cfg.bottom_exposure
                                           : cfg.normal_exposure;

        std::ostringstream g;
        g<<"M701 P\""<<png_path.filename().string()
         <<"\" S"<<exp<<" I"<<cfg.intensity_pct;
        cmd(out,g.str());

        if(l==cfg.bottom_layers-1 && cfg.bottom_layers>0)
            comment(out,"Bottom layers completed");
    }

    // ─ End
    {
        std::ostringstream s; s<<"G1 Z"<<cfg.lift_mm<<" F100"; cmd(out,s.str());
    }
    cmd(out,"M702");
    cmd(out,"M84");
    cmd(out,"M30");

    comment(out,"PNG layers stored in "+cfg.png_dir.string());
}

// ── Laser‑SLA specialization ───────────────────────────────────────────
template<IsSLA Cfg, typename Out>
void generateGCode(const std::filesystem::path& stl,
                   const WorkingCurve& wc,
                   const Cfg& cfg,
                   Out out)
{
    const Bounds bb = stlBounds2D(stl);
    const double step = cfg.hatch_mm>0 ? cfg.hatch_mm : 2.0*cfg.spot_radius;

    // ─ Header
    comment(out,"**** Laser SLA Print ****");
    {
        std::ostringstream s; s<<"Equal "<<cfg.layer_height<<" mm layer thickness, no heat commands"; comment(out,s.str());
    }
    cmd(out,"G28 X Y Z");
    cmd(out,"G90");

    // Pre‑compute motion feed rates
    double expose_feed = 150;   // mm/min → example
    double rapid_feed  = 200;   // mm/min

    // ─ Layer loop
    for(int l=0;l<cfg.layers;++l){
        double z = (l+1)*cfg.layer_height;
        {
            std::ostringstream s; s<<" ; Layer "<<(l+1)<<" (Z = "<<std::fixed<<std::setprecision(2)<<z<<" mm)";
            cmd(out,s.str());
        }
        {
            std::ostringstream s; s<<"G1 Z"<<z<<" F60"; cmd(out,s.str());
        }
        {
            std::ostringstream s; s<<"M3 S"<<cfg.laser_power_pct; cmd(out,s.str());
        }

        // simple rectangular outline identical to sample
        {
            std::ostringstream s; s<<"G1 X"<<bb.min_x<<" Y"<<bb.min_y<<" F"<<rapid_feed;
            cmd(out,s.str());
        }
        {
            std::ostringstream s; s<<"G1 X"<<bb.max_x<<" Y"<<bb.min_y<<" F"<<expose_feed; cmd(out,s.str());
        }
        {
            std::ostringstream s; s<<"G1 X"<<bb.max_x<<" Y"<<bb.max_y<<" F"<<expose_feed; cmd(out,s.str());
        }
        {
            std::ostringstream s; s<<"G1 X"<<bb.min_x<<" Y"<<bb.max_y<<" F"<<expose_feed; cmd(out,s.str());
        }
        {
            std::ostringstream s; s<<"G1 X"<<bb.min_x<<" Y"<<bb.min_y<<" F"<<expose_feed; cmd(out,s.str());
        }

        cmd(out,"M5");
        {
            std::ostringstream s; s<<"G4 P"<<static_cast<int>(cfg.dwell_ms); cmd(out,s.str());
        }
    }

    // ─ End
    {
        std::ostringstream s; s<<"G1 Z"<<cfg.lift_mm<<" F200"; cmd(out,s.str());
    }
    cmd(out,"M84");
    cmd(out,"M30");
}

} // namespace Stratum
