#!/bin/bash
# Post-uninstallation script for simple-dnsd RPM

set -e

# Reload systemd
systemctl daemon-reload

exit 0

