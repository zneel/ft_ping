#!/usr/bin/env bash
# Opens a shell on the running ft_ping VM (or runs the given command there),
# using the same port and key as run.sh.
set -euo pipefail

source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

exec ssh -p "$SSH_PORT" -i "$SSH_KEY" "$SSH_USER@localhost" "$@"
