#!/bin/bash
# Pre-installation script for simple-dnsd RPM
# License acceptance check

set -e

PROJECT_NAME="simple-dnsd"

# Display license and require acceptance
echo "=========================================="
echo "simple-dnsd License Agreement"
echo "=========================================="
echo ""
cat /usr/share/doc/$simple-dnsd/LICENSE 2>/dev/null || \
    cat /usr/share/$simple-dnsd/LICENSE.txt 2>/dev/null || \
    echo "Please review the license at: https://github.com/simpledaemons/$simple-dnsd/blob/main/LICENSE"
echo ""
echo "By continuing, you agree to the terms of the license."
echo ""
read -p "Do you accept the license? [y/N]: " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "License not accepted. Installation cancelled."
    exit 1
fi

exit 0

