#!/usr/bin/env python3
"""Patch HiSilicon baseparam (PDM DB): set a key's u32 value inside a given table.
usage: patch_baseparam.py in.img out.img TABLE KEY=VALUE [KEY=VALUE...]"""
import sys, struct
src, dst, table = sys.argv[1:4]
d = bytearray(open(src, 'rb').read())
t = d.find(table.encode() + b'\0'); assert t >= 0, table
nxt = d.find(b'BASE_TABLE_', t + 1); nxt = len(d) if nxt < 0 else nxt
for kv in sys.argv[4:]:
    k, v = kv.split('='); o = d.find(k.encode() + b'\0', t, nxt); assert o >= 0, k
    assert struct.unpack_from('<I', d, o + 32)[0] == 4
    old = struct.unpack_from('<I', d, o + 36)[0]; struct.pack_into('<I', d, o + 36, int(v, 0))
    print(f'{table}.{k}: {old} -> {v}')
open(dst, 'wb').write(d)
