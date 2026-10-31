#!/bin/bash
# Installs a vcan.ko built by build_vcan.sh for the running WSL2 kernel, and adds a
# systemd service that creates vcan0 on every WSL start. Needs systemd in WSL
# ([boot] systemd=true in /etc/wsl.conf).
#
# Usage: sudo tools/wsl/install_vcan.sh <path to vcan.ko>
#
# AgISOVirtualTerminal then uses it with Configure -> Configure CAN Hardware ->
# SocketCAN interface name "vcan0". Only programs inside WSL see vcan0.
#
# Undo: systemctl disable --now vcan0.service
#       rm /etc/systemd/system/vcan0.service /lib/modules/$(uname -r)/updates/vcan.ko
#       depmod -a
set -euo pipefail
[ "$(id -u)" -eq 0 ] || { echo "Run as root (sudo)."; exit 1; }
KO="${1:?usage: install_vcan.sh <path to vcan.ko>}"
KREL=$(uname -r)
[ -f "$KO" ] || { echo "$KO not found; run build_vcan.sh first"; exit 1; }

WANT=$(modinfo -F vermagic "/lib/modules/$KREL/kernel/net/can/can.ko")
GOT=$(modinfo -F vermagic "$KO")
[ "$WANT" = "$GOT" ] || { echo "vermagic mismatch: $GOT, kernel expects $WANT"; exit 1; }

install -D -m 644 "$KO" "/lib/modules/$KREL/updates/vcan.ko"
depmod -a "$KREL"
modprobe vcan

cat > /etc/systemd/system/vcan0.service <<'EOF'
[Unit]
Description=Virtual CAN interface vcan0 (AgISOVirtualTerminal)

[Service]
Type=oneshot
RemainAfterExit=yes
ExecStartPre=/usr/sbin/modprobe vcan
# txqueuelen 1000: ISO 11783 transport protocol bursts overflow vcan's default of 10
ExecStart=/bin/sh -c '/usr/sbin/ip link show vcan0 >/dev/null 2>&1 || /usr/sbin/ip link add dev vcan0 type vcan; /usr/sbin/ip link set vcan0 txqueuelen 1000 up'
ExecStop=/usr/sbin/ip link delete vcan0

[Install]
WantedBy=multi-user.target
EOF
systemctl daemon-reload
systemctl enable --now vcan0.service
systemctl is-active vcan0.service
ip -details link show vcan0
