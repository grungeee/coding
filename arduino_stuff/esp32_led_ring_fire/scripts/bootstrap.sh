#!/usr/bin/env bash
set -euo pipefail

log() {
  printf '[bootstrap] %s\n' "$1"
}

require_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    log "Missing dependency: $1"
    return 1
  fi
}

log "Preparing Arduino toolchain"

if ! require_command arduino-cli; then
  log "Install arduino-cli first: https://arduino.github.io/arduino-cli/latest/installation/"
  exit 1
fi

if ! arduino-cli config dump >/dev/null 2>&1; then
  log "Initializing arduino-cli configuration"
  arduino-cli config init
fi

if ! arduino-cli core list | grep -q 'esp32:esp32'; then
  log "Installing Espressif 32-bit core (this may take a while)"
  arduino-cli core install esp32:esp32
else
  log "Espressif 32-bit core already installed"
fi

log "Bootstrap complete"
