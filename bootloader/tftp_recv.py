#!/usr/bin/env python3
"""Minimal TFTP server: uploads (WRQ) -> dumps/, downloads (RRQ) <- tftp/ for pulling eMMC dumps from U-Boot.

Binds UDP/69 (needs root), then immediately drops to $SUDO_UID/$SUDO_GID.
Files are written to OUTDIR (default: ../dumps relative to this script).
Handles 16-bit block-number wraparound (files > 32 MB) and duplicate packets.
Prints size + CRC32 of every received file (compare with U-Boot `crc32`).

Usage: sudo python3 bootloader/tftp_recv.py [OUTDIR] [BIND_IP]
"""
import os, socket, struct, sys, time, zlib

HERE = os.path.dirname(os.path.abspath(__file__))
OUTDIR = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else os.path.join(HERE, "..", "dumps"))
BIND = sys.argv[2] if len(sys.argv) > 2 else "0.0.0.0"
RRQ, WRQ, DATA, ACK, ERR = 1, 2, 3, 4, 5

srv = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind((BIND, 69))

uid, gid = int(os.environ.get("SUDO_UID", os.getuid())), int(os.environ.get("SUDO_GID", os.getgid()))
if os.getuid() == 0 and uid != 0:
    os.setgroups([]); os.setgid(gid); os.setuid(uid)
os.makedirs(OUTDIR, exist_ok=True)
print(f"tftp_recv: listening on {BIND}:69, writing to {OUTDIR}, running as uid {os.getuid()}", flush=True)


def err(sock, addr, code, msg):
    sock.sendto(struct.pack("!HH", ERR, code) + msg.encode() + b"\0", addr)


def receive(peer, fname):
    path = os.path.join(OUTDIR, os.path.basename(fname) or "unnamed.bin")
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((BIND, 0))
    s.settimeout(5)
    ack = lambda n: s.sendto(struct.pack("!HH", ACK, n & 0xFFFF), peer)
    crc, total, expect, t0, last_print = 0, 0, 1, time.time(), 0
    with open(path + ".part", "wb") as f:
        ack(0)
        retries = 0
        while True:
            try:
                pkt, addr = s.recvfrom(70000)
            except socket.timeout:
                retries += 1
                if retries > 12:
                    print(f"\n[{fname}] TIMEOUT after {total} bytes — aborted", flush=True)
                    return
                ack(expect - 1)
                continue
            if addr != peer:
                err(s, addr, 5, "unknown TID"); continue
            op, blk = struct.unpack("!HH", pkt[:4])
            if op != DATA:
                print(f"\n[{fname}] unexpected opcode {op}: {pkt[4:]!r}", flush=True); return
            retries = 0
            if blk == (expect & 0xFFFF):
                data = pkt[4:]
                f.write(data); crc = zlib.crc32(data, crc); total += len(data)
                ack(blk); expect += 1
                now = time.time()
                if now - last_print > 2:
                    last_print = now
                    print(f"\r[{fname}] {total/2**20:9.1f} MiB  {total/2**20/(now-t0+1e-9):5.2f} MiB/s",
                          end="", flush=True)
                if len(data) < 512:
                    break
            else:
                ack(blk)  # duplicate of an already-received block
    os.rename(path + ".part", path)
    dt = time.time() - t0
    print(f"\r[{fname}] DONE {total} bytes (0x{total:x}) in {dt:.1f}s, {total/2**20/dt:.2f} MiB/s, "
          f"crc32=0x{crc:08x}", flush=True)


SERVE = os.path.abspath(os.path.join(HERE, "..", "tftp"))


def send(peer, fname):
    path = os.path.join(SERVE, os.path.basename(fname))
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.bind((BIND, 0)); s.settimeout(2)
    if not os.path.isfile(path):
        err(s, peer, 1, "file not found"); print(f"RRQ {fname!r}: not found in {SERVE}", flush=True); return
    t0 = time.time()
    with open(path, "rb") as f:
        blk = 1
        while True:
            data = f.read(512)
            pkt = struct.pack("!HH", DATA, blk & 0xFFFF) + data
            for _ in range(10):
                s.sendto(pkt, peer)
                try:
                    while True:
                        r, addr = s.recvfrom(1024)
                        op, n = struct.unpack("!HH", r[:4])
                        if op == ACK and n == blk & 0xFFFF:
                            break
                        if op == ERR:
                            print(f"RRQ {fname!r}: client error {r[4:]!r}", flush=True); return
                    break
                except socket.timeout:
                    continue
            else:
                print(f"RRQ {fname!r}: timeout at block {blk}", flush=True); return
            if len(data) < 512:
                break
            blk += 1
    print(f"RRQ {fname!r} sent {os.path.getsize(path)} bytes in {time.time()-t0:.1f}s", flush=True)


while True:
    pkt, peer = srv.recvfrom(2048)
    op = struct.unpack("!H", pkt[:2])[0]
    parts = pkt[2:].split(b"\0")
    fname = parts[0].decode(errors="replace")
    if op == WRQ:
        print(f"WRQ {fname!r} from {peer[0]} opts={parts[2:-1]}", flush=True)
        receive(peer, fname)
    elif op == RRQ:
        print(f"RRQ {fname!r} from {peer[0]} opts={parts[2:-1]}", flush=True)
        send(peer, fname)
    else:
        err(srv, peer, 4, "illegal op")
