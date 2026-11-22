"""Capture ONLY Nintendo NSO GameCube 057e:2073 USB. No Bluetooth support.
Default: descriptors only. --capture explicitly runs SDL's read/init sequence.
Requires PyUSB and libusb; on Linux the user must have access to both interfaces.
Legacy diagnostic: this Python tool still requires init replies, unlike the
module/native probe since 0.1.2. It may stop on a controller the module accepts.
"""
import argparse,json,pathlib,re,time
import usb.core,usb.util
root=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser()
p.add_argument('--capture',action='store_true')
p.add_argument('--seconds',type=float,default=10)
p.add_argument('--label',default='unlabelled')
p.add_argument('--out',type=pathlib.Path,required=True)
a=p.parse_args()
devices=list(usb.core.find(find_all=True,idVendor=0x057e,idProduct=0x2073))
if len(devices)!=1:raise SystemExit(f'Expected exactly one 057e:2073; found {len(devices)}')
d=devices[0];cfg=d.get_active_configuration()
topology=[]
for iface in cfg:
    topology.append({'interface':iface.bInterfaceNumber,'alternate':iface.bAlternateSetting,
        'class':iface.bInterfaceClass,'endpoints':[{'address':ep.bEndpointAddress,
        'attributes':ep.bmAttributes,'max_packet':ep.wMaxPacketSize,'interval':ep.bInterval} for ep in iface]})
result={'vid':0x057e,'pid':0x2073,'bcd_device':d.bcdDevice,'label':a.label,
        'topology':topology,'initialization_run':False,'calibration':{},'reports':[]}
def save():
    a.out.parent.mkdir(parents=True,exist_ok=True)
    a.out.write_text(json.dumps(result,indent=2)+'\n')
if not a.capture:save();print(a.out);raise SystemExit(0)
if not 0<a.seconds<=300:raise SystemExit('seconds must be 0 < n <= 300')
hid=next(i for i in cfg if i.bInterfaceNumber==0 and i.bAlternateSetting==0 and i.bInterfaceClass==3)
vendor=next(i for i in cfg if i.bInterfaceNumber==1 and i.bAlternateSetting==0)
def endpoint(iface,direction,kind):
    eps=[e for e in iface if usb.util.endpoint_direction(e.bEndpointAddress)==direction and
         usb.util.endpoint_type(e.bmAttributes)==kind]
    if len(eps)!=1:raise RuntimeError('Unexpected or ambiguous USB topology')
    return eps[0]
reports=endpoint(hid,usb.util.ENDPOINT_IN,usb.util.ENDPOINT_TYPE_INTR)
bulk_in=endpoint(vendor,usb.util.ENDPOINT_IN,usb.util.ENDPOINT_TYPE_BULK)
bulk_out=endpoint(vendor,usb.util.ENDPOINT_OUT,usb.util.ENDPOINT_TYPE_BULK)
detached=[];claimed=[]
try:
    for i in (0,1):
        try:
            if d.is_kernel_driver_active(i):d.detach_kernel_driver(i);detached.append(i)
        except NotImplementedError:pass
        usb.util.claim_interface(d,i);claimed.append(i)
    def exchange(data,expected=64):
        if bulk_out.write(data,timeout=1000)!=len(data):raise RuntimeError('Short USB write')
        b=bytearray()
        while len(b)<expected:
            n=min(64,expected-len(b));part=bulk_in.read(n,timeout=100);b.extend(part)
            if len(part)<n:break
        if len(b)<8 or b[0]!=data[0] or b[1]!=1 or b[2]!=0 or b[3]!=data[3]:
            raise RuntimeError('USB response does not match request')
        return bytes(b)
    for addr in (0x13000,0x13040,0x13080,0x130c0,0x13100,0x13140,0x1fc040,0x1fc080):
        cmd=bytes([2,0x91,0,1,0,8,0,0,0,0,0,0])+addr.to_bytes(4,'little')
        reply=exchange(cmd,80)
        if len(reply)!=80:raise RuntimeError(f'Short calibration reply {addr:#x}')
        if reply[8]!=64 or reply[12:16]!=addr.to_bytes(4,'little'):
            raise RuntimeError('Calibration response address/length mismatch')
        if addr in (0x13080,0x130c0,0x13140,0x1fc040,0x1fc080):
            result['calibration'][hex(addr)]=reply[16:].hex()
    source=(root/'reference/SDL_hidapi_switch2.c').read_text()
    source=source.split('const Uint8 *init_sequence[] = {',1)[1].split('NULL, // Sentinel',1)[0]
    for arr in re.findall(r'\(Uint8\[\]\)\s*\{(.*?)\}',source,re.S):
        arr=re.sub(r'//[^\n]*','',arr)
        command=bytes(int(x,0) for x in re.findall(r'\b(?:0x[0-9a-fA-F]+|[0-9]+)\b',arr))
        if not exchange(command):raise RuntimeError('No initialization response')
    result['initialization_run']=True
    start=time.monotonic()
    while time.monotonic()-start<a.seconds:
        try:b=bytes(reports.read(64,timeout=100))
        except usb.core.USBTimeoutError:continue
        result['reports'].append({'elapsed_s':time.monotonic()-start,'hex':b.hex()})
finally:
    save()
    for i in reversed(claimed):usb.util.release_interface(d,i)
    for i in detached:
        try:d.attach_kernel_driver(i)
        except usb.core.USBError:pass
    usb.util.dispose_resources(d)
print(f"Saved {len(result['reports'])} real reports to {a.out}")
