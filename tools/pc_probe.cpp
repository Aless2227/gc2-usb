// Target-specific diagnostic for the user's 057e:2073. No driver installation.
// Sends ONLY the same SDL initialization and flash READ commands as gc2-usb.
#define NOMINMAX
#include <windows.h>
#include <setupapi.h>
#include <winusb.h>
#include <hidsdi.h>
#include "gc2_protocol.hpp"
#include "gc2_transport.hpp"
#include <array>
#include <cstdio>
#include <cwctype>
#include <string>
#include <vector>
#include <chrono>

static FILE* log_file{};
template<typename... T> void log(const char* fmt,T... args) {
    if constexpr(sizeof...(args)==0) std::printf("%s",fmt);else std::printf(fmt,args...);
    std::printf("\n");
    if (log_file) {
        if constexpr(sizeof...(args)==0) std::fprintf(log_file,"%s",fmt);else std::fprintf(log_file,fmt,args...);
        std::fprintf(log_file,"\n");std::fflush(log_file);
    }
}
static std::vector<std::wstring> paths(const GUID& guid) {
    std::vector<std::wstring> found;
    HDEVINFO devices=SetupDiGetClassDevsW(&guid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if (devices==INVALID_HANDLE_VALUE) return found;
    for (DWORD i=0;;++i) {
        SP_DEVICE_INTERFACE_DATA data{};data.cbSize=sizeof(data);
        if (!SetupDiEnumDeviceInterfaces(devices,nullptr,&guid,i,&data)) break;
        DWORD size=0;SetupDiGetDeviceInterfaceDetailW(devices,&data,nullptr,0,&size,nullptr);
        std::vector<unsigned char> buffer(size);
        auto* detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(buffer.data());
        detail->cbSize=sizeof(*detail);
        if (SetupDiGetDeviceInterfaceDetailW(devices,&data,detail,size,nullptr,nullptr)) {
            std::wstring p=detail->DevicePath,lower=p;
            for (auto& ch:lower) ch=std::towlower(ch);
            if (lower.find(L"vid_057e&pid_2073")!=std::wstring::npos) found.push_back(p);
        }
    }
    SetupDiDestroyDeviceInfoList(devices);return found;
}
class CommandPipe {
    HANDLE file_{INVALID_HANDLE_VALUE};WINUSB_INTERFACE_HANDLE usb_{};
    UCHAR in_{},out_{};
    class Endpoint:public controllerlib::IUSBEndpoint {
        WINUSB_INTERFACE_HANDLE usb_;UCHAR pipe_;
    public:
        Endpoint(WINUSB_INTERFACE_HANDLE usb,UCHAR pipe):usb_(usb),pipe_(pipe) {}
        controllerlib::Status Open(int=0) override {return controllerlib::Status::Success;}
        void Close() override {}
        Direction GetDirection() override {return pipe_&0x80?USB_ENDPOINT_IN:USB_ENDPOINT_OUT;}
        EndpointDescriptor* GetDescriptor() override {return nullptr;}
        controllerlib::Status Write(const uint8_t* b,size_t size) override {
            ULONG n=0;
            if (!WinUsb_WritePipe(usb_,pipe_,const_cast<PUCHAR>(b),ULONG(size),&n,nullptr) || n!=size) {
                log("WinUSB write error=%lu bytes=%lu",GetLastError(),n);return controllerlib::Status::WriteFailed;
            }
            return controllerlib::Status::Success;
        }
        controllerlib::Status Read(uint8_t* b,size_t* size,uint64_t timeout_us) override {
            ULONG timeout=ULONG((timeout_us+999)/1000),n=0;
            if (!WinUsb_SetPipePolicy(usb_,pipe_,PIPE_TRANSFER_TIMEOUT,sizeof(timeout),&timeout)) return controllerlib::Status::ReadFailed;
            if (!WinUsb_ReadPipe(usb_,pipe_,b,ULONG(*size),&n,nullptr)) {
                const DWORD error=GetLastError();
                if (error==ERROR_SEM_TIMEOUT || error==ERROR_TIMEOUT) return controllerlib::Status::Timeout;
                log("WinUSB read error=%lu",error);return controllerlib::Status::ReadFailed;
            }
            *size=n;return controllerlib::Status::Success;
        }
    };
public:
    ~CommandPipe() { if (usb_) WinUsb_Free(usb_);if (file_!=INVALID_HANDLE_VALUE) CloseHandle(file_); }
    bool open(const std::wstring& path) {
        file_=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
            nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
        if (file_==INVALID_HANDLE_VALUE) {log("WinUSB CreateFile failed: %lu",GetLastError());return false;}
        if (!WinUsb_Initialize(file_,&usb_)) {log("WinUSB Initialize failed: %lu",GetLastError());return false;}
        USB_INTERFACE_DESCRIPTOR desc{};
        if (!WinUsb_QueryInterfaceSettings(usb_,0,&desc) || desc.bInterfaceNumber!=1) return false;
        for (UCHAR i=0;i<desc.bNumEndpoints;++i) {
            WINUSB_PIPE_INFORMATION ep{};
            if (!WinUsb_QueryPipe(usb_,0,i,&ep)) return false;
            log("Command endpoint %02x type=%u max_packet=%u",ep.PipeId,unsigned(ep.PipeType),ep.MaximumPacketSize);
            if (ep.PipeType==UsbdPipeTypeBulk) {if (ep.PipeId&0x80) in_=ep.PipeId;else out_=ep.PipeId;}
        }
        if (!in_ || !out_) return false;
        ULONG timeout=1000;
        return WinUsb_SetPipePolicy(usb_,in_,PIPE_TRANSFER_TIMEOUT,sizeof(timeout),&timeout) &&
            WinUsb_SetPipePolicy(usb_,out_,PIPE_TRANSFER_TIMEOUT,sizeof(timeout),&timeout);
    }
    bool exchange(const gc2::Command& cmd,unsigned char* reply,size_t capacity,size_t& got,gc2::ReplyPolicy policy=gc2::ReplyPolicy::Required) {
        Endpoint in(usb_,in_),out(usb_,out_);gc2::CommandChannel channel(in,out);
        const auto result=channel.exchange(cmd,reply,capacity,policy);got=result.received;
        log("Command %02x/%02x status=%u matched=%u bytes=%zu discarded=%u",cmd.bytes[0],cmd.bytes[3],unsigned(result.status),unsigned(result.matched),got,result.discarded);
        return result.status==controllerlib::Status::Success;
    }
};
static void describe(const char* name,const gc2::StickCalibration& s) {
    log("%s: center=(%u,%u) positive=(%u,%u) negative=(%u,%u) valid=%u",name,
        s.x.center,s.y.center,s.x.positive,s.y.positive,s.x.negative,s.y.negative,unsigned(gc2::valid(s)));
}
static bool probe() {
    // GUID observed in this controller's installed WinUSB DeviceInterfaceGUID.
    const GUID command_guid={0x6f13725e,0xef0e,0x4fd3,{0xae,0x5f,0xb2,0xde,0x98,0x9e,0xc8,0x25}};
    GUID hid_guid{};HidD_GetHidGuid(&hid_guid);
    auto commands=paths(command_guid),inputs=paths(hid_guid);
    log("Nintendo 057e:2073: command interfaces=%zu HID paths=%zu",commands.size(),inputs.size());
    if (commands.size()!=1 || inputs.size()!=1) {log("Expected exactly one controller");return false;}
    CommandPipe pipe;if (!pipe.open(commands[0])) return false;
    gc2::Calibration cal{};
    for (uint32_t addr:{0x13000u,0x13040u,0x13080u,0x130c0u,0x13100u,0x13140u,0x1fc040u,0x1fc080u}) {
        std::array<unsigned char,80> reply{};size_t n=0;
        if (!pipe.exchange(gc2::flash_read(addr),reply.data(),reply.size(),n) || !gc2::flash_reply_matches(addr,reply.data(),n)) {
            log("Flash READ failed at %06lx",(unsigned long)addr);return false;
        }
        const auto* b=reply.data()+16;
        log("Flash READ %06lx: response length/address OK (serial not logged)",(unsigned long)addr);
        if (addr==0x13080 && !gc2::parse_stick_calibration(b+0x28,24,cal.left)) return false;
        if (addr==0x130c0 && !gc2::parse_stick_calibration(b+0x28,24,cal.right)) return false;
        if (addr==0x13140) {cal.left_trigger_zero=b[0];cal.right_trigger_zero=b[1];cal.triggers_valid=b[0]<232&&b[1]<232;}
        if ((addr==0x1fc040 || addr==0x1fc080) && b[0]==0xb2 && b[1]==0xa1 &&
            !gc2::parse_stick_calibration(b+2,62,addr==0x1fc040?cal.left:cal.right)) return false;
    }
    describe("Left",cal.left);describe("C-stick",cal.right);
    log("Trigger zero: L=%u R=%u valid=%u",cal.left_trigger_zero,cal.right_trigger_zero,unsigned(cal.triggers_valid));
    for (size_t i=0;i<gc2::InitCommands.size();++i) {
        unsigned char reply[64]{};size_t n=0;
        if (!pipe.exchange(gc2::InitCommands[i],reply,sizeof(reply),n,gc2::ReplyPolicy::OptionalInitialization)) {log("Init command %zu write/transport FAILED",i);return false;}
        log("Init command %zu sent (%zu matching reply bytes)",i,n);
    }
    HANDLE hid=CreateFileW(inputs[0].c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
        nullptr,OPEN_EXISTING,FILE_FLAG_OVERLAPPED,nullptr);
    if (hid==INVALID_HANDLE_VALUE) {log("HID open error=%lu",GetLastError());return false;}
    OVERLAPPED ov{};ov.hEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if (!ov.hEvent) {CloseHandle(hid);return false;}
    log("Move sticks and press buttons for 15 seconds. No input is sent to the keyboard or console.");
    auto end=std::chrono::steady_clock::now()+std::chrono::seconds(15);
    size_t valid=0,other=0;gc2::State previous{};
    while (std::chrono::steady_clock::now()<end) {
        unsigned char report[64]{};DWORD n=0;ResetEvent(ov.hEvent);
        BOOL ok=ReadFile(hid,report,sizeof(report),&n,&ov);
        if (!ok && GetLastError()==ERROR_IO_PENDING) {
            if (WaitForSingleObject(ov.hEvent,500)!=WAIT_OBJECT_0) {
                CancelIoEx(hid,&ov);GetOverlappedResult(hid,&ov,&n,TRUE);continue;
            }
            ok=GetOverlappedResult(hid,&ov,&n,TRUE);
        }
        if (!ok) {log("HID read error=%lu",GetLastError());break;}
        gc2::State state{};
        if (!gc2::decode(report,n,cal,state)) {if (++other<4) log("Unexpected HID id=%02x length=%lu",report[0],n);continue;}
        ++valid;
        if (valid==1 || state.buttons!=previous.buttons) {
            log("Input: buttons=%04lx LX=%d LY=%d CX=%d CY=%d L=%.3f R=%.3f",(unsigned long)state.buttons,
                state.sticks[0],state.sticks[1],state.sticks[2],state.sticks[3],state.left_trigger,state.right_trigger);
        }
        previous=state;
    }
    CloseHandle(ov.hEvent);CloseHandle(hid);
    log("Valid common-format USB reports=%zu other=%zu",valid,other);
    log("This PC test does NOT validate Switch USB acquisition or Smash compatibility.");
    return valid>0;
}
int main() {
    wchar_t path[MAX_PATH]{};GetModuleFileNameW(nullptr,path,MAX_PATH);
    std::wstring file=path;auto slash=file.find_last_of(L"\\/");file=file.substr(0,slash+1)+L"gc2-pc-probe.log";
    log_file=_wfopen(file.c_str(),L"w");
    log("gc2-usb 0.1.2 PC diagnostic (USB only; no driver or firmware installation)");
    const bool ok=probe();log("Result: %s",ok?"USB input received on PC":"TEST INCOMPLETE / FAILED; inspect lines above");
    if (log_file) {std::fclose(log_file);log_file=nullptr;}
    std::printf("\nLog saved next to this program. Press Enter to close.\n");std::getchar();
    return ok?0:1;
}
