#!/usr/bin/env bash
# Boots the ft_ping Debian dev VM (aarch64, HVF-accelerated) in the background.
# Access is over SSH only; the project directory is shared read/write into the
# guest via virtio-9p at /mnt/ft_ping.
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

QEMU_SHARE="$(dirname "$(dirname "$(command -v qemu-system-aarch64)")")/share/qemu"
FW_CODE="$QEMU_SHARE/edk2-aarch64-code.fd"
RAM="${RAM:-4G}"
CPUS="${CPUS:-4}"
NET="${NET:-user}"

[ -f "$DISK_IMG" ] || { echo "No disk found. Run ./setup.sh first."; exit 1; }
[ -f "$FW_CODE" ] || { echo "UEFI firmware not found at $FW_CODE (brew install qemu?)"; exit 1; }

if [ ! -f "$FW_VARS" ]; then
  echo "==> Creating EFI vars store"
  truncate -s 64m "$FW_VARS"
fi

case "$NET" in
  user)
    # Slirp: convenient (SSH on localhost:$SSH_PORT) but it proxies ICMP through
    # a host socket, so TTL is faked and ICMP time-exceeded never comes back.
    NET_ARGS=(-netdev "user,id=net0,hostfwd=tcp::${SSH_PORT}-:22")
    ;;
  vmnet)
    # Real NAT interface via macOS vmnet: honours TTL and delivers ICMP
    # time-exceeded, which ft_ping needs. Requires root, and SSH goes to the
    # guest's own IP rather than a forwarded port.
    if [ "$(id -u)" -ne 0 ]; then
      echo "NET=vmnet needs root: sudo NET=vmnet $0"
      exit 1
    fi
    NET_ARGS=(-netdev "vmnet-shared,id=net0")
    ;;
  *)
    echo "Unknown NET='$NET' (expected 'user' or 'vmnet')"
    exit 1
    ;;
esac

CDROM_ARGS=()
if [ -f "$SEED_ISO" ]; then
  CDROM_ARGS=(-drive "if=none,id=cidata,file=$SEED_ISO,format=raw,readonly=on" \
              -device virtio-scsi-pci,id=scsi0 \
              -device scsi-cd,bus=scsi0.0,drive=cidata)
fi

if [ -f "$PID_FILE" ] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
  echo "VM already running (pid $(cat "$PID_FILE")). Stop it with ./stop.sh"
  exit 1
fi

echo "==> Booting VM in the background (console log: $CONSOLE_LOG)"

qemu-system-aarch64 \
  -machine virt,highmem=on \
  -accel hvf \
  -cpu host \
  -smp "$CPUS" \
  -m "$RAM" \
  -drive "if=pflash,format=raw,readonly=on,file=$FW_CODE" \
  -drive "if=pflash,format=raw,file=$FW_VARS" \
  -drive "if=virtio,format=qcow2,file=$DISK_IMG" \
  "${CDROM_ARGS[@]}" \
  -virtfs "local,path=$PROJECT_DIR,mount_tag=ft_ping,security_model=mapped-xattr,id=ft_ping" \
  "${NET_ARGS[@]}" \
  -device virtio-net-pci,netdev=net0 \
  -device virtio-rng-pci \
  -display none \
  -serial "file:$CONSOLE_LOG" \
  -pidfile "$PID_FILE" \
  -daemonize

if [ "$NET" != user ]; then
  echo "==> vmnet mode: no port forward — find the guest IP in $CONSOLE_LOG"
  exit 0
fi

# Also require a non-cloud kernel: cloud-init reports "done" just before the
# reboot it schedules, so that alone would report ready on a VM about to drop.
echo -n "==> Waiting for the VM (first boot provisions, then reboots once)"
for _ in $(seq 240); do
  if ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
         -o BatchMode=yes -o ConnectTimeout=3 \
         -p "$SSH_PORT" -i "$SSH_KEY" "$SSH_USER@localhost" \
         'cloud-init status 2>/dev/null | grep -q "^status: done" && ! uname -r | grep -q cloud' 2>/dev/null; then
    echo
    echo "==> Ready.  vm/ssh.sh  (ssh -p $SSH_PORT -i $SSH_KEY $SSH_USER@localhost)"
    echo "    Project shared at /mnt/ft_ping"
    exit 0
  fi
  echo -n "."
  sleep 2
done

echo
echo "VM did not become ready in time — check $CONSOLE_LOG"
exit 1
