# Packaging

CPack in the top-level `CMakeLists.txt` is the supported package path (`make package`). These files are the maintainer scripts and installer sources that CPack uses.

## Installed layout (must match production templates)

| Platform | Binary | Config | Data | Logs |
|----------|--------|--------|------|------|
| Linux (`CMAKE_INSTALL_PREFIX=/usr`) | `/usr/bin/simple-dnsd` | `/etc/simple-dnsd/simple-dnsd.conf` | `/var/lib/simple-dnsd` | `/var/log/simple-dnsd` |
| macOS | `/usr/local/bin/simple-dnsd` | `/etc/simple-dnsd/simple-dnsd.conf` | `/var/lib/simple-dnsd` | `/var/log/simple-dnsd` |
| Windows | `%PROGRAMFILES%\simple-dnsd\simple-dnsd.exe` | `%PROGRAMDATA%\simple-dnsd\simple-dnsd.conf` | `%PROGRAMDATA%\simple-dnsd` | `%PROGRAMDATA%\simple-dnsd\logs` |

Linux and macOS units start `--config` then `--foreground` so the flag wins over `foreground = false` in the production templates. Packages create the data/log directories and the `simple-dnsd` service user; they do not enable or start the daemon.

On macOS, `packaging/macos/pkg/rebuild-from-cpack.sh` rebuilds the product PKG after CPack so the payload is only `usr/`, `etc/`, `Library/`, and `var/`.
