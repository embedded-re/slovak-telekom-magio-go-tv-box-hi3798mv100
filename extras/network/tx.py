import socket, sys, time
host, port, mb = sys.argv[1], int(sys.argv[2]), int(sys.argv[3])
s = socket.create_connection((host, port)); buf = b"\0" * (1 << 20)
t0 = time.monotonic()
for _ in range(mb): s.sendall(buf)
s.shutdown(socket.SHUT_WR)
while s.recv(65536): pass          # wait until the receiver has read everything and closed
t1 = time.monotonic()
print(f"{mb} MiB in {t1-t0:.2f} s = {mb*1048576*8/(t1-t0)/1e6:.1f} Mbit/s")
