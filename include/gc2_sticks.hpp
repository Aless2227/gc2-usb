#pragma once
#include <array>
#include <cstdint>

namespace gc2 {
// Output policy, not a Nintendo protocol/calibration value.
struct StickRangeConfig {
    unsigned left_outer_percent{85};
    unsigned right_outer_percent{85};
    unsigned diagnostics_seconds{0};
};
enum class ConfigLineResult { Ignored, Applied, Invalid };
ConfigLineResult parse_stick_config_line(const char*,StickRangeConfig&);
std::array<int16_t,2> expand_stick_outer_range(int16_t x,int16_t y,unsigned outer_percent);
std::array<int16_t,4> apply_stick_ranges(const std::array<int16_t,4>&,const StickRangeConfig&);
struct StickRangeStats {
    uint64_t samples{};
    std::array<int16_t,4> minimum{},maximum{};
    void observe(const std::array<int16_t,4>&);
};
}
