import socket, sys, time
s = socket.socket(); s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
s.bind(("0.0.0.0", int(sys.argv[1]))); s.listen(1); c, a = s.accept()
n = 0; t0 = None; buf = bytearray(1 << 20)
while True:
    k = c.recv_into(buf)
    if not k: break
    if t0 is None: t0 = time.monotonic()
    n += k; t1 = time.monotonic()
print(f"{n} bytes in {t1-t0:.2f} s = {n*8/(t1-t0)/1e6:.1f} Mbit/s")
