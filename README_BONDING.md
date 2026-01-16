# OpenVPN Channel Bonding

## Overview

This project extends OpenVPN GUI to support multi-channel bonding, allowing bandwidth aggregation across multiple physical network interfaces (Ethernet, WiFi, LTE, etc.) on Windows clients. The architecture combines client-side packet distribution with server-side Linux bonding to achieve higher throughput and redundancy.

### Purpose

- **Bandwidth Aggregation**: Combine multiple network connections to increase total throughput
- **Redundancy**: Maintain connectivity if one interface fails
- **Load Distribution**: Distribute traffic across multiple VPN tunnels

### Architecture

- **Client-Side**: Multiple OpenVPN instances, each bound to a different physical NIC through separate TAP adapters
- **Server-Side**: Linux bonding driver in balance-rr mode to aggregate tunnels
- **Packet Distribution**: Client-side logic distributes packets across tunnels using round-robin or weighted algorithms

### Supported Platforms

- **Client**: Windows 10/11
- **Server**: Ubuntu/Debian/CentOS with Linux bonding driver

## Build Requirements

### Required Software

- **Visual Studio 2022** Community Edition or higher with "Desktop development with C++" workload
- **CMake** version 3.15 or higher ([Download](https://cmake.org/download/))
- **vcpkg** package manager ([GitHub](https://github.com/Microsoft/vcpkg))
- **Git for Windows** ([Download](https://git-scm.com/download/win))
- **Windows SDK** 10.0.19041.0 or higher (included with Visual Studio)

### Dependencies

- **OpenSSL** (via vcpkg)
- **LZO compression** (via vcpkg)
- **TAP-Windows driver** (bundled with OpenVPN)
- **Windows API libraries**: `iphlpapi.lib`, `ws2_32.lib`, `advapi32.lib`

## Build Instructions

### 1. Clone Repository

```bash
git clone <your-fork-url> openvpn-gui-bonding
cd openvpn-gui-bonding
git checkout feature/channel-bonding
```

### 2. Setup vcpkg

```bash
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg

# Bootstrap vcpkg
C:\vcpkg\bootstrap-vcpkg.bat

# Set environment variable
set VCPKG_ROOT=C:\vcpkg

# Add to PATH (optional, for convenience)
set PATH=%PATH%;C:\vcpkg
```

### 3. Install Dependencies

```bash
vcpkg install openssl:x64-windows lzo:x64-windows
vcpkg integrate install
```

### 4. Generate Build Files

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### 5. Build

```bash
# Command line
cmake --build build --config Release

# Or open in Visual Studio
# Open: build\openvpn-gui.sln
# Build → Build Solution (Ctrl+Shift+B)
```

### 6. Output Location

- **Executable**: `build\Release\openvpn-gui.exe`

## Development Workflow

### Branch Naming

- Feature branches: `feature/bonding-<component-name>`
- Bug fixes: `fix/bonding-<issue-description>`
- Documentation: `docs/bonding-<topic>`

### Commit Message Format

```
[BONDING] <component>: <description>

Detailed explanation if needed.
```

Examples:
- `[BONDING] config: Add bonding profile parser`
- `[BONDING] manager: Implement multi-instance OpenVPN spawner`
- `[BONDING] docs: Update architecture documentation`

### Code Style

Follow existing OpenVPN GUI conventions:
- **Indentation**: 4 spaces (K&R style)
- **Naming**: `snake_case` for functions and variables
- **Comments**: Clear, concise, explain why not what

## Project Structure

```
openvpn-gui-bonding/
├── bonding/                          # Bonding modules
│   ├── include/                      # Header files
│   │   ├── bonding_config.h         # Configuration structures
│   │   ├── bonding_manager.h        # Manager interface
│   │   ├── openvpn_manager.h        # Multi-instance manager
│   │   ├── packet_distributor.h     # Packet distribution
│   │   ├── nic_detector.h           # NIC detection
│   │   └── bonding_types.h          # Common types and enums
│   ├── src/                          # Implementation files
│   │   ├── config_parser.c          # Config file parser
│   │   ├── bonding_manager.c        # Main coordinator
│   │   ├── openvpn_manager.c        # Process management
│   │   ├── packet_distributor.c     # Packet routing
│   │   └── nic_detector.c           # NIC enumeration
│   ├── service/                      # Windows service component
│   │   ├── bonding_service.c        # Service entry point
│   │   └── service_ipc.c            # IPC with GUI
│   ├── ui/                           # GUI integration
│   │   ├── bonding_dialog.c         # Configuration dialog
│   │   └── bonding_status.c         # Status display
│   ├── utils/                        # Utility functions
│   │   ├── logging.c                # Logging system
│   │   └── routing_helper.c        # Windows routing API
│   ├── tests/                        # Unit tests
│   │   └── test_config_parser.c     # Config parser tests
│   ├── CMakeLists.txt               # CMake build for bonding
│   └── README.md                     # Module documentation
├── docs/                             # Documentation
│   └── bonding/                      # Bonding-specific docs
│       ├── architecture.md           # Architecture overview
│       ├── api.md                    # API documentation
│       └── configuration.md          # Config file format
└── README_BONDING.md                 # This file
```

### Component Responsibilities

- **bonding_config.h/c**: Configuration data structures and file I/O
- **bonding_manager.h/c**: Central coordinator managing all bonding components
- **openvpn_manager.h/c**: Spawns and monitors multiple OpenVPN processes
- **packet_distributor.h/c**: Distributes packets across tunnels
- **nic_detector.h/c**: Enumerates and monitors physical network interfaces
- **bonding_service.c**: Windows service running with elevated privileges
- **service_ipc.c**: Named pipe communication between GUI and service
- **bonding_dialog.c**: User interface for bonding configuration
- **bonding_status.c**: Status display and monitoring

## Testing

### Unit Tests

- **Location**: `bonding/tests/`
- **Build**: `cmake --build build --target bonding_tests`
- **Run**: `build\Release\bonding_tests.exe`

### Integration Testing

- Test with multiple physical NICs (Ethernet + WiFi)
- Verify TAP adapter creation and binding
- Validate routing table configuration
- Test failover scenarios

## Troubleshooting

### Common Build Errors

**Error: CMake cannot find vcpkg**
- Solution: Ensure `VCPKG_ROOT` environment variable is set
- Verify: `cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake`

**Error: Missing OpenSSL or LZO**
- Solution: Run `vcpkg install openssl:x64-windows lzo:x64-windows`
- Verify: `vcpkg list`

**Error: Visual Studio version incompatible**
- Solution: Use Visual Studio 2022 or higher
- Verify: Check `CMAKE_C_COMPILER_ID` in CMake output

### vcpkg Integration Issues

**Packages not found during build**
- Run `vcpkg integrate install` to register with Visual Studio
- Restart Visual Studio after integration

**Wrong architecture (x86 vs x64)**
- Ensure using `:x64-windows` suffix: `vcpkg install openssl:x64-windows`
- Check CMake generator: `cmake -G "Visual Studio 17 2022" -A x64`

### CMake Configuration Problems

**CMake cache issues**
- Delete `build/` directory and regenerate: `rm -rf build && cmake -B build -S .`

**Missing Windows SDK**
- Install Windows 10 SDK via Visual Studio Installer
- Verify: `cmake --version` and check SDK version

## Next Steps

1. **Configuration Data Structures**: Implement bonding profile parser and data structures
2. **Multi-Instance Manager**: Create OpenVPN process spawner and monitor
3. **NIC Detection**: Implement Windows API-based network interface enumeration
4. **Packet Distribution**: Develop round-robin and weighted distribution algorithms
5. **GUI Integration**: Add bonding configuration dialog and status display
6. **Windows Service**: Implement elevated-privilege service for TAP and routing management

See `docs/bonding/architecture.md` for detailed architecture documentation.
