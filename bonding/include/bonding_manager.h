#ifndef BONDING_MANAGER_H
#define BONDING_MANAGER_H

#include "bonding_config.h"

/**
 * @file bonding_manager.h
 * @brief Main bonding manager interface
 */

/* Forward declaration */
struct bonding_manager;

/**
 * @brief Bonding manager handle
 */
typedef struct bonding_manager bonding_manager_t;

/* Manager functions (to be implemented) */
bonding_manager_t* bonding_manager_create(void);
int bonding_manager_start(bonding_manager_t *mgr, bonding_profile_t *profile);
int bonding_manager_stop(bonding_manager_t *mgr);
int bonding_manager_get_status(bonding_manager_t *mgr, tunnel_state_t *states, int max_tunnels);
void bonding_manager_destroy(bonding_manager_t *mgr);

#endif /* BONDING_MANAGER_H */
