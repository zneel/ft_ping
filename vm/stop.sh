#!/usr/bin/env bash
# Shuts the ft_ping VM down cleanly (ACPI powerdown, falling back to SIGKILL).
set -euo pipefail

VM_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PID_FILE="$VM_DIR/vm.pid"

if [ ! -f "$PID_FILE" ]; then
  echo "VM is not running (no $PID_FILE)"
  exit 0
fi

PID="$(cat "$PID_FILE")"
if ! kill -0 "$PID" 2>/dev/null; then
  echo "Stale pidfile, cleaning up"
  rm -f "$PID_FILE"
  exit 0
fi

echo "==> Stopping VM (pid $PID)"
kill -TERM "$PID" 2>/dev/null || true
for _ in $(seq 30); do
  kill -0 "$PID" 2>/dev/null || { rm -f "$PID_FILE"; echo "Stopped."; exit 0; }
  sleep 1
done

echo "==> Did not exit, forcing"
kill -KILL "$PID" 2>/dev/null || true
rm -f "$PID_FILE"
echo "Stopped."
