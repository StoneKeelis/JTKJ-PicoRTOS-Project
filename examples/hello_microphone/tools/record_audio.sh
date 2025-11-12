#!/usr/bin/env bash
set -euo pipefail

PORT="${1:-/dev/ttyACM0}"       # optional arg 1: serial device
SECS=5
SR=8000
BYTES=$((SECS * SR * 2))        # 16-bit mono => 2 bytes/sample
OUT=mic_5s_$(date +%Y%m%d_%H%M%S).raw

echo "Preparing the output" 
# Put TTY in raw, 8-bit clean mode
stty -F "$PORT" raw -echo -echoe -echok

# Open the port once for read+write on FD 3
exec 3<> "$PORT"
trap 'exec 3>&- 3<&- || true' EXIT

echo "Collecting data"
# Read exactly the expected byte count into the file
head -c "$BYTES" <&3 > "$OUT"

# All good — close and report
exec 3>&- 3<&-
echo "done: File written in $OUT"

# Quick play/convert hints
echo "Play it:   aplay -f S16_LE -r $SR -c 1 \"$OUT\""
echo "To WAV:    sox -r $SR -e signed -b 16 -c 1 \"$OUT\" \"${OUT%.raw}.wav\""
