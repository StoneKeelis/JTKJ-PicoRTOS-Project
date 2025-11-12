#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"   # serial device
SR="${2:-8000}"            # sample rate Hz (default 16 kHz)

# Put TTY in raw, 8-bit mode (baud ignored for USB CDC but fine to set)
stty -F "$PORT" 115200 raw -echo -echoe -echok

# Open the port once (shared FD) and auto-close on exit
exec 3<> "$PORT"
trap 'exec 3>&- 3<&- || true' EXIT

# Wait until device prints READY (consumes that line)
grep -m1 -a "READY" <&3 >/dev/null

# Stream the remaining bytes (binary PCM) straight to aplay
exec aplay -q -f S16_LE -r "$SR" -c 1 <&3