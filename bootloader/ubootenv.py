#!/usr/bin/env python3
"""Edit a HiSilicon Fastboot env image (non-redundant: crc32 LE + 65532 bytes of key=val\\0).

Usage: ubootenv.py IN OUT [key=value ...] [-key]
  key=value sets/replaces, -key deletes. Prints the resulting env.
Env lives at eMMC offset 0x100000, size 0x10000.
"""
import struct, sys, zlib

SIZE = 0x10000
src, dst, edits = sys.argv[1], sys.argv[2], sys.argv[3:]
d = open(src, "rb").read()
assert len(d) == SIZE
crc = struct.unpack("<I", d[:4])[0]
assert crc == zlib.crc32(d[4:]), "input env CRC mismatch"
env = {}
order = []
for kv in d[4:].split(b"\0\0")[0].split(b"\0"):
    if kv:
        k, v = kv.decode().split("=", 1)
        env[k] = v; order.append(k)
for e in edits:
    if e.startswith("-"):
        env.pop(e[1:], None); order = [k for k in order if k != e[1:]]
    else:
        k, v = e.split("=", 1)
        if k not in env:
            order.append(k)
        env[k] = v
data = b"".join(f"{k}={env[k]}".encode() + b"\0" for k in order) + b"\0"
assert len(data) <= SIZE - 4
data = data.ljust(SIZE - 4, b"\0")
open(dst, "wb").write(struct.pack("<I", zlib.crc32(data)) + data)
for k in order:
    print(f"{k}={env[k]}")
