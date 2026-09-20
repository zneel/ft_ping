#!/usr/bin/env bash
# Stops the VM and deletes every generated artifact except the downloaded base
# image, so the next ./setup.sh rebuilds from scratch.
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

"$VM_DIR/stop.sh"

rm -f "$DISK_IMG" "$FW_VARS" "$SEED_ISO" "$CONSOLE_LOG"
rm -rf "$CIDATA_DIR"
echo "Cleaned. Run ./setup.sh to rebuild (base image kept)."
