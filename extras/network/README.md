# Network tests

No extra packages needed: busybox `nc` on the box, Python on the PC.
Results: [`docs/06-performance.md`](../../docs/06-performance.md#network).

| File | Runs on | What |
|---|---|---|
| `rx.py PORT` | PC | TCP receiver, timed from first to last byte |
| `tx.py HOST PORT MB` | PC | TCP sender; waits until the receiver has everything |
| `udpsend.c HOST PORT SECONDS PAYLOAD` | box | UDP sender, packets per second |
| `udprecv.c PORT IDLE_SECONDS` | PC | UDP receiver, counts what arrived |

```
# box -> PC                                    (PC)  python3 rx.py 5001
(box) head -c 314572800 /dev/zero | nc PC 5001
# PC -> box                                    (box) nc -l -p 5002 > /dev/null
(PC)  python3 tx.py BOX 5002 500
# smallest packets: run 2+ senders in parallel to reach wire speed
(PC)  ./udprecv 5003 3          (box) ./udpsend PC 5003 10 18 & ./udpsend PC 5003 10 18
```

Use `head -c BYTES` (busybox `head` doesn't take `300M`). Don't stop a sender with `timeout` around a
pipe: only the shell gets killed and `nc` keeps sending.

The box's `tx_dropped` counter grows during these tests: the `hisi-femac` driver counts "transmit queue
full, try again" there. Nothing is lost.
