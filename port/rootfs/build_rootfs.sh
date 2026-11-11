#!/usr/bin/env bash
# Build Alpine 3.23.3 armv7 rootfs for eMMC p4 (rootfs, block 0x11000) and pack it as ext4 image (tftp/alpine-p17.ext4).
# Runs entirely on the host under fakeroot; package scripts are NOT executed (foreign arch),
# so OpenRC runlevels and config are set up by hand below.
set -euo pipefail
P=/home/arch/projects/RE/HIsilicon
A=$P/alpine
R=$A/rootfs
IMG=$P/tftp/alpine-rootfs.ext4
# MINIMAL=1 skips GPU userspace (Mesa pulls in LLVM, ~200 MB); image size follows.
MINIMAL=${MINIMAL:-0}
if [ "$MINIMAL" = 1 ]; then IMG_MB=${IMG_MB:-192}; else IMG_MB=${IMG_MB:-640}; fi
APK=$A/apk-static/sbin/apk.static
STATE=$A/fakeroot.state
PKGS="alpine-base openrc busybox-openrc busybox-mdev-openrc e2fsprogs e2fsprogs-extra util-linux-misc u-boot-tools openssh chrony"
PKGS="$PKGS iw wpa_supplicant zram-init zram-init-openrc"          # Wi-Fi (RTL8192DU, b50+), zram swap (b51+)
[ "$MINIMAL" = 1 ] || PKGS="$PKGS mesa-dri-gallium mesa-egl mesa-utils"   # lima GPU userspace (b47+)

rm -rf "$R" "$STATE"; mkdir -p "$R"; : > "$STATE"
FR="fakeroot -i $STATE -s $STATE --"   # only for tar/apk/config (chown calls)

$FR tar -xzf "$A/alpine-minirootfs-3.23.3-armv7.tar.gz" -C "$R"
$FR "$APK" --root "$R" --arch armv7 --no-scripts --no-interactive \
    --keys-dir "$R/etc/apk/keys" --repositories-file "$R/etc/apk/repositories" \
    --cache-dir "$A/apk-cache" add $PKGS

$FR install -m 0644 "$P/port/board/fw_env.config" "$R/etc/fw_env.config"
$FR install -D -m 0755 "$P/port/board/uboot-prompt" "$R/usr/local/bin/uboot-prompt"
$FR install -D -m 0755 "$P/port/board/devmem" "$R/usr/local/bin/devmem"   # built on-board from devmem.c (musl)
$FR install -D -m 0755 "$P/port/board/tsensor" "$R/usr/local/bin/tsensor"
$FR install -D -m 0644 "$P/extras/gpu/gles2_test.c" "$R/root/gltest/gles2_test.c"     # gcc -O2 -o t gles2_test.c -lEGL -lGLESv2
$FR install -D -m 0644 "$P/extras/gpu/gles2_stress.c" "$R/root/gltest/gles2_stress.c"
$FR install -D -m 0600 "$HOME/.ssh/id_ed25519.pub" "$R/root/.ssh/authorized_keys"

$FR sh -e -s "$R" <<'EOF'
R=$1
cat > $R/etc/inittab <<'X'
::sysinit:/sbin/openrc sysinit
::sysinit:/sbin/openrc boot
::wait:/sbin/openrc default
ttyAMA0::respawn:/sbin/getty -L 115200 ttyAMA0 vt100
::ctrlaltdel:/sbin/reboot
::shutdown:/sbin/openrc shutdown
X
grep -qx ttyAMA0 $R/etc/securetty 2>/dev/null || echo ttyAMA0 >> $R/etc/securetty
echo hi3798 > $R/etc/hostname
printf '127.0.0.1\thi3798 localhost\n::1\tlocalhost\n' > $R/etc/hosts
cat > $R/etc/fstab <<'X'
/dev/mmcblk0p4   /      ext4   rw,noatime  0 1
tmpfs            /tmp   tmpfs  nosuid,nodev 0 0
X
# network: static (same address U-Boot uses), LAN gateway/DNS 192.168.1.1
printf 'auto lo\niface lo inet loopback\n\nauto eth0\niface eth0 inet static\n\taddress 192.168.1.10/24\n\tgateway 192.168.1.1\n' > $R/etc/network/interfaces
# Wi-Fi (wlan0, RTL8192DU): fill in /etc/wpa_supplicant/wpa_supplicant.conf, uncomment below,
# then: rc-update add wpa_supplicant boot
cat >> $R/etc/network/interfaces <<'X'

