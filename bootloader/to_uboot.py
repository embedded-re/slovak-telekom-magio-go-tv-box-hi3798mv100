#!/usr/bin/env python3
"""Get the board to the U-Boot `fastboot#` prompt via kstb-bridge.

If Linux is running: log in as root (if at login prompt) and `reboot`.
Then press Enter every 50 ms to interrupt autoboot (bootdelay=1) until the prompt shows.
Power-cycling by hand also works while this runs.
"""
import socket, sys, time

s = socket.socket(socket.AF_UNIX); s.connect("/tmp/kstb.sock"); s.settimeout(0.05)
buf = b""
def rd():
    global buf
    try:
        d = s.recv(65536); buf += d; return d
    except socket.timeout:
        return b""

s.sendall(b"\r"); time.sleep(1); rd()
if buf.rstrip().endswith(b"fastboot#"):
    print("already at fastboot#"); sys.exit(0)
if b"login:" in buf[-200:]:
    s.sendall(b"root\r"); time.sleep(2); rd()
s.sendall(b"reboot\r")
end = time.time() + 120
while time.time() < end:
    rd()
    if b"Bootrom start" in buf or b"Restarting" in buf:
        s.sendall(b"\r")
    if buf.rstrip().endswith(b"fastboot#") and (b"Bootrom start" in buf):
        print("at fastboot#"); sys.exit(0)
print("timeout; last output:\n" + buf[-500:].decode("latin1")); sys.exit(1)
