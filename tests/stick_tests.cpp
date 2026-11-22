#include "gc2_sticks.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <random>

static unsigned checks=0;
#define REQUIRE(x) do {++checks;if (!(x)) {std::cerr<<__LINE__<<": " #x "\n";std::exit(1);}} while(0)
int main() {
    using namespace gc2;
    REQUIRE((expand_stick_outer_range(0,0,85)==std::array<int16_t,2>{0,0}));
    for (int x:{0,100,8000,16000,-100,-8000,-16000})
        REQUIRE((expand_stick_outer_range(int16_t(x),0,85)==std::array<int16_t,2>{int16_t(x),0}));
    REQUIRE((expand_stick_outer_range(27853,0,85)==std::array<int16_t,2>{32767,0}));
    REQUIRE((expand_stick_outer_range(-27853,0,85)==std::array<int16_t,2>{-32767,0}));
    REQUIRE((expand_stick_outer_range(0,27853,85)==std::array<int16_t,2>{0,32767}));
    REQUIRE((expand_stick_outer_range(0,-27853,85)==std::array<int16_t,2>{0,-32767}));
    const auto almost=expand_stick_outer_range(26214,0,85); // 80% of calibrated range
    REQUIRE(almost[0]>30000 && almost[0]<32767);REQUIRE(almost[1]==0);
    REQUIRE((expand_stick_outer_range(30000,30000,85)==std::array<int16_t,2>{30000,30000}));
    const auto diagonal=expand_stick_outer_range(20000,20000,85);
    REQUIRE(diagonal[0]==diagonal[1]);REQUIRE(diagonal[0]>20000);
    REQUIRE(std::abs(std::hypot(double(diagonal[0]),double(diagonal[1]))-32767)<2);
    int previous=0;
    for (int x=0;x<=32767;++x) {
        const auto positive=expand_stick_outer_range(int16_t(x),0,85);
        const auto negative=expand_stick_outer_range(int16_t(-x),0,85);
        REQUIRE(positive[0]>=previous);REQUIRE(positive[0]>=x);
        REQUIRE(negative[0]==-positive[0]);REQUIRE(positive[1]==0 && negative[1]==0);
        previous=positive[0];
    }
    std::mt19937 random(0x1303);
    std::uniform_int_distribution<int> axis(-32767,32767);
    for (unsigned i=0;i<10000;++i) {
        const int16_t x=int16_t(axis(random)),y=int16_t(axis(random));
        const auto adjusted=expand_stick_outer_range(x,y,85);
        REQUIRE((expand_stick_outer_range(x,y,100)==std::array<int16_t,2>{x,y}));
        REQUIRE(std::abs(int(adjusted[0]))>=std::abs(int(x)));
        REQUIRE(std::abs(int(adjusted[1]))>=std::abs(int(y)));
        const int64_t cross=int64_t(x)*adjusted[1]-int64_t(y)*adjusted[0];
        REQUIRE(std::abs(cross)<=std::abs(int(x))+std::abs(int(y)));
        if (x) REQUIRE((adjusted[0]>0)==(x>0));
        if (y) REQUIRE((adjusted[1]>0)==(y>0));
    }
    StickRangeConfig config{};
    REQUIRE(config.left_outer_percent==85 && config.right_outer_percent==85);
    REQUIRE(parse_stick_config_line(" left_stick_outer_percent = 90 ; comment",config)==ConfigLineResult::Applied);
    REQUIRE(config.left_outer_percent==90);
    REQUIRE(parse_stick_config_line("right_stick_outer_percent=100",config)==ConfigLineResult::Applied);
    REQUIRE(config.right_outer_percent==100);
    REQUIRE(parse_stick_config_line("stick_diagnostics_seconds=60",config)==ConfigLineResult::Applied);
    REQUIRE(config.diagnostics_seconds==60);
    for (const char* line:{"left_stick_outer_percent=59","left_stick_outer_percent=101",
        "left_stick_outer_percent=-1","left_stick_outer_percent=85garbage",
        "left_stick_outer_percent=99999999999999999999999","left_stick_outer_percent=",
        "stick_diagnostics_seconds=121"}) REQUIRE(parse_stick_config_line(line,config)==ConfigLineResult::Invalid);
    REQUIRE(config.left_outer_percent==90 && config.diagnostics_seconds==60);
    REQUIRE(parse_stick_config_line("[sticks]",config)==ConfigLineResult::Ignored);
    REQUIRE(parse_stick_config_line("# comment",config)==ConfigLineResult::Ignored);
    const std::array<int16_t,4> input{30000,0,30000,0};
    auto adjusted=apply_stick_ranges(input,config);
    REQUIRE(adjusted[0]==32767 && adjusted[1]==0);REQUIRE(adjusted[2]==30000 && adjusted[3]==0);
    config.left_outer_percent=config.right_outer_percent=100;
    REQUIRE(apply_stick_ranges(input,config)==input);
    StickRangeStats stats{};
    stats.observe({200,300,400,500});
    REQUIRE(stats.minimum==stats.maximum);REQUIRE(stats.minimum[0]==200);
    stats.observe({-100,800,-300,200});
    REQUIRE((stats.minimum==std::array<int16_t,4>{-100,300,-300,200}));
    REQUIRE((stats.maximum==std::array<int16_t,4>{200,800,400,500}));
    REQUIRE(stats.samples==2);
    std::cout<<checks<<" stick range checks passed; mathematical/synthetic, user console test required\n";
}
