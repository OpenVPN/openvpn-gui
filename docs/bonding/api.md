# Bonding API Documentation

## Overview

This document describes the API for the bonding module components. All functions are implemented in C and follow the OpenVPN GUI coding conventions.

## Configuration API

### bonding_config.h

#### bonding_profile_t

Main configuration structure for a bonding profile.

```c
typedef struct bonding_profile {
    char *profile_name;          /* Profile name */
    bonding_mode_t mode;         /* Bonding mode */
    int tunnel_count;            /* Number of tunnels */
    tunnel_config_t *tunnels;    /* Array of tunnel configurations */
    char *config_path;           /* Path to .ovpn-bond file */
} bonding_profile_t;
```

#### bonding_config_load()

Load a bonding profile from a configuration file.

```c
bonding_profile_t* bonding_config_load(const char *config_path);
```

**Parameters:**
- `config_path`: Path to `.ovpn-bond` configuration file

**Returns:**
- Pointer to `bonding_profile_t` on success
- `NULL` on error

**Example:**
```c
bonding_profile_t *profile = bonding_config_load("configs/my-bond.ovpn-bond");
if (profile == NULL) {
    /* Handle error */
}
```

#### bonding_config_save()

Save a bonding profile to a configuration file.

```c
int bonding_config_save(bonding_profile_t *profile, const char *config_path);
```

**Parameters:**
- `profile`: Bonding profile to save
- `config_path`: Path where to save the configuration

**Returns:**
- `0` on success
- `-1` on error

#### bonding_config_free()

Free memory allocated for a bonding profile.

```c
void bonding_config_free(bonding_profile_t *profile);
```

**Parameters:**
- `profile`: Profile to free (can be `NULL`)

## Bonding Manager API

### bonding_manager.h

#### bonding_manager_create()

Create a new bonding manager instance.

```c
bonding_manager_t* bonding_manager_create(void);
```

**Returns:**
- Pointer to `bonding_manager_t` on success
- `NULL` on error

#### bonding_manager_start()

Start bonding with the given profile.

```c
int bonding_manager_start(bonding_manager_t *mgr, bonding_profile_t *profile);
```

**Parameters:**
- `mgr`: Bonding manager instance
- `profile`: Configuration profile to use

**Returns:**
- `0` on success
- `-1` on error

**Side Effects:**
- Spawns OpenVPN processes
- Creates TAP adapters
- Configures routing

#### bonding_manager_stop()

Stop all bonding tunnels.

```c
int bonding_manager_stop(bonding_manager_t *mgr);
```

**Parameters:**
- `mgr`: Bonding manager instance

**Returns:**
- `0` on success
- `-1` on error

**Side Effects:**
- Terminates OpenVPN processes
- Removes routing entries
- Cleans up TAP adapters

#### bonding_manager_get_status()

Get status of all tunnels.

```c
int bonding_manager_get_status(bonding_manager_t *mgr, tunnel_state_t *states, int max_tunnels);
```

**Parameters:**
- `mgr`: Bonding manager instance
- `states`: Array to receive tunnel states
- `max_tunnels`: Maximum number of states to return

**Returns:**
- Number of tunnels on success
- `-1` on error

#### bonding_manager_destroy()

Destroy bonding manager and free resources.

```c
void bonding_manager_destroy(bonding_manager_t *mgr);
```

**Parameters:**
- `mgr`: Bonding manager instance (can be `NULL`)

## OpenVPN Manager API

### openvpn_manager.h

#### openvpn_manager_create()

Create a new OpenVPN process manager.

```c
openvpn_manager_t* openvpn_manager_create(void);
```

**Returns:**
- Pointer to `openvpn_manager_t` on success
- `NULL` on error

#### openvpn_manager_spawn_instance()

Spawn an OpenVPN process for a specific tunnel.

```c
int openvpn_manager_spawn_instance(openvpn_manager_t *mgr, int tunnel_id, const char *config_path, const char *tap_adapter);
```

**Parameters:**
- `mgr`: OpenVPN manager instance
- `tunnel_id`: Unique identifier for this tunnel
- `config_path`: Path to OpenVPN configuration file
- `tap_adapter`: Name of TAP adapter to use

**Returns:**
- `0` on success
- `-1` on error

#### openvpn_manager_stop_instance()

Stop a specific OpenVPN instance.

```c
int openvpn_manager_stop_instance(openvpn_manager_t *mgr, int tunnel_id);
```

**Parameters:**
- `mgr`: OpenVPN manager instance
- `tunnel_id`: ID of tunnel to stop

**Returns:**
- `0` on success
- `-1` on error

#### openvpn_manager_get_instance_status()

Get status of a specific OpenVPN instance.

```c
int openvpn_manager_get_instance_status(openvpn_manager_t *mgr, int tunnel_id, tunnel_state_t *state);
```

**Parameters:**
- `mgr`: OpenVPN manager instance
- `tunnel_id`: ID of tunnel to query
- `state`: Pointer to receive tunnel state

**Returns:**
- `0` on success
- `-1` on error

## Packet Distributor API

### packet_distributor.h

#### packet_distributor_create()

Create a new packet distributor.

```c
packet_distributor_t* packet_distributor_create(bonding_mode_t mode, int tunnel_count);
```

