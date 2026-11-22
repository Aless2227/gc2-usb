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
#include "gc2_device.hpp"
#include "SwitchLogger.h"
#include <algorithm>
#include <cstring>

using controllerlib::IUSBEndpoint;
using controllerlib::Status;
namespace gc2 {
static IUSBEndpoint* endpoint(SwitchUSBInterface& iface,IUSBEndpoint::Direction dir,uint8_t type) {
    IUSBEndpoint* found=nullptr;
    for (unsigned i=0;i<15;++i) {
        auto* ep=iface.GetEndpoint(dir,i);
        if (!ep || (ep->GetDescriptor()->bmAttributes&3)!=type) continue;
        if (found) return nullptr; // ambiguous topology: require a capture, don't guess
        found=ep;
    }
    return found;
}
void Device::close() {
    reports_=bulk_in_=bulk_out_=nullptr;
    if (commands_) { commands_->Close(); commands_.reset(); }
    if (input_) { input_->Close(); input_.reset(); }
    calibration={};
}
bool Device::exchange(const Command& cmd,uint8_t* reply,size_t capacity,size_t& received,ReplyPolicy policy) {
    CommandChannel channel(*bulk_in_,*bulk_out_);
    const auto result=channel.exchange(cmd,reply,capacity,policy);
    received=result.received;
    if (result.discarded) {
        const auto& h=result.last_foreign_header;
        syscon::logger::LogWarning("Command %02x/%02x skipped %u unrelated reply packet(s); last header length=%u bytes=%02x %02x %02x %02x %02x %02x %02x %02x",
            cmd.bytes[0],cmd.bytes[3],result.discarded,unsigned(result.foreign_header_size),h[0],h[1],h[2],h[3],h[4],h[5],h[6],h[7]);
    }
    if (result.status!=Status::Success) {
        syscon::logger::LogError("Command %02x/%02x failed status=%u bytes=%u",cmd.bytes[0],cmd.bytes[3],unsigned(result.status),unsigned(received));return false;
    }
    if (policy==ReplyPolicy::OptionalInitialization) {
        syscon::logger::LogInfo("Init %02x/%02x written; matching reply=%s bytes=%u",cmd.bytes[0],cmd.bytes[3],result.matched?"yes":"not received (SDL policy)",unsigned(received));
    }
    return true;
}
bool Device::flash(uint32_t address,std::array<uint8_t,64>& out) {
    uint8_t reply[80]{};
    size_t n=0;
    if (!exchange(flash_read(address),reply,sizeof(reply),n) || !flash_reply_matches(address,reply,n)) return false;
    // SDL copies the 64-byte payload after the 16-byte reply header.
    // Validate documented command/address/length fields; do not invent an ACK byte.
    std::copy(reply+16,reply+80,out.begin());
    return true;
}
bool Device::open(UsbHsInterface& input,UsbHsInterface& commands) {
    close();
    if (input.device_desc.idVendor!=Vendor || input.device_desc.idProduct!=Product ||
        commands.device_desc.idVendor!=Vendor || commands.device_desc.idProduct!=Product ||
        input.busID!=commands.busID || input.deviceID!=commands.deviceID ||
        input.inf.interface_desc.bInterfaceClass!=USB_CLASS_HID ||
        input.inf.interface_desc.bInterfaceNumber!=0 ||
        commands.inf.interface_desc.bInterfaceNumber!=1 ||
        input.inf.interface_desc.bAlternateSetting || commands.inf.interface_desc.bAlternateSetting)
        return false;
    input_=std::make_unique<SwitchUSBInterface>(input);
    commands_=std::make_unique<SwitchUSBInterface>(commands);
    if (input_->Open()!=Status::Success || commands_->Open()!=Status::Success) { close();return false; }
    reports_=endpoint(*input_,IUSBEndpoint::USB_ENDPOINT_IN,3);
    bulk_in_=endpoint(*commands_,IUSBEndpoint::USB_ENDPOINT_IN,2);
    bulk_out_=endpoint(*commands_,IUSBEndpoint::USB_ENDPOINT_OUT,2);
    if (!reports_ || !bulk_in_ || !bulk_out_ || reports_->GetDescriptor()->wMaxPacketSize!=64 ||
        bulk_in_->GetDescriptor()->wMaxPacketSize<64 || bulk_in_->GetDescriptor()->wMaxPacketSize>512 ||
        bulk_out_->GetDescriptor()->wMaxPacketSize>512) {
        syscon::logger::LogError("Unsupported or ambiguous USB endpoint topology");close();return false;
    }
    syscon::logger::LogInfo("Acquired HID and command interfaces; opening endpoints");
    // Bulk replies are 64-byte chunks regardless of endpoint's maximum packet size.
    // Arm the HID IN pipe before starting controller output (sys-con's strategy).
    if (reports_->Open()!=Status::Success || bulk_in_->Open(64)!=Status::Success ||
        bulk_out_->Open()!=Status::Success) { close();return false; }
    std::array<uint8_t,64> b{};
    // Match SDL's read order. Serial/IMU values are not used or logged.
    const uint32_t addresses[]{0x13000,0x13040,0x13080,0x130c0,0x13100,0x13140,0x1fc040,0x1fc080};
    for (uint32_t a:addresses) {
        if (!flash(a,b)) {
            syscon::logger::LogError("Calibration read failed: 0x%06lx",(unsigned long)a);
            close();return false;
        }
        if (a==0x13080 && !parse_stick_calibration(b.data()+0x28,24,calibration.left)) { syscon::logger::LogError("Invalid factory left stick calibration");close();return false; }
        if (a==0x130c0 && !parse_stick_calibration(b.data()+0x28,24,calibration.right)) { syscon::logger::LogError("Invalid factory right stick calibration");close();return false; }
        if (a==0x13140) {
            calibration.left_trigger_zero=b[0];calibration.right_trigger_zero=b[1];
            calibration.triggers_valid=b[0]<232 && b[1]<232;
        }
        if ((a==0x1fc040 || a==0x1fc080) && b[0]==0xb2 && b[1]==0xa1) {
            auto& stick=a==0x1fc040?calibration.left:calibration.right;
            if (!parse_stick_calibration(b.data()+2,62,stick)) { close();return false; }
        }
    }
    if (!valid(calibration.left)||!valid(calibration.right)||!calibration.triggers_valid) { close();return false; }
    for (size_t i=0;i<InitCommands.size();++i) {
        size_t n=0;uint8_t reply[64]{};
        if (!exchange(InitCommands[i],reply,sizeof(reply),n,ReplyPolicy::OptionalInitialization)) {
            syscon::logger::LogError("USB initialization failed at command %u",unsigned(i));
            close();return false;
        }
    }
    syscon::logger::LogInfo("USB initialization sequence sent; waiting for first valid report 05 (input NOT yet confirmed)");
    return true;
}
Status Device::read(State& state,uint64_t timeout_us) {
    if (!reports_) return Status::InvalidEndpoint;
    uint8_t b[64]{};size_t n=sizeof(b);
    const auto rc=reports_->Read(b,&n,timeout_us);
    if (rc!=Status::Success) return rc;
    if (!decode(b,n,calibration,state)) {
        static u64 last_log=0;
        const u64 now=armTicksToNs(armGetSystemTick())/1000000;
        if (!last_log || now-last_log>=1000) {
            syscon::logger::LogError("Unexpected input report id=%02x length=%u",b[0],unsigned(n));last_log=now;
        }
        return Status::UnexpectedData;
    }
    return Status::Success;
}
} // namespace gc2
