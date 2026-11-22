/* Distribution notice, 2026-11-22: altered sys-con-derived work, GPL version 3.
 * Preserve upstream credits; see LICENSE and THIRD_PARTY.md.
 */
/* gc2-usb: altered sys-con runtime/discovery/HDLS design.
 * Based on cathery/sys-con and o0Zz/sys-con (GPL-3.0); see THIRD_PARTY.md.
 * Experimental: output-range adjustment requires testing on the user's console.
 */
#include <switch.h>
#include "gc2_device.hpp"
#include "gc2_sticks.hpp"
#include "SwitchLogger.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <sys/stat.h>

static HiddbgHdlsSessionId hdls_session{};
alignas(0x1000) static u8 hdls_buffer[0x8000];
static FILE* log_file=nullptr;
static void log_message(const char* level,const char* fmt,va_list ap) {
    if (!log_file) return;
    std::fprintf(log_file,"[%s t=%llu ms] ",level,(unsigned long long)(armTicksToNs(armGetSystemTick())/1000000));
    std::vfprintf(log_file,fmt,ap);
    std::fputc('\n',log_file);std::fflush(log_file);
}
namespace syscon::logger {
// No per-report logging: SD writes in the input loop add latency.
void LogTrace(const char*,...) {}
void LogDebug(const char*,...) {}
void LogPerf(const char*,...) {}
void LogBuffer(controllerlib::LogLevel,const uint8_t*,size_t) {}
#define LOG_FN(name,label) void name(const char* fmt,...) { va_list ap;va_start(ap,fmt);log_message(label,fmt,ap);va_end(ap); }
LOG_FN(LogInfo,"info")
LOG_FN(LogWarning,"warning")
LOG_FN(LogError,"error")
#undef LOG_FN
}
extern "C" {
u32 __nx_applet_type=AppletType_None;
u32 __nx_fs_num_sessions=1;
void __libnx_initheap() {
    alignas(0x1000) static u8 heap[0x60000];
    extern void *fake_heap_start,*fake_heap_end;
    fake_heap_start=heap;fake_heap_end=heap+sizeof(heap);
}
void __appInit() {
    #define CHECK(call) do { Result rc=(call);if (R_FAILED(rc)) diagAbortWithResult(rc); } while(0)
    CHECK(smInitialize());
    CHECK(setsysInitialize());
    SetSysFirmwareVersion fw{};CHECK(setsysGetFirmwareVersion(&fw));
    hosversionSet(MAKEHOSVERSION(fw.major,fw.minor,fw.micro));setsysExit();
    CHECK(fsInitialize());CHECK(fsdevMountSdmc());
    CHECK(usbHsInitialize());CHECK(pscmInitialize());CHECK(hiddbgInitialize());
    CHECK(hiddbgAttachHdlsWorkBuffer(&hdls_session,hdls_buffer,sizeof(hdls_buffer)));
    #undef CHECK
}
void __appExit() {
    if (log_file) { std::fclose(log_file);log_file=nullptr; }
    hiddbgReleaseHdlsWorkBuffer(hdls_session);hiddbgExit();
    pscmExit();usbHsExit();fsdevUnmountAll();fsExit();smExit();
}
}