#auto wlan0
#iface wlan0 inet dhcp
X
mkdir -p $R/etc/wpa_supplicant
cat > $R/etc/wpa_supplicant/wpa_supplicant.conf <<'X'
ctrl_interface=/var/run/wpa_supplicant
country=SK
#network={
#	ssid="YOUR_SSID"
#	psk="YOUR_PASSPHRASE"
#}
X
chmod 600 $R/etc/wpa_supplicant/wpa_supplicant.conf
# zram swap: 256 MiB zstd-compressed (kernel CONFIG_ZRAM, b51+)
cat > $R/etc/conf.d/zram-init <<'X'
load_on_start=yes
unload_on_stop=yes
num_devices=1
type0=swap
flag0=
size0=256
algo0=zstd
labl0=zram_swap
X
echo 'nameserver 192.168.1.1' > $R/etc/resolv.conf
# SP805 hardware watchdog: busybox daemon pets every 5 s, hw resets after 30 s
printf 'WATCHDOG_OPTS="-T 30 -t 5"\nWATCHDOG_DEV="/dev/watchdog"\n' > $R/etc/conf.d/watchdog
chmod 700 $R/root/.ssh
# accounts normally created by package pre-install scripts (skipped by --no-scripts)
grep -q '^sshd:' $R/etc/passwd  || echo 'sshd:x:22:22:sshd:/dev/null:/sbin/nologin' >> $R/etc/passwd
grep -q '^sshd:' $R/etc/group   || echo 'sshd:x:22:' >> $R/etc/group
grep -q '^chrony:' $R/etc/passwd || echo 'chrony:x:100:101:chrony:/dev/null:/sbin/nologin' >> $R/etc/passwd
grep -q '^chrony:' $R/etc/group  || echo 'chrony:x:101:chrony' >> $R/etc/group
grep -q '^sshd:' $R/etc/shadow  || echo 'sshd:!::0:::::' >> $R/etc/shadow
grep -q '^chrony:' $R/etc/shadow || echo 'chrony:!::0:::::' >> $R/etc/shadow
mkdir -p $R/var/lib/chrony
# no RTC: boots ~days behind; let chrony step instead of slewing (slew runs the clock ~8 % fast for days)
sed -i '/^makestep/d' $R/etc/chrony/chrony.conf; echo 'makestep 1 -1' >> $R/etc/chrony/chrony.conf
# root: empty password (serial-only dev board) -- change with passwd after first login
sed -i 's|^root:[^:]*:|root::|' $R/etc/shadow
# OpenRC runlevels (normally created by package scripts / setup-alpine)
add() { mkdir -p $R/etc/runlevels/$1; for s in $2; do [ -e $R/etc/init.d/$s ] && ln -sf /etc/init.d/$s $R/etc/runlevels/$1/$s || echo "skip $s"; done; }
add sysinit "devfs dmesg mdev"
add boot    "bootmisc hostname modules sysctl swclock seedrng syslog networking"
add default "sshd chronyd watchdog zram-init"
add shutdown "killprocs mount-ro savecache"
# no RTC on board: swclock keeps time monotonic across reboots
chmod 1777 $R/tmp
chown -R 0:0 $R
EOF

# mke2fs runs WITHOUT fakeroot (its inode db gets confused by apk's delete/recreate),
# so ownership and special modes are fixed inside the image with debugfs afterwards.
find "$R" -type f ! -perm -u+r -exec chmod u+r {} +

rm -f "$IMG"
mke2fs -q -t ext4 -L alpine-root -d "$R" "$IMG" "${IMG_MB}M"
{
  (cd "$R" && find . -mindepth 1) | sed 's|^\.||' | while read -r f; do
    printf 'sif "%s" uid 0\nsif "%s" gid 0\n' "$f" "$f"
  done
  printf 'sif / uid 0\nsif / gid 0\n'
  echo 'sif /bin/bbsuid mode 0104111'
  echo 'sif /etc/shadow gid 42'
  echo 'sif /etc/shadow mode 0100640'
  echo 'sif /usr/bin/wall gid 5'
  echo 'sif /usr/bin/wall mode 0102755'
  echo 'sif /var/lib/chrony uid 100'
  echo 'sif /var/lib/chrony gid 101'
  echo 'sif /tmp mode 041777'
  echo 'sif /var/tmp mode 041777'
} > "$A/debugfs-fixup.cmd"
debugfs -w -f "$A/debugfs-fixup.cmd" "$IMG" >/dev/null 2>"$A/debugfs-fixup.err" || true
grep -v '^debugfs' "$A/debugfs-fixup.err" | head -5 || true
e2fsck -fn "$IMG" >/dev/null && echo "fsck clean"
ls -la "$IMG"; sha256sum "$IMG" | tee "$IMG.sha256"
