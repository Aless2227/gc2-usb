#include "gc2_sticks.hpp"
#include <algorithm>
#include <cmath>
#include <string_view>

namespace gc2 {
static std::string_view trim(std::string_view s) {
    const auto first=s.find_first_not_of(" \t\r\n");
    if (first==s.npos) return {};
    const auto last=s.find_last_not_of(" \t\r\n");
    return s.substr(first,last-first+1);
}
ConfigLineResult parse_stick_config_line(const char* text,StickRangeConfig& config) {
    if (!text) return ConfigLineResult::Invalid;
    std::string_view line=text;
    line=trim(line.substr(0,line.find_first_of("#;")));
    if (line.empty() || line.front()=='[') return ConfigLineResult::Ignored;
    const auto equal=line.find('=');
    const auto key=trim(line.substr(0,equal));
    unsigned* setting=nullptr;
    if (key=="left_stick_outer_percent") setting=&config.left_outer_percent;
    else if (key=="right_stick_outer_percent") setting=&config.right_outer_percent;
    else if (key=="stick_diagnostics_seconds") setting=&config.diagnostics_seconds;
    else return ConfigLineResult::Ignored;
    if (equal==line.npos) return ConfigLineResult::Invalid;
    const auto value=trim(line.substr(equal+1));
    if (value.empty()) return ConfigLineResult::Invalid;
    unsigned number=0;
    for (const char ch:value) {
        if (ch<'0' || ch>'9' || number>1000) return ConfigLineResult::Invalid;
        number=number*10+unsigned(ch-'0');
    }
    if (setting==&config.diagnostics_seconds) {
        if (number>120) return ConfigLineResult::Invalid;
    } else if (number<60 || number>100) return ConfigLineResult::Invalid;
    *setting=number;return ConfigLineResult::Applied;
}
std::array<int16_t,2> expand_stick_outer_range(int16_t x,int16_t y,unsigned outer_percent) {
    const std::array<int16_t,2> original{x,y};
    // 100 is an exact bypass, including the original diagonal coordinates.
    if (outer_percent>=100 || outer_percent<60) return original;
    const float fx=float(x)/32767.0f,fy=float(y)/32767.0f;
    const float radius=std::sqrt(fx*fx+fy*fy);
    constexpr float inner=0.5f;
    // Preserve fine movement and inputs that already have full radial travel.
    if (radius<=inner || radius>=1.0f) return original;
    const float outer=float(outer_percent)/100.0f;
    const float expanded=std::min(1.0f,inner+(radius-inner)*(1.0f-inner)/(outer-inner));
    const float scale=expanded/radius;
    auto map=[scale](int16_t v) {
        return static_cast<int16_t>(std::clamp<long>(std::lround(float(v)*scale),-32767,32767));
    };
    return {map(x),map(y)};
}
std::array<int16_t,4> apply_stick_ranges(const std::array<int16_t,4>& s,const StickRangeConfig& config) {
    const auto left=expand_stick_outer_range(s[0],s[1],config.left_outer_percent);
    const auto right=expand_stick_outer_range(s[2],s[3],config.right_outer_percent);
    return {left[0],left[1],right[0],right[1]};
}
void StickRangeStats::observe(const std::array<int16_t,4>& state) {
    if (!samples) minimum=maximum=state;
    else for (unsigned i=0;i<4;++i) {
        minimum[i]=std::min(minimum[i],state[i]);maximum[i]=std::max(maximum[i],state[i]);
    }
    ++samples;
}
}
