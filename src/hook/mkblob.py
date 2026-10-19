"""Turn the linked hook ELF into a flat image and patch hook offsets into the header."""
import sys, struct
from elftools.elf.elffile import ELFFile
src, dst = sys.argv[1], sys.argv[2]
HOOKS = ["GetDeviceList", "FreeDeviceList", "GetDeviceDescriptor", "GetBusNumber", "GetDeviceAddress",
         "OpenDeviceWithVidPid", "GetConfigDescriptor", "FreeConfigDescriptor", "KernelDriverActive",
         "DetachKernelDriver", "ClaimInterface", "ResetDevice", "CheckConnected", "GetMaxPacketSize",
         "SubmitTransfer", "HandleEvents", "PadReadState", "FillInterruptTransfer"]
with open(src, "rb") as f:
    e = ELFFile(f)
    for s in e.iter_sections():
        if s.name.startswith(".rela") and s.data_size:
            sys.exit(f"ERROR: dynamic relocations present in {s.name}; image is not position independent")
    sym = {s.name: s["st_value"] for s in e.get_section_by_name(".symtab").iter_symbols()}
    size = sym["_end"]
    img = bytearray(size)
    for s in e.iter_sections():
        if s["sh_flags"] & 2 and s["sh_type"] != "SHT_NOBITS":
            d = s.data(); img[s["sh_addr"]:s["sh_addr"] + len(d)] = d
hdr = sym["HDR"]; assert hdr == 0, "header must be at offset 0"
struct.pack_into("<I", img, 8, size)
for i, h in enumerate(HOOKS):
    struct.pack_into("<I", img, 12 + 4 * i, sym["h_" + h])
struct.pack_into("<I", img, 12 + 4 * len(HOOKS), sym["ld_ping"])
struct.pack_into("<I", img, 16 + 4 * len(HOOKS), sym["h_RenderHook"])
struct.pack_into("<I", img, 20 + 4 * len(HOOKS), sym["h_DCICapture"])
struct.pack_into("<I", img, 24 + 4 * len(HOOKS), sym["h_IconCapture"])
struct.pack_into("<I", img, 28 + 4 * len(HOOKS), sym["h_SelectionCapture"])
open(dst, "wb").write(img)
print(f"{dst}: {size} bytes ({len(HOOKS)} hooks)")
