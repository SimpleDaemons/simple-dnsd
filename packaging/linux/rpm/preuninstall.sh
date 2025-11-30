#!/bin/bash
# Pre-uninstallation script for simple-dnsd RPM

set -e

PROJECT_NAME="simple-dnsd"

# Stop service before removal
if [ "$1" -eq 0 ]; then
    systemctl stop "$simple-dnsd" 2>/dev/null || true
    systemctl disable "$simple-dnsd" 2>/dev/null || true
fi

exit 0

