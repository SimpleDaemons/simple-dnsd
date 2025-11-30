#!/bin/bash
# Post-installation script for simple-dnsd RPM

set -e

PROJECT_NAME="simple-dnsd"
SERVICE_USER="dnsddev"

# Create service user if it doesn't exist
if ! id "$SERVICE_USER" &>/dev/null; then
    useradd -r -s /sbin/nologin -d /var/lib/$simple-dnsd -c "$simple-dnsd service user" "$SERVICE_USER"
fi

# Set ownership
chown -R "$SERVICE_USER:$SERVICE_USER" /etc/$simple-dnsd 2>/dev/null || true
chown -R "$SERVICE_USER:$SERVICE_USER" /var/log/$simple-dnsd 2>/dev/null || true
chown -R "$SERVICE_USER:$SERVICE_USER" /var/lib/$simple-dnsd 2>/dev/null || true

# Enable and start service
systemctl daemon-reload
systemctl enable "$simple-dnsd" 2>/dev/null || true
systemctl start "$simple-dnsd" 2>/dev/null || true

exit 0

