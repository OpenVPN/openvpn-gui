# Bonding Configuration

## Configuration File Format

Bonding profiles use the `.ovpn-bond` file extension and extend the standard OpenVPN configuration format with bonding-specific parameters.

## File Structure

```
# OpenVPN Channel Bonding Configuration
# Profile: <profile_name>

# Bonding Mode
# Options: round-robin, weighted, active-backup
bonding-mode round-robin

# Number of tunnels
tunnel-count 3

# Tunnel 0 Configuration
[tunnel-0]
nic-name "Ethernet"
server-host vpn.example.com
server-port 1194
server-config server1.ovpn
weight 1

# Tunnel 1 Configuration
[tunnel-1]
nic-name "Wi-Fi"
server-host vpn.example.com
server-port 1195
server-config server2.ovpn
weight 1

# Tunnel 2 Configuration
[tunnel-2]
nic-name "Ethernet 2"
server-host vpn.example.com
server-port 1196
server-config server3.ovpn
weight 2

# Standard OpenVPN parameters (applied to all tunnels)
remote vpn.example.com
proto udp
dev tun
ca ca.crt
cert client.crt
key client.key
```

## Configuration Parameters

### Global Parameters

- **bonding-mode**: Bonding distribution algorithm
  - `round-robin`: Sequential packet distribution
  - `weighted`: Weight-based distribution
  - `active-backup`: Active/standby redundancy

- **tunnel-count**: Number of tunnels to create (must match number of `[tunnel-N]` sections)

### Per-Tunnel Parameters

Each tunnel section `[tunnel-N]` contains:

- **nic-name**: Physical NIC name to bind this tunnel to
  - Must match a physical NIC name detected by the system
  - Examples: "Ethernet", "Wi-Fi", "Local Area Connection"

- **server-host**: OpenVPN server hostname or IP address
  - Can be same for all tunnels (different ports) or different servers

- **server-port**: OpenVPN server port for this tunnel
  - Must be unique per tunnel
  - Example: 1194, 1195, 1196

- **server-config**: Path to base OpenVPN configuration file
  - Contains common OpenVPN parameters
  - Tunnel-specific parameters override base config

- **weight**: Distribution weight (for weighted mode)
  - Higher weight = more traffic
  - Default: 1
  - Ignored in round-robin mode

## Example Configuration Files

### Simple Round-Robin (2 Tunnels)

```
# Simple 2-tunnel bonding
bonding-mode round-robin
tunnel-count 2

[tunnel-0]
nic-name "Ethernet"
server-host vpn.example.com
server-port 1194
server-config base.ovpn
weight 1

[tunnel-1]
nic-name "Wi-Fi"
server-host vpn.example.com
server-port 1195
server-config base.ovpn
weight 1
```

### Weighted Distribution (3 Tunnels)

```
# Weighted bonding with 3 tunnels
bonding-mode weighted
tunnel-count 3

[tunnel-0]
nic-name "Ethernet"
server-host vpn.example.com
server-port 1194
server-config base.ovpn
weight 3

[tunnel-1]
nic-name "Wi-Fi"
server-host vpn.example.com
server-port 1195
server-config base.ovpn
weight 2

[tunnel-2]
nic-name "LTE"
server-host vpn.example.com
server-port 1196
server-config base.ovpn
weight 1
```

### Active-Backup Redundancy

```
# Active-backup for redundancy
bonding-mode active-backup
tunnel-count 2

[tunnel-0]
nic-name "Ethernet"
server-host vpn.example.com
server-port 1194
server-config base.ovpn
weight 1

[tunnel-1]
nic-name "Wi-Fi"
server-host vpn.example.com
server-port 1195
server-config base.ovpn
weight 1
```

## Configuration Validation

The configuration parser validates:

1. **Required parameters**: All global and per-tunnel parameters must be present
2. **NIC names**: Must match detected physical NICs
3. **Port uniqueness**: Each tunnel must have a unique server port
4. **Tunnel count**: Must match number of tunnel sections
5. **File paths**: Server-config files must exist and be readable
6. **Mode compatibility**: Weight parameter required for weighted mode

## Configuration Generation

When a user configures bonding through the GUI:

1. User selects physical NICs
2. User configures server endpoints
3. GUI generates `.ovpn-bond` file
4. File is saved to OpenVPN config directory
5. Configuration is sent to bonding service

## Integration with OpenVPN Configs

Each tunnel uses a base OpenVPN configuration file (specified in `server-config`). The bonding system:

1. Loads base OpenVPN config
2. Overrides tunnel-specific parameters:
   - `--dev-node <tap-adapter>`: Binds to specific TAP adapter
   - `--remote <server-host> <server-port>`: Sets server endpoint
   - `--nobind`: Allows binding to specific interface
3. Generates temporary config for OpenVPN process
4. Spawns OpenVPN with generated config

## Server-Side Requirements

For each tunnel, the server must:

1. Run OpenVPN server instance on specified port
2. Use separate TUN interface (tun0, tun1, tun2, etc.)
3. Configure Linux bonding to aggregate tunnel interfaces
4. Use same CA for certificate validation

See `architecture.md` for server-side configuration details.
