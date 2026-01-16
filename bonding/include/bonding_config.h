#ifndef BONDING_CONFIG_H
#define BONDING_CONFIG_H

#include "bonding_types.h"

/**
 * @file bonding_config.h
 * @brief Configuration structures for bonding profiles
 */

/* Forward declarations */
struct bonding_profile;
struct tunnel_config;

/**
 * @brief Configuration structure for a single tunnel
 */
typedef struct tunnel_config {
    char *nic_name;              /* Physical NIC name */
    char *tap_adapter;           /* TAP adapter name */
    char *server_host;           /* Server hostname/IP */
    int server_port;             /* Server port */
    char *config_file;           /* OpenVPN config file path */
    int weight;                  /* Distribution weight (for weighted mode) */
    tunnel_state_t state;        /* Current tunnel state */
} tunnel_config_t;

/**
 * @brief Main bonding profile configuration
 */
typedef struct bonding_profile {
    char *profile_name;          /* Profile name */
    bonding_mode_t mode;         /* Bonding mode */
    int tunnel_count;            /* Number of tunnels */
    tunnel_config_t *tunnels;    /* Array of tunnel configurations */
    char *config_path;           /* Path to .ovpn-bond file */
} bonding_profile_t;

/* Configuration functions (to be implemented) */
bonding_profile_t* bonding_config_load(const char *config_path);
int bonding_config_save(bonding_profile_t *profile, const char *config_path);
void bonding_config_free(bonding_profile_t *profile);

#endif /* BONDING_CONFIG_H */
