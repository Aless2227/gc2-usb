#pragma once
#include "gc2_protocol.hpp"
#include "IUSBEndpoint.h"

namespace gc2 {
enum class ReplyPolicy { Required, OptionalInitialization };
struct ExchangeResult {
    controllerlib::Status status{controllerlib::Status::UnknownError};
    size_t received{};
    bool matched{};
    unsigned discarded{};
    std::array<uint8_t,8> last_foreign_header{};
    size_t foreign_header_size{};
};
class CommandChannel {
    controllerlib::IUSBEndpoint& in_;
    controllerlib::IUSBEndpoint& out_;
public:
    // Six bounded reads are a host retry policy, not a Nintendo protocol constant.
    static constexpr unsigned MaxReads=6;
    CommandChannel(controllerlib::IUSBEndpoint& in,controllerlib::IUSBEndpoint& out):in_(in),out_(out) {}
    ExchangeResult exchange(const Command&,uint8_t*,size_t,ReplyPolicy);
};
}
