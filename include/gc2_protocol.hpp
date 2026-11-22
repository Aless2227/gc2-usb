#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// USB protocol adapted from SDL_hidapi_switch2.c; see THIRD_PARTY.md.
// Host independent: no libnx, USB or Bluetooth dependency.
namespace gc2 {
constexpr uint16_t Vendor = 0x057e, Product = 0x2073;
constexpr size_t ReportSize = 64;
enum Button : uint32_t {
    A=1u<<0, B=1u<<1, X=1u<<2, Y=1u<<3,
    Up=1u<<4, Down=1u<<5, Left=1u<<6, Right=1u<<7,
    ZL=1u<<8, Z=1u<<9, LClick=1u<<10, RClick=1u<<11,
    Start=1u<<12, Home=1u<<13, Capture=1u<<14, C=1u<<15
};
struct AxisCalibration { uint16_t center{}, positive{}, negative{}; };
struct StickCalibration { AxisCalibration x{}, y{}; };
struct Calibration {
    StickCalibration left{}, right{};
    uint8_t left_trigger_zero{}, right_trigger_zero{};
    bool triggers_valid{};
};
struct State {
    uint32_t buttons{};
    std::array<int16_t,4> sticks{}; // LX, LY, CX, CY; +Y up (libnx).
    float left_trigger{}, right_trigger{}; // 0 released .. 1 pressed
};
struct Command { std::array<uint8_t,28> bytes{}; size_t size{}; };
extern const std::array<Command,10> InitCommands;
bool valid(const StickCalibration&);
bool parse_stick_calibration(const uint8_t*,size_t,StickCalibration&);
int16_t map_axis(uint16_t,const AxisCalibration&);
bool decode(const uint8_t*,size_t,const Calibration&,State&);
Command flash_read(uint32_t address);
bool reply_matches(const Command&,const uint8_t*,size_t);
bool flash_reply_matches(uint32_t,const uint8_t*,size_t);
class TriggerLatch {
    bool held_{};
public:
    bool update(float value,bool click,float press=0.25f,float release=0.20f);
    void reset() { held_=false; }
};
} // namespace gc2
