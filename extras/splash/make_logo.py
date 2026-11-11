#!/usr/bin/env python3
"""Build a HiSilicon Fastboot 'logo' partition image: 8 KiB PDM DB header (copied from the
original p07_logo.img, LOGO_KEY_LEN patched) + baseline JPEG at 0x2000. No checksum in the format.
usage: make_logo.py original_logo.img splash.jpg out.img"""
import sys, struct
orig, jpg, out = sys.argv[1:4]
hdr = bytearray(open(orig, 'rb').read(0x2000))
data = open(jpg, 'rb').read()
assert hdr[:3] == b'###' and data[:3] == b'\xff\xd8\xff'
# key layout: name (32 bytes) then u32 value-len then value; find LOGO_KEY_LEN and patch its u32 value
i = hdr.find(b'LOGO_KEY_LEN'); assert i > 0
vlen_off = i + 32; assert struct.unpack_from('<I', hdr, vlen_off)[0] == 4
old = struct.unpack_from('<I', hdr, vlen_off + 4)[0]
struct.pack_into('<I', hdr, vlen_off + 4, len(data))
open(out, 'wb').write(bytes(hdr) + data)
print(f'LOGO_KEY_LEN {old} -> {len(data)}; image {0x2000 + len(data)} bytes')
