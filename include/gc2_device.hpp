#pragma once
#include "gc2_protocol.hpp"
#include "gc2_transport.hpp"
#include "SwitchUSBInterface.h"
#include <memory>

namespace gc2 {
class Device {
    std::unique_ptr<SwitchUSBInterface> input_,commands_;
    controllerlib::IUSBEndpoint *reports_{},*bulk_in_{},*bulk_out_{};
    bool exchange(const Command&,uint8_t*,size_t,size_t&,ReplyPolicy=ReplyPolicy::Required);
    bool flash(uint32_t,std::array<uint8_t,64>&);
public:
    Calibration calibration{};
    ~Device() { close(); }
    bool open(UsbHsInterface&,UsbHsInterface&);
    void close();
    controllerlib::Status read(State&,uint64_t timeout_us);
};
} // namespace gc2
