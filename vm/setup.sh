#!/usr/bin/env bash
# Downloads a Debian 12 (bookworm) cloud image and builds a bootable qcow2
# disk + cloud-init seed ISO for the ft_ping dev VM. Run once (or after
# `make vm-clean`) before `./run.sh`.
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

IMG_URL="https://cloud.debian.org/images/cloud/bookworm/latest/debian-12-genericcloud-arm64.qcow2"
DISK_SIZE="16G"

echo "==> Checking prerequisites"
command -v qemu-img >/dev/null || { echo "qemu-img not found (brew install qemu)"; exit 1; }
command -v hdiutil >/dev/null || { echo "hdiutil not found (this script expects macOS)"; exit 1; }

if [ ! -f "$BASE_IMG" ]; then
  echo "==> Downloading Debian 12 genericcloud arm64 image"
  curl -L --fail -o "$BASE_IMG.tmp" "$IMG_URL"
  mv "$BASE_IMG.tmp" "$BASE_IMG"
else
  echo "==> Base image already downloaded, skipping"
fi

echo "==> Creating writable overlay disk ($DISK_SIZE)"
qemu-img create -f qcow2 -F qcow2 -b "$BASE_IMG" "$DISK_IMG" "$DISK_SIZE"

if [ ! -f "$SSH_KEY" ]; then
  echo "==> Generating SSH keypair for the VM"
  ssh-keygen -t ed25519 -N "" -C "ft_ping-vm" -f "$SSH_KEY" >/dev/null
else
  echo "==> SSH keypair already exists, skipping"
fi
PUBKEY="$(cat "$SSH_KEY.pub")"

echo "==> Building cloud-init seed ISO"
rm -rf "$CIDATA_DIR"
mkdir -p "$CIDATA_DIR"
sed -e "s#__SSH_PUBKEY__#${PUBKEY}#" -e "s#__HOST_UID__#$(id -u)#" \
  "$VM_DIR/cloud-init/user-data" > "$CIDATA_DIR/user-data"
cp "$VM_DIR/cloud-init/meta-data" "$CIDATA_DIR/meta-data"
rm -f "$SEED_ISO"
hdiutil makehybrid -iso -joliet -default-volume-name cidata -o "$SEED_ISO" "$CIDATA_DIR"

echo
echo "==> Done. Disk: $DISK_IMG"
echo "    Seed:  $SEED_ISO"
echo "    Key:   $SSH_KEY"
echo
echo "Next: ./run.sh (first boot runs cloud-init and installs build tools, ~1-2 min)"
