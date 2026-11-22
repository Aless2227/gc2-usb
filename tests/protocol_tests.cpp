#include "gc2_protocol.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>

static unsigned checks=0;
#define REQUIRE(x) do { ++checks; if (!(x)) { std::cerr<<__LINE__<<": " #x "\n";std::exit(1); } } while(0)
static void pack(uint8_t* p,unsigned x,unsigned y) {
    p[0]=uint8_t(x);p[1]=uint8_t((x>>8)|((y&15)<<4));p[2]=uint8_t(y>>4);
}
int main() {
    using namespace gc2;
    Calibration c{{{2048,1500,1500},{2048,1500,1500}},{{2048,1500,1500},{2048,1500,1500}},10,20,true};
    std::array<uint8_t,64> p{};p[0]=5;
    pack(p.data()+11,2048,2048);pack(p.data()+14,2048,2048);p[61]=10;p[62]=20;
    State s{};REQUIRE(decode(p.data(),64,c,s));REQUIRE(s.buttons==0);
    for (auto v:s.sticks) REQUIRE(v==0);
    REQUIRE(s.left_trigger==0);REQUIRE(s.right_trigger==0);
    const struct {unsigned offset,mask;uint32_t button;} cases[]{
        {5,1,Y},{5,2,X},{5,4,B},{5,8,A},{5,64,RClick},{5,128,Z},
        {6,2,Start},{6,16,Home},{6,32,Capture},{6,64,C},
        {7,1,Down},{7,2,Up},{7,4,Right},{7,8,Left},{7,64,LClick},{7,128,ZL}
    };
    for (const auto& v:cases) {
        p[v.offset]=uint8_t(v.mask);REQUIRE(decode(p.data(),64,c,s));REQUIRE(s.buttons==v.button);
        p[v.offset]=0;
    }
    p[5]=0xcf;p[6]=0x72;p[7]=0xcf;
    REQUIRE(decode(p.data(),64,c,s));REQUIRE(s.buttons==0xffff);
    p[5]=p[6]=p[7]=0;
    pack(p.data()+11,548,3548);pack(p.data()+14,3548,548);
    REQUIRE(decode(p.data(),64,c,s));REQUIRE(s.sticks[0]==-32767);REQUIRE(s.sticks[1]==32767);
    REQUIRE(s.sticks[2]==32767);REQUIRE(s.sticks[3]==-32767);
    p[61]=232;p[62]=255;REQUIRE(decode(p.data(),64,c,s));REQUIRE(s.left_trigger==1 && s.right_trigger==1);
    p[61]=0;p[62]=0;REQUIRE(decode(p.data(),64,c,s));REQUIRE(s.left_trigger==0 && s.right_trigger==0);
    for (size_t n=0;n<64;++n) REQUIRE(!decode(p.data(),n,c,s));
    REQUIRE(!decode(nullptr,64,c,s));REQUIRE(!decode(p.data(),65,c,s));
    p[0]=0x30;REQUIRE(!decode(p.data(),64,c,s));p[0]=5;
    uint8_t cal[9]{};pack(cal,2048,2048);pack(cal+3,1500,1400);pack(cal+6,1450,1350);
    StickCalibration stick{};REQUIRE(parse_stick_calibration(cal,9,stick));
    REQUIRE(stick.x.positive==1500 && stick.y.negative==1350);
    REQUIRE(!parse_stick_calibration(cal,8,stick));
    cal[3]=cal[4]=cal[5]=0;REQUIRE(!parse_stick_calibration(cal,9,stick));
    TriggerLatch latch;REQUIRE(!latch.update(0,false));REQUIRE(latch.update(.25f,false));
    REQUIRE(latch.update(.22f,false));REQUIRE(!latch.update(.20f,false));
    REQUIRE(latch.update(0,true));REQUIRE(!latch.update(0,false));
    REQUIRE(!latch.update(std::numeric_limits<float>::quiet_NaN(),false));
    REQUIRE(!latch.update(.5f,false,.1f,.2f));
    auto f=flash_read(0x1fc040);REQUIRE(f.size==16);REQUIRE(f.bytes[0]==2);
    REQUIRE(f.bytes[12]==0x40 && f.bytes[13]==0xc0 && f.bytes[14]==0x1f && f.bytes[15]==0);
    std::array<uint8_t,80> reply{};reply[0]=2;reply[1]=1;reply[3]=1;reply[8]=64;
    reply[12]=0x40;reply[13]=0xc0;reply[14]=0x1f;
    REQUIRE(flash_reply_matches(0x1fc040,reply.data(),reply.size()));
    REQUIRE(!flash_reply_matches(0x13080,reply.data(),reply.size()));
    REQUIRE(!flash_reply_matches(0x1fc040,reply.data(),79));
    reply[1]=0x91;REQUIRE(!reply_matches(f,reply.data(),reply.size()));reply[1]=1;
    reply[8]=0;REQUIRE(!flash_reply_matches(0x1fc040,reply.data(),reply.size()));
    for (const auto& cmd:InitCommands) REQUIRE(cmd.size==size_t(cmd.bytes[5])+8);
    REQUIRE(InitCommands[8].bytes[8]==5);REQUIRE(InitCommands[9].size==16);
    std::mt19937 random(0x2073);
    for (unsigned i=0;i<10000;++i) {
        for (auto& b:p) b=uint8_t(random());p[0]=5;
        REQUIRE(decode(p.data(),64,c,s));
        REQUIRE(s.left_trigger>=0 && s.left_trigger<=1 && s.right_trigger>=0 && s.right_trigger<=1);
    }
    std::cout<<checks<<" checks passed; SYNTHETIC data, NOT hardware compatibility\n";
}
