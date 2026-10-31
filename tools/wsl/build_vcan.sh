#!/bin/bash
# Builds the vcan (virtual CAN) kernel module for the running WSL2 kernel, so the
# Linux build of AgISOVirtualTerminal can use a SocketCAN vcan interface in WSL.
#
# The stock WSL2 kernel has CAN and CAN_RAW but not vcan. This builds Microsoft's
# kernel source for the exact running version, with the running configuration plus
# CONFIG_CAN_VCAN=m. A full build is needed because the kernel enforces symbol
# versions (CONFIG_MODVERSIONS). Nothing is installed and the running kernel is
# not replaced. Repeat after every WSL kernel update.
#
# Needs: build-essential flex bison bc libelf-dev libssl-dev dwarves cpio curl
# Usage: tools/wsl/build_vcan.sh     (15-25 min; source under ${KERNEL_SRC_ROOT:-~/src})
# Then:  sudo tools/wsl/install_vcan.sh <path to vcan.ko printed at the end>
set -euo pipefail
KREL=$(uname -r)
case "$KREL" in
  *microsoft*) ;;
  *) echo "Not a WSL2 kernel: $KREL"; exit 1 ;;
esac
TAG="linux-msft-wsl-${KREL%%-*}"
ROOT="${KERNEL_SRC_ROOT:-$HOME/src}"
SRC="$ROOT/WSL2-Linux-Kernel-$TAG"
LOG="$ROOT/vcan_kernel_build.log"

mkdir -p "$ROOT"
if [ ! -d "$SRC" ]; then
  echo "Downloading $TAG"
  curl -fsSL --retry 3 -o "$ROOT/$TAG.tar.gz" \
    "https://github.com/microsoft/WSL2-Linux-Kernel/archive/refs/tags/$TAG.tar.gz"
  tar -xzf "$ROOT/$TAG.tar.gz" -C "$ROOT"
  rm -f "$ROOT/$TAG.tar.gz"
fi

cd "$SRC"
zcat /proc/config.gz > .config
scripts/config --module CONFIG_CAN_VCAN
make -s olddefconfig
BUILT=$(make -s kernelrelease)
[ "$BUILT" = "$KREL" ] || { echo "Kernel release mismatch: $BUILT, running $KREL"; exit 1; }

echo "Building in $SRC with $(nproc) jobs, log: $LOG"
if ! make -j"$(nproc)" vmlinux modules > "$LOG" 2>&1; then
  echo "Build failed. Last lines of $LOG:"
  tail -20 "$LOG"
  exit 1
fi

KO="$SRC/drivers/net/can/vcan.ko"
WANT=$(modinfo -F vermagic "/lib/modules/$KREL/kernel/net/can/can.ko")
GOT=$(modinfo -F vermagic "$KO")
[ "$WANT" = "$GOT" ] || { echo "vermagic mismatch: $GOT, kernel expects $WANT"; exit 1; }
if grep -q 'WARNING: modpost: .*vcan' "$LOG"; then
  grep 'WARNING: modpost: .*vcan' "$LOG"
  exit 1
fi

echo "OK: $KO"
echo "Install with: sudo $(cd "$(dirname "$0")" && pwd)/install_vcan.sh $KO"
