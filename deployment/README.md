# simple-dnsd Deployment

This directory contains deployment configurations and examples for simple-dnsd.

## Directory Structure

```
deployment/
├── systemd/                    # Linux systemd service files
│   └── simple-dnsd.service
├── launchd/                    # macOS launchd service files
│   └── com.simple-dnsd.simple-dnsd.plist
├── logrotate.d/                # Linux log rotation configuration
│   └── simple-dnsd
├── windows/                    # Windows service management
│   └── simple-dnsd.service.bat
└── examples/                   # Deployment examples
    └── docker/                 # Docker deployment examples
        ├── docker-compose.yml
        └── README.md
```

## Platform-Specific Deployment

### Linux (systemd)

1. **Install the service file:**
   ```bash
   sudo cp deployment/systemd/simple-dnsd.service /etc/systemd/system/
   sudo systemctl daemon-reload
   ```

2. **Create user and group:**
   ```bash
   sudo useradd --system --no-create-home --shell /bin/false simple-dnsd
   ```

3. **Enable and start the service:**
   ```bash
   sudo systemctl enable simple-dnsd
   sudo systemctl start simple-dnsd
   ```

4. **Check status:**
   ```bash
   sudo systemctl status simple-dnsd
   sudo journalctl -u simple-dnsd -f
   ```

### macOS (launchd)

1. **Install the plist file:**
   ```bash
   sudo cp deployment/launchd/com.simple-dnsd.simple-dnsd.plist /Library/LaunchDaemons/
   sudo chown root:wheel /Library/LaunchDaemons/com.simple-dnsd.simple-dnsd.plist
   ```

2. **Load and start the service:**
   ```bash
   sudo launchctl load /Library/LaunchDaemons/com.simple-dnsd.simple-dnsd.plist
   sudo launchctl start com.simple-dnsd.simple-dnsd
   ```

3. **Check status:**
   ```bash
   sudo launchctl list | grep simple-dnsd
   tail -f /var/log/simple-dnsd.log
   ```

### Windows

1. **Run as Administrator:**
   ```cmd
   # Install service
   deployment\windows\simple-dnsd.service.bat install
   
   # Start service
   deployment\windows\simple-dnsd.service.bat start
   
   # Check status
   deployment\windows\simple-dnsd.service.bat status
   ```

2. **Service management:**
   ```cmd
   # Stop service
   deployment\windows\simple-dnsd.service.bat stop
   
   # Restart service
   deployment\windows\simple-dnsd.service.bat restart
   
   # Uninstall service
   deployment\windows\simple-dnsd.service.bat uninstall
   ```

## Log Rotation (Linux)

1. **Install logrotate configuration:**
   ```bash
   sudo cp deployment/logrotate.d/simple-dnsd /etc/logrotate.d/
   ```

2. **Test logrotate configuration:**
   ```bash
   sudo logrotate -d /etc/logrotate.d/simple-dnsd
   ```

3. **Force log rotation:**
   ```bash
   sudo logrotate -f /etc/logrotate.d/simple-dnsd
   ```

## Docker Deployment

See [examples/docker/README.md](examples/docker/README.md) for detailed Docker deployment instructions.

### Quick Start

```bash
# Build and run with Docker Compose
cd deployment/examples/docker
docker-compose up -d

# Check status
docker-compose ps
docker-compose logs simple-dnsd
```

## Configuration

### Service Configuration

Each platform has specific configuration requirements:

- **Linux**: Edit `/etc/systemd/system/simple-dnsd.service`
- **macOS**: Edit `/Library/LaunchDaemons/com.simple-dnsd.simple-dnsd.plist`
- **Windows**: Edit the service binary path in the batch file

### Application Configuration

Place your application configuration in:
- **Linux/macOS**: `/etc/simple-dnsd/simple-dnsd.conf`
- **Windows**: `%PROGRAMFILES%\simple-dnsd\simple-dnsd.conf`

## Security Considerations

### User and Permissions

1. **Create dedicated user:**
   ```bash
   # Linux
   sudo useradd --system --no-create-home --shell /bin/false simple-dnsd
   
   # macOS
   sudo dscl . -create /Users/_simple-dnsd UserShell /usr/bin/false
   sudo dscl . -create /Users/_simple-dnsd UniqueID 200
   sudo dscl . -create /Users/_simple-dnsd PrimaryGroupID 200
   sudo dscl . -create /Groups/_simple-dnsd GroupID 200
   ```

