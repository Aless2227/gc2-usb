#include "gc2_transport.hpp"
#include <algorithm>
#include <cstring>

namespace gc2 {
ExchangeResult CommandChannel::exchange(const Command& command,uint8_t* reply,size_t capacity,ReplyPolicy policy) {
    using controllerlib::Status;
    ExchangeResult result{};
    if (!reply || capacity<8 || capacity>80 || !command.size || command.size>command.bytes.size()) {
        result.status=Status::InvalidArgument;return result;
    }
    result.status=out_.Write(command.bytes.data(),command.size);
    if (result.status!=Status::Success) return result;
    for (unsigned read=0;read<MaxReads;++read) {
        // Always receive into a full packet buffer. A queued packet may be larger
        // than the remaining space of the response currently being assembled.
        uint8_t packet[64]{};size_t count=sizeof(packet);
        const auto status=in_.Read(packet,&count,100000);
        if (status==Status::Timeout || status==Status::NoDataAvailable) continue;
        if (status!=Status::Success) {result.status=status;return result;}
        if (!count) continue;
        if (count>sizeof(packet)) {result.status=Status::UnexpectedData;return result;}
        if (!result.received && !reply_matches(command,packet,count)) {
            ++result.discarded;
            result.foreign_header_size=std::min(count,result.last_foreign_header.size());
            std::copy_n(packet,result.foreign_header_size,result.last_foreign_header.begin());
            continue; // Never accept a foreign command's reply as this command's ACK.
        }
        if (count>capacity-result.received) {result.status=Status::UnexpectedData;return result;}
        std::memcpy(reply+result.received,packet,count);result.received+=count;
        if (result.received==capacity || count<sizeof(packet)) {
            result.matched=true;result.status=Status::Success;return result;
        }
    }
    // SDL checks writes but ignores RecvBulkData's result for initialization.
    // Preserve that policy explicitly, without claiming a reply was received.
    result.status=policy==ReplyPolicy::OptionalInitialization?Status::Success:Status::Timeout;
    return result;
}
}
