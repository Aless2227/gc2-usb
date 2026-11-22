/* gc2-usb: altered adaptation of SDL Switch 2 USB logic, prepared 2026-11-22.
 * GameCube-only extraction with local validation/libnx integration.
 * This is not the original SDL source. See THIRD_PARTY.md.
 */
/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2026 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
/* This driver supports the Nintendo Switch Pro controller.
   Code and logic contributed by Valve Corporation under the SDL zlib license.
*/
#include "gc2_protocol.hpp"
#include <algorithm>
#include <cmath>

namespace gc2 {
// Byte-for-byte SDL init sequence, including commands whose purpose is unknown.
// Kept as evidence, NOT reverse-engineered command semantics or ACK definitions.
const std::array<Command,10> InitCommands{{
    {{{0x07,0x91,0,1,0,0,0,0}},8},
    {{{0x0c,0x91,0,2,0,4,0,0,0x27,0,0,0}},12},
    {{{0x11,0x91,0,1,0,0,0,0}},8},
    {{{0x0a,0x91,0,8,0,0x14,0,0,1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
        0xff,0x35,0,0x46,0,0,0,0,0,0,0,0}},28},
    {{{0x0c,0x91,0,4,0,4,0,0,0x27,0,0,0}},12},
    {{{0x01,0x91,0,0x0c,0,0,0,0}},8},
    {{{0x01,0x91,0,1,0,0,0,0}},8},
    {{{0x08,0x91,0,2,0,4,0,0,1,0,0,0}},12},
    {{{0x03,0x91,0,0x0a,0,4,0,0,5,0,0,0}},12},
    {{{0x03,0x91,0,0x0d,0,8,0,0,1,0,0xff,0xff,0xff,0xff,0xff,0xff}},16}
}};
static uint16_t lo12(const uint8_t* p) { return uint16_t(p[0] | ((p[1]&15u)<<8)); }
static uint16_t hi12(const uint8_t* p) { return uint16_t((p[1]>>4) | (p[2]<<4)); }
static bool valid_axis(const AxisCalibration& a) {
    return a.center>0 && a.center<4096 && a.negative>0 && a.positive>0 &&
        a.negative<=a.center && a.positive<=4095-a.center;
}
bool valid(const StickCalibration& s) { return valid_axis(s.x)&&valid_axis(s.y); }
bool parse_stick_calibration(const uint8_t* p,size_t n,StickCalibration& out) {
    if (!p || n<9) return false;
    StickCalibration s{{lo12(p),lo12(p+3),lo12(p+6)},
                       {hi12(p),hi12(p+3),hi12(p+6)}};
    if (!valid(s)) return false;
    out=s;
    return true;
}
int16_t map_axis(uint16_t raw,const AxisCalibration& c) {
    // Deliberately require calibration at runtime; fallback only for host decoder diagnostics.
    const int center=valid_axis(c)?c.center:2048;
    const int d=int(raw)-center;
    const int span=valid_axis(c)?(d<0?c.negative:c.positive):2048;
    return static_cast<int16_t>(std::clamp(d*32767/span,-32767,32767));
}
bool decode(const uint8_t* p,size_t n,const Calibration& c,State& out) {
    // SDL configures report format 5. Reject other IDs and incomplete reports.
    if (!p || n!=ReportSize || p[0]!=0x05) return false;
    State s{};
    const struct {uint8_t byte,mask;uint32_t button;} fields[]{
        {5,1,Y},{5,2,X},{5,4,B},{5,8,A},{5,0x40,RClick},{5,0x80,Z},
        {6,2,Start},{6,0x10,Home},{6,0x20,Capture},{6,0x40,C},
        {7,1,Down},{7,2,Up},{7,4,Right},{7,8,Left},{7,0x40,LClick},{7,0x80,ZL}
    };
    for (const auto& f:fields) if (p[f.byte]&f.mask) s.buttons|=f.button;
    s.sticks={map_axis(lo12(p+11),c.left.x), map_axis(hi12(p+11),c.left.y),
              map_axis(lo12(p+14),c.right.x), map_axis(hi12(p+14),c.right.y)};
    auto trigger=[](uint8_t raw,uint8_t zero) {
        // SDL's observed full-scale value is 232; do not treat it as a USB specification.
        return zero<232 ? std::clamp((float(raw)-zero)/(232.0f-zero),0.0f,1.0f) : 0.0f;
    };
    s.left_trigger=trigger(p[61],c.left_trigger_zero);
    s.right_trigger=trigger(p[62],c.right_trigger_zero);
    out=s;
    return true;
}
Command flash_read(uint32_t address) {
    Command c{{{2,0x91,0,1,0,8,0,0,0,0,0,0,0,0,0,0}},16};
    for (unsigned i=0;i<4;++i) c.bytes[12+i]=uint8_t(address>>(i*8));
    return c;
}
bool reply_matches(const Command& command,const uint8_t* p,size_t n) {
    // Header correlation documented by ndeadly/commands.md, Command Header.
    // Byte 5 (ACK) is deliberately not assigned an invented universal success value.
    return p && n>=8 && p[0]==command.bytes[0] && p[1]==1 &&
        p[2]==0 && p[3]==command.bytes[3];
}
bool flash_reply_matches(uint32_t address,const uint8_t* p,size_t n) {
    if (n!=80 || !reply_matches(flash_read(address),p,n) || p[8]!=64) return false;
    for (unsigned i=0;i<4;++i) if (p[12+i]!=uint8_t(address>>(i*8))) return false;
    return true;
}
bool TriggerLatch::update(float v,bool click,float press,float release) {
    if (!std::isfinite(v) || !(release>=0 && release<press && press<=1)) {
        held_=false;
        return false;
    }
    if (click || v>=press) held_=true;
    else if (v<=release) held_=false;
    return held_;
}
} // namespace gc2
