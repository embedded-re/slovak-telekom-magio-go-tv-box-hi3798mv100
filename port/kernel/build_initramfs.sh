#!/usr/bin/env bash
# Build the rescue/boot initramfs embedded in the kernel (alpine/initramfs).
# /init: 3 s "press any key" window -> rescue shell; otherwise mount root= and switch_root.
# Needed because HiSilicon Fastboot autoboot cannot be interrupted once bootcmd is set.
set -euo pipefail
P=/home/arch/projects/RE/HIsilicon
A=$P/alpine
R=$A/initramfs
APK=$A/apk-static/sbin/apk.static
STATE=$A/fakeroot-initramfs.state
rm -rf "$R" "$STATE"; mkdir -p "$R" "$A/apk-cache"; : > "$STATE"
FR="fakeroot -i $STATE -s $STATE --"
$FR tar -xzf "$A/alpine-minirootfs-3.23.3-armv7.tar.gz" -C "$R"
$FR "$APK" --root "$R" --arch armv7 --no-scripts --no-interactive \
    --keys-dir "$R/etc/apk/keys" --repositories-file "$R/etc/apk/repositories" \
    --cache-dir "$A/apk-cache" add u-boot-tools e2fsprogs e2fsprogs-extra >/dev/null
install -m 0644 "$P/port/board/fw_env.config" "$R/etc/fw_env.config"
install -m 0755 "$P/port/board/uboot-prompt" "$R/usr/local/bin/uboot-prompt"
install -m 0755 "$P/port/kernel/initramfs-init" "$R/init"
# kernel's gen_init_cpio maps uid/gid 1000 -> 0 (CONFIG_INITRAMFS_ROOT_UID/GID)
find "$R" -type f ! -perm -u+r -exec chmod u+r {} +
du -sh "$R"