**Parameters:**
- `mode`: Bonding mode (round-robin, weighted, etc.)
- `tunnel_count`: Number of tunnels

**Returns:**
- Pointer to `packet_distributor_t` on success
- `NULL` on error

#### packet_distributor_select_tunnel()

Select which tunnel to use for the next packet.

```c
int packet_distributor_select_tunnel(packet_distributor_t *dist, int *tunnel_id);
```

**Parameters:**
- `dist`: Packet distributor instance
- `tunnel_id`: Pointer to receive selected tunnel ID

**Returns:**
- `0` on success
- `-1` on error

**Example:**
```c
int tunnel_id;
if (packet_distributor_select_tunnel(dist, &tunnel_id) == 0) {
    /* Route packet to tunnel_id */
}
```

#### packet_distributor_set_tunnel_weight()

Set distribution weight for a tunnel (weighted mode only).

```c
int packet_distributor_set_tunnel_weight(packet_distributor_t *dist, int tunnel_id, int weight);
```

**Parameters:**
- `dist`: Packet distributor instance
- `tunnel_id`: ID of tunnel
- `weight`: Distribution weight (higher = more traffic)

**Returns:**
- `0` on success
- `-1` on error

#### packet_distributor_set_tunnel_health()

Mark a tunnel as healthy or unhealthy.

```c
int packet_distributor_set_tunnel_health(packet_distributor_t *dist, int tunnel_id, int healthy);
```

**Parameters:**
- `dist`: Packet distributor instance
- `tunnel_id`: ID of tunnel
- `healthy`: Non-zero if healthy, zero if unhealthy

**Returns:**
- `0` on success
- `-1` on error

## NIC Detector API

### nic_detector.h

#### nic_info_t

Structure containing information about a network interface.

```c
typedef struct nic_info {
    char *name;                  /* NIC name */
    char *description;           /* NIC description */
    char *guid;                  /* NIC GUID */
    nic_type_t type;             /* NIC type */
    nic_status_t status;          /* Connection status */
    unsigned long speed;          /* Link speed (Mbps) */
    int index;                   /* Interface index */
} nic_info_t;
```

#### nic_detector_get_count()

Get the number of physical NICs available.

```c
int nic_detector_get_count(void);
```

**Returns:**
- Number of physical NICs
- `-1` on error

#### nic_detector_enumerate()

Enumerate all physical network interfaces.

```c
int nic_detector_enumerate(nic_info_t *nics, int max_count);
```

**Parameters:**
- `nics`: Array to receive NIC information
- `max_count`: Maximum number of NICs to return

**Returns:**
- Number of NICs found
- `-1` on error

**Example:**
```c
nic_info_t nics[10];
int count = nic_detector_enumerate(nics, 10);
for (int i = 0; i < count; i++) {
    printf("NIC: %s (%s)\n", nics[i].name, nics[i].description);
}
```

#### nic_detector_get_by_name()

Get information about a specific NIC by name.

```c
int nic_detector_get_by_name(const char *name, nic_info_t *nic);
```

**Parameters:**
- `name`: NIC name to lookup
- `nic`: Pointer to receive NIC information

**Returns:**
- `0` on success
- `-1` on error

#### nic_detector_is_physical()

Check if a NIC name refers to a physical interface.

```c
int nic_detector_is_physical(const char *name);
```

**Parameters:**
- `name`: NIC name to check

**Returns:**
- Non-zero if physical NIC
- Zero if virtual adapter (TAP, loopback, etc.)

## Types and Enums

### bonding_types.h

#### bonding_mode_t

Bonding distribution mode.

```c
typedef enum {
    BONDING_MODE_ROUND_ROBIN = 0,    /* Round-robin packet distribution */
    BONDING_MODE_WEIGHTED = 1,       /* Weighted distribution */
    BONDING_MODE_ACTIVE_BACKUP = 2   /* Active/backup redundancy */
} bonding_mode_t;
```

#### tunnel_state_t

State of a VPN tunnel.

```c
typedef enum {
    TUNNEL_STATE_DISCONNECTED = 0,
    TUNNEL_STATE_CONNECTING = 1,
    TUNNEL_STATE_CONNECTED = 2,
    TUNNEL_STATE_FAILED = 3
} tunnel_state_t;
```

#### nic_type_t

Type of network interface.

```c
typedef enum {
    NIC_TYPE_ETHERNET = 0,
    NIC_TYPE_WIFI = 1,
    NIC_TYPE_LTE = 2,
    NIC_TYPE_UNKNOWN = 3
} nic_type_t;
```

## Error Handling

All functions follow a consistent error handling pattern:

- **Return values**: Functions return `0` on success, `-1` on error (unless otherwise specified)
- **NULL pointers**: Functions that return pointers return `NULL` on error
- **Error logging**: Errors are logged using the logging system (see `logging.c`)
- **Resource cleanup**: All resources are properly freed even on error

## Thread Safety

- Most functions are **not thread-safe** and should be called from a single thread
- The bonding manager coordinates all operations and should be the primary interface
- IPC functions handle thread synchronization internally

## Memory Management

- All allocated memory must be freed using corresponding free/destroy functions
- String parameters are copied internally where needed
- Caller is responsible for freeing returned strings unless otherwise documented
