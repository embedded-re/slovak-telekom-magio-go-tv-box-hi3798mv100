# Magio GO TV Box (Hi3798MV100, G6O110SVT): hardware reference

Everything here was verified on the board with mainline Linux 6.6, unless marked otherwise. Sources: the
vendor SDK (HiSTBLinuxV100R005, GPL: kernel 3.18, Fastboot, device tree), the stock 3.10 kernel binary,
and register probing with `devmem`.

**Interrupt numbers:** the vendor lists raw GIC numbers; a device tree `GIC_SPI` number is that minus 32.

## SoC / boot

- HiSilicon Hi3798MV100, SC_SYSID0 = `0x37980100`; 4× Cortex-A7, VFPv4 + NEON
- 1 GB DDR3 at physical `0x0`; eMMC "4FPD3R" 3.64 GiB (MMC 5.0, 8-bit)
- HiSilicon Fastboot 3.3.0 (U-Boot derivative); settings at eMMC `0x100000`, 64 KiB, CRC32 + `key=value`
  ([`bootloader/ubootenv.py`](../bootloader/ubootenv.py)); no autoboot interrupt, stops only if `bootcmd` is empty
- Kernel: uImage load/entry **`0x02000000`** (loaded at `0x04000000`; entry `0x04000000` crashes the decompressor)
- Partitions only via `blkdevparts=` on the kernel command line (no GPT/MBR)
- No battery-backed RTC
- Reset: write `0xdeadbeef` to `0xf8000004` (SC_SYSRES)

## Address map

| Block | Address | IRQ (SPI) | Notes |
|---|---|---|---|
| GIC distributor / CPU interface | `0xf8a01000` / `0xf8a02000` | | |
| UART0 (PL011, console) | `0xf8b00000` | 49 | `reg-io-width = 4`, clock 83.3 MHz |
| Timers (SP804-style) | `0xf8a29000` (timer2), `0xf8a2a000`, `0xf8a2b000` | 26, 59, 27, 60 | 24 MHz; the ARM architected timer is unusable |
| Watchdog WDG0 (SP805) | `0xf8a2c000` | | 24 MHz, PrimeCell ID regs read 0 |
| CRG (clocks/resets) | `0xf8a22000` | | |
| PERI_CTRL | `0xf8a20000` | | |
| PMC (T-sensor, voltages, GPU power) | `0xf8a23000` | | |
| System controller | `0xf8000000` | | |
| eMMC (DW MSHC 2.70a, FIFO 256) | `0xf9830000` | 35 | |
| SD slot (DW MSHC) | `0xf9820000` | 34 | not working, see [appendix](../docs/appendix-sd-card.md) |
| Ethernet (`hisi-femac`) | `0xf9840000`, MDIO `+0x1100`, GLB `+0x1300` | 71 | internal PHY `0x20669823` at MDIO 1, MII |
| EHCI0 / OHCI0 | `0xf9890000` / `0xf9880000` | 66 / 67 | the external USB port; internal Wi-Fi on port 2 |
| EHCI1 / OHCI1 | `0xf9930000` / `0xf9920000` | 62 / 63 | no external port |
| USB3 (DWC3 2.50a as xHCI) | `0xf98a0000` | 69 | no external port |
| USB OTG (dwc2) | `0xf98c0000` | 68 | not enabled |
| GPU Mali-450 MP2 | `0xf9200000` | 94 (all units) | GP, 2 PP, 2 L2, PMU, broadcast/DLBU |
| IR receiver | `0xf8001000` | 47 | clock: system controller `+0x48` bit 4 enable, bit 5 reset |
| Hardware RNG | `0xf8005000` | | old layout: CTRL +0, NUMBER +4, STAT +8 |
| Display (VDP) | `0xf8cc0000` | | see HDMI below |
| GPIO banks | `0xf8b20000`–`0xf8b26000`, `0xf8004000` | | from the stock kernel; unused so far |

## Clocks and resets (CRG `0xf8a22000`)

| Offset | Block | Notes |
|---|---|---|
| `+0x48` CPU_LP | CPU clock mux | sel 0 APLL, 1 800 MHz, 3 1.5 GHz, 4 24 MHz, 5 1.2 GHz, 6 400 MHz, 7 600 MHz; write `sel\|0x300`, then `sel\|0x700`. **Don't** copy the vendor's 400 MHz switch during core start-up: it breaks Ethernet RX until reset |
| `+0x50` | CPU resets | bits 12+n (power-on), 20+n (debug), 16+n (core) for core n; secondaries enter through a jump at physical `0xffff0000` |
| `+0x9c` | SD clock | `0x203`: 50 MHz |
| `+0xa0` | eMMC clock | `0x00040103`: clk_sel[9:8] (0 75, **1 100**, 2 50, 3 24 MHz), drive phase [18:16] 180°, sample phase [14:12] 0°. DDR52 at 50 MHz works with these phases |
| `+0xb0`, `+0x194` | USB3 controller / PHY | replay of vendor `clk-hiusb3.c` |
| `+0xb8`/`+0xbc`, `+0x190`/`+0x198` | USB2 host 0 / host 1 | PHY via a test bus at PERI_CTRL `+0x124` / `+0x158` (the PHY is not memory-mapped) |
| `+0xd4` | GPU | bit 0 clock, bits 8–10 PP clocks, bit 4 reset |
| `+0x124` | GPU clock | [2:0] sel 6 200, 5 250, 4 300, 3 345.6, 2 375, 1 400, 0 432 MHz; switch: PMC `+0x54` bit 18 (direction) + bit 21 request pulse |

## Temperature and voltage (PMC `0xf8a23000`)

- **T-sensor** (from the stock kernel's `mpu_get_temperature`): write `0x07200000` to `+0x28`, average the
  8 bytes in `+0x30`/`+0x34`, `T = avg × 180 / 255 − 40` °C
- CPU voltage: PWM duty at `+0x1c` (core `+0x18`), set by the bootloader from fused tables;
  V = 1495 − (duty − 1) × 2.5 mV, about 1380 mV here
- GPU power setup (vendor `hisi_pmc_setparameter`): `+0xb0` clear bits 0/1, `+0x54` bit 20 + wait `0x2fff`,
  `+0x48` = `0x00ffff88`, CRG `+0x124` clear bits 9/8/5:4
- Stock cooling: full load at 1.2 GHz passes 100 °C in 15 min, freezes around 90 °C. Vendor thresholds:
  throttle 115, recover 100, reboot 125 °C

## HDMI

- The bootloader inits HDMI and shows a JPEG logo if `blkdevparts` contains partitions named
  **`baseparam`** and **`logo`** ([`extras/splash/`](../extras/splash/))
- `baseparam`: display format `0` = 1080p60 (Android's was `24` = 1024×768) and no overscan margins
- HD graphics layer: VDP `+0x6800`; framebuffer `0x3efd5000`, ARGB8888, stride 7680; the bootloader
  reserves `0x3e357000`–`0x40000000`, which Linux must leave alone

## Internal devices

- Wi-Fi: Realtek **RTL8192DU** (USB `0bda:8193`), dual-band
- The box has **one** external USB port (EHCI0/OHCI0 port 1)

## Files

- [`vendor/stock-android-boot.txt`](vendor/stock-android-boot.txt): stock boot log (Fastboot + kernel 3.10)
- [`vendor/stock-android-discovery.txt`](vendor/stock-android-discovery.txt): `/proc` and `/sys` dump from
  the stock kernel (memory map, interrupts, devices, kernel config)
