#include "gc2_transport.hpp"
#include <algorithm>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <vector>

using controllerlib::Status;
static unsigned checks=0;
#define REQUIRE(x) do {++checks;if (!(x)) {std::cerr<<__LINE__<<": " #x "\n";std::exit(1);}} while(0)
class FakeEndpoint:public controllerlib::IUSBEndpoint {
public:
    struct ReadStep {Status status;std::vector<uint8_t> packet;};
    std::deque<ReadStep> reads;
    std::vector<std::vector<uint8_t>> writes;
    unsigned read_count{};
    Status write_status{Status::Success};
    Status Open(int=0) override {return Status::Success;}
    void Close() override {}
    Status Write(const uint8_t* b,size_t n) override {writes.emplace_back(b,b+n);return write_status;}
    Status Read(uint8_t* b,size_t* n,uint64_t timeout) override {
        ++read_count;REQUIRE(timeout==100000);
        if (reads.empty()) return Status::Timeout;
        auto step=reads.front();reads.pop_front();
        if (step.status!=Status::Success) return step.status;
        REQUIRE(step.packet.size()<=*n);
        std::copy(step.packet.begin(),step.packet.end(),b);*n=step.packet.size();return Status::Success;
    }
    Direction GetDirection() override {return USB_ENDPOINT_IN;}
    EndpointDescriptor* GetDescriptor() override {return nullptr;}
};
static std::vector<uint8_t> header(const gc2::Command& cmd,size_t size=8) {
    std::vector<uint8_t> b(size);b[0]=cmd.bytes[0];b[1]=1;b[2]=0;b[3]=cmd.bytes[3];return b;
}
int main() {
    using namespace gc2;
    uint8_t reply[80]{};
    const auto cmd=InitCommands[1]; // 0c/02: the user's log showed a timeout here.
    {
        FakeEndpoint in,out;CommandChannel channel(in,out);
        auto r=channel.exchange(cmd,reply,64,ReplyPolicy::OptionalInitialization);
        REQUIRE(r.status==Status::Success);REQUIRE(!r.matched);REQUIRE(r.received==0);
        REQUIRE(in.read_count==CommandChannel::MaxReads);REQUIRE(out.writes.size()==1);
    }
    {
        FakeEndpoint in,out;CommandChannel channel(in,out);
        auto r=channel.exchange(cmd,reply,64,ReplyPolicy::Required);
        REQUIRE(r.status==Status::Timeout);REQUIRE(!r.matched);
    }
    {
        // Synthetic unrelated 8-byte reply: the actual mismatched bytes were NOT logged.
        FakeEndpoint in,out;CommandChannel channel(in,out);
        in.reads.push_back({Status::Success,header(InitCommands[2])});
        in.reads.push_back({Status::Success,header(cmd)});
        auto r=channel.exchange(cmd,reply,64,ReplyPolicy::Required);
        REQUIRE(r.status==Status::Success && r.matched && r.received==8);
        REQUIRE(r.discarded==1);REQUIRE(r.last_foreign_header[0]==0x11);
        REQUIRE(reply[0]==0x0c);REQUIRE(reply[3]==2);REQUIRE(in.read_count==2);
    }
    {
        FakeEndpoint in,out;CommandChannel channel(in,out);
        in.reads.push_back({Status::Success,header(InitCommands[2])});
        auto r=channel.exchange(InitCommands[3],reply,64,ReplyPolicy::OptionalInitialization);
        REQUIRE(r.status==Status::Success && !r.matched);REQUIRE(r.discarded==1);REQUIRE(r.received==0);
    }
    {
        FakeEndpoint in,out;CommandChannel channel(in,out);
        const auto flash=flash_read(0x13080);
        auto b=header(flash,80);b[8]=64;b[12]=0x80;b[13]=0x30;b[14]=1;
        for (size_t i=16;i<b.size();++i) b[i]=uint8_t(i);
        in.reads.push_back({Status::Success,header(InitCommands[3])});
        in.reads.push_back({Status::Timeout,{}});
        in.reads.push_back({Status::Success,{b.begin(),b.begin()+64}});
        in.reads.push_back({Status::Success,{b.begin()+64,b.end()}});
        auto r=channel.exchange(flash,reply,80,ReplyPolicy::Required);
        REQUIRE(r.status==Status::Success && r.matched && r.received==80);
        REQUIRE(r.discarded==1);REQUIRE(std::equal(b.begin(),b.end(),reply));
        REQUIRE(flash_reply_matches(0x13080,reply,r.received));
    }
    {
        FakeEndpoint in,out;CommandChannel channel(in,out);
        out.write_status=Status::WriteFailed;
        auto r=channel.exchange(cmd,reply,64,ReplyPolicy::OptionalInitialization);
        REQUIRE(r.status==Status::WriteFailed);REQUIRE(!r.matched);REQUIRE(in.read_count==0);
    }
    {
        FakeEndpoint in,out;CommandChannel channel(in,out);
        in.reads.push_back({Status::ReadFailed,{}});
        auto r=channel.exchange(cmd,reply,64,ReplyPolicy::OptionalInitialization);
        REQUIRE(r.status==Status::ReadFailed);REQUIRE(!r.matched);
    }
    {
        FakeEndpoint in,out;CommandChannel channel(in,out);
        auto flash=flash_read(0x13080);auto b=header(flash,64);
        in.reads.push_back({Status::Success,b});
        in.reads.push_back({Status::Success,std::vector<uint8_t>(64,0xab)});
        std::array<uint8_t,82> guarded{};guarded.front()=0x12;guarded.back()=0x34;
        auto r=channel.exchange(flash,guarded.data()+1,80,ReplyPolicy::Required);
        REQUIRE(r.status==Status::UnexpectedData);REQUIRE(!r.matched);
        REQUIRE(guarded.front()==0x12 && guarded.back()==0x34);
    }
    {
        FakeEndpoint in,out;CommandChannel channel(in,out);
        auto r=channel.exchange(cmd,nullptr,64,ReplyPolicy::Required);
        REQUIRE(r.status==Status::InvalidArgument);REQUIRE(out.writes.empty());
    }
    {
        FakeEndpoint in,out;CommandChannel channel(in,out);
        for (unsigned i=0;i<100;++i) in.reads.push_back({Status::Success,header(InitCommands[2])});
        auto r=channel.exchange(cmd,reply,64,ReplyPolicy::Required);
        REQUIRE(r.status==Status::Timeout);REQUIRE(!r.matched);
        REQUIRE(in.read_count==CommandChannel::MaxReads);REQUIRE(r.discarded==CommandChannel::MaxReads);
    }
    std::cout<<checks<<" transport checks passed; simulated endpoints, NOT hardware\n";
}