namespace {
using syscon::logger::LogInfo;
using syscon::logger::LogError;
static u64 millis() { return armTicksToNs(armGetSystemTick())/1000000; }
static void open_log() {
    if (log_file) return;
    mkdir("sdmc:/config",0777);mkdir("sdmc:/config/gc2-usb",0777);
    log_file=std::fopen("sdmc:/config/gc2-usb/log.txt","a");
}
static gc2::StickRangeConfig load_stick_config() {
    gc2::StickRangeConfig config{};
    FILE* file=std::fopen("sdmc:/config/gc2-usb/config.ini","r");
    if (file) {
        char line[256]{};unsigned number=0;
        while (std::fgets(line,sizeof(line),file)) {
            ++number;
            if (gc2::parse_stick_config_line(line,config)==gc2::ConfigLineResult::Invalid)
                LogError("Invalid stick config at line %u; retaining previous/default value",number);
        }
        std::fclose(file);
    }
    LogInfo("Stick outer range: left=%u%% right=%u%% (100=disabled); inner 50%% unchanged; diagnostics=%us",
        config.left_outer_percent,config.right_outer_percent,config.diagnostics_seconds);
    return config;
}
// Reuses sys-con's HDLS path. Pro controller semantics, with digital triggers.
class VirtualPad {
    HiddbgHdlsHandle handle_{};
    gc2::TriggerLatch left_,right_;
    gc2::StickRangeConfig range_config_;
    gc2::StickRangeStats ranges_{};
    u64 range_deadline_{};
    u64 range_last_input_{},range_max_gap_{};
    bool range_logged_{};
    void log_ranges(const char* reason) {
        if (!ranges_.samples || range_logged_) return;
        range_logged_=true;
        LogInfo("Stick measurement (%s), samples=%llu; calibrated values BEFORE compensation",reason,(unsigned long long)ranges_.samples);
        LogInfo("Largest observed gap between inputs during measurement=%llu ms (not a USB polling-rate measurement)",(unsigned long long)range_max_gap_);
        const char* names[]{"LX","LY","CX","CY"};
        for (unsigned i=0;i<4;++i)
            LogInfo("Stick %s observed min=%d max=%d (negative=%.1f%% positive=%.1f%% of calibrated range)",
                names[i],ranges_.minimum[i],ranges_.maximum[i],
                std::max(0,-int(ranges_.minimum[i]))*100.0/32767,
                std::max(0,int(ranges_.maximum[i]))*100.0/32767);
    }
public:
    explicit VirtualPad(const gc2::StickRangeConfig& config):range_config_(config) {}
    void detach() {
        if (range_config_.diagnostics_seconds) log_ranges("session ended");
        if (handle_.handle) { hiddbgDetachHdlsVirtualDevice(handle_);handle_={}; }
        left_.reset();right_.reset();
        ranges_={};range_deadline_=0;range_logged_=false;range_last_input_=range_max_gap_=0;
    }
    bool update(const gc2::State& s) {
        if (range_config_.diagnostics_seconds && !range_logged_) {
            const u64 now=millis();
            if (!range_deadline_) {
                range_deadline_=now+range_config_.diagnostics_seconds*1000;
                LogInfo("Stick range measurement started for %us; rotate sticks and hold full cardinal directions",range_config_.diagnostics_seconds);
            }
            if (range_last_input_) range_max_gap_=std::max(range_max_gap_,now-range_last_input_);
            range_last_input_=now;
            ranges_.observe(s.sticks);
            if (now>=range_deadline_) log_ranges("measurement complete");
        }
        if (!handle_.handle) {
            HiddbgHdlsDeviceInfo info{};
            info.deviceType=HidDeviceType_FullKey3;
            info.npadInterfaceType=HidNpadInterfaceType_USB;
            info.singleColorBody=0x503080ff;info.singleColorButtons=0x303030ff;
            const Result rc=hiddbgAttachHdlsVirtualDevice(&handle_,&info);
            if (R_FAILED(rc)) { LogError("HDLS attach: 0x%08lx",(unsigned long)rc);return false; }
            LogInfo("HDLS virtual Pro pad attached (Smash test still required)");
        }
        HiddbgHdlsState h{};h.battery_level=4;
        const struct {uint32_t from;u64 to;} map[]{
            {gc2::A,HidNpadButton_A},{gc2::B,HidNpadButton_B},{gc2::X,HidNpadButton_X},{gc2::Y,HidNpadButton_Y},
            {gc2::Up,HidNpadButton_Up},{gc2::Down,HidNpadButton_Down},
            {gc2::Left,HidNpadButton_Left},{gc2::Right,HidNpadButton_Right},
            {gc2::ZL,HidNpadButton_ZL},{gc2::Z,HidNpadButton_ZR},
            {gc2::Start,HidNpadButton_Plus},{gc2::C,HidNpadButton_Minus},
            {gc2::Home,HiddbgNpadButton_Home},{gc2::Capture,HiddbgNpadButton_Capture}
        };
        for (const auto& m:map) if (s.buttons&m.from) h.buttons|=m.to;
        if (left_.update(s.left_trigger,s.buttons&gc2::LClick)) h.buttons|=HidNpadButton_L;
        if (right_.update(s.right_trigger,s.buttons&gc2::RClick)) h.buttons|=HidNpadButton_R;
        const auto sticks=gc2::apply_stick_ranges(s.sticks,range_config_);
        h.analog_stick_l={sticks[0],sticks[1]};
        h.analog_stick_r={sticks[2],sticks[3]};
        const Result rc=hiddbgSetHdlsState(handle_,&h);
        if (R_FAILED(rc)) { LogError("HDLS state: 0x%08lx; detaching",(unsigned long)rc);detach();return false; }
        return true;
    }
    void neutral() {
        left_.reset();right_.reset();
        if (handle_.handle) {
            HiddbgHdlsState h{};h.battery_level=4;
            if (R_FAILED(hiddbgSetHdlsState(handle_,&h))) detach();
        }
    }
};
// Keep USB discovery buffers off the small sysmodule main-thread stack.
static UsbHsInterface available[16]{};
static UsbHsInterface all_interfaces[16]{};
static void dump_interfaces(const char* kind,const UsbHsInterface* entries,s32 count) {
    for (s32 i=0;i<count;++i) {
        const auto& a=entries[i];
        LogInfo("USB %s[%ld] id=%ld bus=%lu dev=%lu dev2=%lu if=%u alt=%u class=%02x endpoints=%u path=%.64s",
            kind,(long)i,(long)a.inf.ID,(unsigned long)a.busID,(unsigned long)a.deviceID,
            (unsigned long)a.inf.deviceID_2,a.inf.interface_desc.bInterfaceNumber,
            a.inf.interface_desc.bAlternateSetting,a.inf.interface_desc.bInterfaceClass,
            a.inf.interface_desc.bNumEndpoints,a.pathstr);
        for (unsigned ep=0;ep<15;++ep) {
            const auto& in=a.inf.input_endpoint_descs[ep];
            const auto& out=a.inf.output_endpoint_descs[ep];
            if (in.bLength) LogInfo("  endpoint=%02x attributes=%02x max_packet=%u",in.bEndpointAddress,in.bmAttributes,in.wMaxPacketSize);
            if (out.bLength) LogInfo("  endpoint=%02x attributes=%02x max_packet=%u",out.bEndpointAddress,out.bmAttributes,out.wMaxPacketSize);
        }
    }
}
bool discover(gc2::Device& device) {
    UsbHsInterfaceFilter filter{};
    filter.Flags=UsbHsInterfaceFilterFlags_idVendor|UsbHsInterfaceFilterFlags_idProduct;
    filter.idVendor=gc2::Vendor;filter.idProduct=gc2::Product;
    s32 count=0;
    const Result rc=usbHsQueryAvailableInterfaces(&filter,available,sizeof(available),&count);
    if (R_FAILED(rc)) { LogError("USB query: 0x%08lx",(unsigned long)rc);return false; }
    count=std::clamp<s32>(count,0,16);
    s32 all_count=0;
    const Result all_rc=usbHsQueryAllInterfaces(&filter,all_interfaces,sizeof(all_interfaces),&all_count);
    all_count=std::clamp<s32>(all_count,0,16);
    // QueryAll is diagnostic only: its IDs are -1 and MUST NOT be acquired.
    static u64 next_snapshot=0;
    static s32 previous_available=-1,previous_all=-1;
    const bool snapshot=count!=previous_available || all_count!=previous_all || millis()>=next_snapshot;
    if (snapshot) {
        LogInfo("USB scan 057e:2073 available=%ld visible_to_session=%ld query_all_rc=0x%08lx",(long)count,(long)all_count,(unsigned long)all_rc);
        dump_interfaces("available",available,count);
        if (R_SUCCEEDED(all_rc)) dump_interfaces("all (not acquirable IDs)",all_interfaces,all_count);
        previous_available=count;previous_all=all_count;next_snapshot=millis()+30000;
    }
    for (s32 i=0;i<count;++i) {
        auto& a=available[i];
        if (a.inf.interface_desc.bInterfaceClass!=USB_CLASS_HID || a.inf.interface_desc.bAlternateSetting) continue;
        for (s32 j=0;j<count;++j) {
            auto& b=available[j];
            if (i==j || a.busID!=b.busID || a.deviceID!=b.deviceID ||
                b.inf.interface_desc.bInterfaceNumber!=1 || b.inf.interface_desc.bAlternateSetting) continue;
            LogInfo("Found 057e:2073 bus=%lu device=%lu HID=%u command=%u",
                (unsigned long)a.busID,(unsigned long)a.deviceID,
                a.inf.interface_desc.bInterfaceNumber,b.inf.interface_desc.bInterfaceNumber);
            return device.open(a,b);
        }
    }
    if (snapshot && count) LogInfo("No valid HID0/bulk1 pair; interfaces left untouched");
    return false;
}
}