2. **Set proper permissions:**
   ```bash
   # Configuration files
   sudo chown root:simple-dnsd /etc/simple-dnsd/simple-dnsd.conf
   sudo chmod 640 /etc/simple-dnsd/simple-dnsd.conf
   
   # Log files
   sudo chown simple-dnsd:simple-dnsd /var/log/simple-dnsd/
   sudo chmod 755 /var/log/simple-dnsd/
   ```

### Firewall Configuration

Configure firewall rules as needed:

```bash
# Linux (ufw)
sudo ufw allow 67/tcp

# Linux (firewalld)
sudo firewall-cmd --permanent --add-port=67/tcp
sudo firewall-cmd --reload

# macOS
sudo pfctl -f /etc/pf.conf
```

## Monitoring

### Health Checks

1. **Service status:**
   ```bash
   # Linux
   sudo systemctl is-active simple-dnsd
   
   # macOS
   sudo launchctl list | grep simple-dnsd
   
   # Windows
   sc query simple-dnsd
   ```

2. **Port availability:**
   ```bash
   netstat -tlnp | grep 67
   ss -tlnp | grep 67
   ```

3. **Process monitoring:**
   ```bash
   ps aux | grep simple-dnsd
   top -p $(pgrep simple-dnsd)
   ```

### Log Monitoring

1. **Real-time logs:**
   ```bash
   # Linux
   sudo journalctl -u simple-dnsd -f
   
   # macOS
   tail -f /var/log/simple-dnsd.log
   
   # Windows
   # Use Event Viewer or PowerShell Get-WinEvent
   ```

2. **Log analysis:**
   ```bash
   # Search for errors
   sudo journalctl -u simple-dnsd --since "1 hour ago" | grep -i error
   
   # Count log entries
   sudo journalctl -u simple-dnsd --since "1 day ago" | wc -l
   ```

## Troubleshooting

### Common Issues

1. **Service won't start:**
   - Check configuration file syntax
   - Verify user permissions
   - Check port availability
   - Review service logs

2. **Permission denied:**
   - Ensure service user exists
   - Check file permissions
   - Verify directory ownership

3. **Port already in use:**
   - Check what's using the port: `netstat -tlnp | grep 67`
   - Stop conflicting service or change port

4. **Service stops unexpectedly:**
   - Check application logs
   - Verify resource limits
   - Review system logs

### Debug Mode

Run the service in debug mode for troubleshooting:

```bash
# Linux/macOS
sudo -u simple-dnsd /usr/local/bin/simple-dnsd --debug

# Windows
simple-dnsd.exe --debug
```

### Log Levels

Adjust log level for more verbose output:

```bash
# Set log level in configuration
log_level = debug

# Or via environment variable
export SIMPLE-DNSD_LOG_LEVEL=debug
```

## Backup and Recovery

### Configuration Backup

```bash
# Backup configuration
sudo tar -czf simple-dnsd-config-backup-$(date +%Y%m%d).tar.gz /etc/simple-dnsd/

# Backup logs
sudo tar -czf simple-dnsd-logs-backup-$(date +%Y%m%d).tar.gz /var/log/simple-dnsd/
```

### Service Recovery

```bash
# Stop service
sudo systemctl stop simple-dnsd

# Restore configuration
sudo tar -xzf simple-dnsd-config-backup-YYYYMMDD.tar.gz -C /

# Start service
sudo systemctl start simple-dnsd
```

## Updates

### Service Update Process

1. **Stop service:**
   ```bash
   sudo systemctl stop simple-dnsd
   ```

2. **Backup current version:**
   ```bash
   sudo cp /usr/local/bin/simple-dnsd /usr/local/bin/simple-dnsd.backup
   ```

3. **Install new version:**
   ```bash
   sudo cp simple-dnsd /usr/local/bin/
   sudo chmod +x /usr/local/bin/simple-dnsd
   ```

4. **Start service:**
   ```bash
   sudo systemctl start simple-dnsd
   ```

5. **Verify update:**
   ```bash
   sudo systemctl status simple-dnsd
   simple-dnsd --version
   ```
