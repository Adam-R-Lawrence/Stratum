#pragma once

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

namespace Stratum {

// ── types ──────────────────────────────────────────────────────────────
using WorkingCurve = std::variant<
        std::vector<std::pair<double, double>>,
        std::function<double(double)>>;

struct LCDConfig {
    int    cols;
    int    rows;
    double led_radius;     // mm
    double optical_power;  // mW/cm2
    double layer_height;   // mm
    double pitch_mm = 0.0; // centre‑to‑centre; 0 → 2*led_radius
};

struct SLAConfig {
    double spot_radius;    // mm
    double optical_power;  // mW/cm2
    double layer_height;   // mm
    double hatch_mm = 0.0; // 0 → 2*spot_radius
};

template<class T>
concept IsLCD = requires(T t) { t.cols; t.rows; t.led_radius; };

template<class T>
concept IsSLA = requires(T t) { t.spot_radius; };

struct Bounds { double min_x, min_y, max_x, max_y; };

// ── helpers ────────────────────────────────────────────────────────────
inline Bounds stlBounds(const std::filesystem::path& p)
{
    std::ifstream f(p);
    if (!f) throw std::runtime_error("Cannot open " + p.string());

    Bounds b {  std::numeric_limits<double>::max(),
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

template<typename Out>
inline void header(Out& o, const std::string& m,double p,double step,double feed)
{
    std::ostringstream s;
    s<<" ; Stratum "<<m; *o++=s.str();
    s.str(""); s<<" ; Power "<<p<<" mW/cm2"; *o++=s.str();
    s.str(""); s<<" ; Step "<<step<<" mm"; *o++=s.str();
    *o++="G21";
    *o++="G90";
}

// ── LCD specialization ────────────────────────────────────────────────
template<IsLCD Cfg, typename Out>
void generateGCode(const std::filesystem::path& stl,
                   const WorkingCurve& wc,
                   const Cfg& cfg,
                   Out out)
{
    const Bounds bb = stlBounds(stl);
    const double pitch = cfg.pitch_mm>0 ? cfg.pitch_mm : 2.0*cfg.led_radius;
    const double exposure  = exposureAt(cfg.led_radius, wc);
    const double feed_rate = exposure>0 ? 1000.0/exposure : 1000.0;

    header(out,"LCD",cfg.optical_power,pitch,feed_rate);
    {
        std::ostringstream z; z<<"G1 Z"<<cfg.layer_height<<" F300"; *out++=z.str();
    }

    for(int r=0;r<cfg.rows;++r){
        double y = r * pitch;
        std::string bits;
        bits.reserve(cfg.cols);
        for(int c=0;c<cfg.cols;++c){
            double x = c * pitch;
            bool inside = (x>=bb.min_x && x<=bb.max_x &&
                           y>=bb.min_y && y<=bb.max_y);
            bits.push_back(inside ? '1' : '0');
        }
        std::ostringstream cm; cm<<";ROW "<<r<<" "<<bits;   *out++=cm.str();
        std::ostringstream cmd; cmd<<"M650 R"<<r<<" B"<<bits; *out++=cmd.str();
    }
    *out++="; End";
}

// ── SLA specialization ────────────────────────────────────────────────
template<IsSLA Cfg, typename Out>
void generateGCode(const std::filesystem::path& stl,
                   const WorkingCurve& wc,
                   const Cfg& cfg,
                   Out out)
{
    const Bounds bb = stlBounds(stl);
    const double step = cfg.hatch_mm>0 ? cfg.hatch_mm : 2.0*cfg.spot_radius;
    const double exposure  = exposureAt(cfg.spot_radius, wc);
    const double feed_rate = exposure>0 ? 1000.0/exposure : 1000.0;

    header(out,"SLA",cfg.optical_power,step,feed_rate);
    {
        std::ostringstream z; z<<"G1 Z"<<cfg.layer_height<<" F300"; *out++=z.str();
    }

    bool fwd=true;
    for(double y=bb.min_y; y<=bb.max_y; y+=step){
        std::ostringstream m, s;
        if(fwd){
            m<<"G0 X"<<bb.min_x<<" Y"<<y<<" F3000";
            s<<"G1 X"<<bb.max_x<<" Y"<<y<<" F"<<feed_rate;
        }else{
            m<<"G0 X"<<bb.max_x<<" Y"<<y<<" F3000";
            s<<"G1 X"<<bb.min_x<<" Y"<<y<<" F"<<feed_rate;
        }
        *out++=m.str();
        *out++=s.str();
        fwd=!fwd;
    }
    *out++="; End";
}

} // namespace Stratum