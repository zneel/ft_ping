#!/usr/bin/env bash
# Shared locations and connection settings for the ft_ping dev VM. Sourced by
# the other vm/ scripts (and by the Makefile's wrappers) so the artifact paths
# and the SSH port have a single definition.

VM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$VM_DIR/.." && pwd)"

BASE_IMG="$VM_DIR/debian-12-genericcloud-arm64.qcow2"
DISK_IMG="$VM_DIR/disk.qcow2"
SEED_ISO="$VM_DIR/seed.iso"
SSH_KEY="$VM_DIR/id_ed25519"
CIDATA_DIR="$VM_DIR/.cidata"
FW_VARS="$VM_DIR/efi-vars.fd"
PID_FILE="$VM_DIR/vm.pid"
CONSOLE_LOG="$VM_DIR/console.log"

SSH_PORT="${SSH_PORT:-2222}"
SSH_USER="${SSH_USER:-debian}"