int main(int,char**) {
    open_log();
    LogInfo("gc2-usb 0.1.3 experimental; HOS %u.%u.%u; output range adjustment pending user validation",
        HOSVER_MAJOR(hosversionGet()),HOSVER_MINOR(hosversionGet()),HOSVER_MICRO(hosversionGet()));
    // Same PSC module slot as sys-con. Must replace, never coexist with sys-con.
    PscPmModule power{};const u32 dependencies[]{PscPmModuleId_Fs};
    Result rc=pscmGetPmModule(&power,PscPmModuleId(126),dependencies,1,true);
    if (R_FAILED(rc)) { LogError("PSC registration: 0x%08lx",(unsigned long)rc);return 1; }
    Event available_event{};
    UsbHsInterfaceFilter event_filter{};
    event_filter.Flags=UsbHsInterfaceFilterFlags_idVendor|UsbHsInterfaceFilterFlags_idProduct|UsbHsInterfaceFilterFlags_bcdDevice_Min;
    event_filter.idVendor=gc2::Vendor;event_filter.idProduct=gc2::Product;
    const Result event_rc=usbHsCreateInterfaceAvailableEvent(&available_event,true,0,&event_filter);
    const bool event_open=R_SUCCEEDED(event_rc);
    LogInfo("USB availability event registration: 0x%08lx",(unsigned long)event_rc);
    const auto stick_config=load_stick_config();
    gc2::Device device;VirtualPad pad(stick_config);
    bool connected=false,awake=true,running=true,neutralized=false,received_input=false;
    u64 last_valid=0,retry_at=0,next_event_retry=0;
    while (running) {
        if (R_SUCCEEDED(eventWait(&power.event,0))) {
            PscPmState state{};u32 flags=0;
            rc=pscPmModuleGetRequest(&power,&state,&flags);
            if (R_FAILED(rc)) { LogError("PSC get request failed");break; }
            if (awake) LogInfo("PSC request state=%u flags=%lu",unsigned(state),(unsigned long)flags);
            if (state==PscPmState_ReadySleep || state==PscPmState_ReadyShutdown) {
                pad.detach();device.close();connected=false;awake=false;
                LogInfo("Power transition: USB and pad released");
                if (log_file) { std::fclose(log_file);log_file=nullptr; }
                if (state==PscPmState_ReadyShutdown) running=false;
            } else if (state==PscPmState_Awake || state==PscPmState_ReadyAwaken) {
                awake=true;retry_at=millis()+500;open_log();LogInfo("Wake request state=%u; rediscovering USB",unsigned(state));
            }
            rc=pscPmModuleAcknowledge(&power,state);
            if (R_FAILED(rc)) break;
        }
        if (!running) break;
        if (!awake) { svcSleepThread(10000000);continue; }
        if (!connected) {
            if (event_open && R_SUCCEEDED(eventWait(&available_event,0)) && millis()>=next_event_retry) {
                LogInfo("USB availability event signaled");retry_at=0;next_event_retry=millis()+2000;
            }
            if (millis()>=retry_at) {
                connected=discover(device);retry_at=millis()+2000;
                last_valid=millis();neutralized=false;received_input=false;
            }
            svcSleepThread(1000000);continue;
        }
        gc2::State s{};
        const auto status=device.read(s,8000);
        if (status==controllerlib::Status::Success) {
            if (!received_input) {
                LogInfo("First valid USB input report 05 received; publishing virtual Pro controller");
                received_input=true;
            }
            last_valid=millis();neutralized=false;
            if (!pad.update(s)) { device.close();connected=false;retry_at=millis()+2000; }
        } else {
            const u64 elapsed=millis()-last_valid;
            if (elapsed>=100 && !neutralized) { pad.neutral();neutralized=true; }
            if (elapsed>=500 || (status!=controllerlib::Status::Timeout &&
                status!=controllerlib::Status::NoDataAvailable && status!=controllerlib::Status::UnexpectedData)) {
                LogInfo("Input unavailable/status=%u; detached and retrying",unsigned(status));
                pad.detach();device.close();connected=false;retry_at=millis()+2000;
            }
        }
    }
    pad.detach();device.close();
    if (event_open) usbHsDestroyInterfaceAvailableEvent(&available_event,0);
    pscPmModuleFinalize(&power);pscPmModuleClose(&power);eventClose(&power.event);
    return 0;
}
