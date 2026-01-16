#ifndef OPENVPN_MANAGER_H
#define OPENVPN_MANAGER_H

#include "bonding_types.h"

/**
 * @file openvpn_manager.h
 * @brief Multi-instance OpenVPN process manager
 */

/* Forward declaration */
struct openvpn_manager;

/**
 * @brief OpenVPN manager handle
 */
typedef struct openvpn_manager openvpn_manager_t;

/* Process management functions (to be implemented) */
openvpn_manager_t* openvpn_manager_create(void);
int openvpn_manager_spawn_instance(openvpn_manager_t *mgr, int tunnel_id, const char *config_path, const char *tap_adapter);
int openvpn_manager_stop_instance(openvpn_manager_t *mgr, int tunnel_id);
int openvpn_manager_get_instance_status(openvpn_manager_t *mgr, int tunnel_id, tunnel_state_t *state);
void openvpn_manager_destroy(openvpn_manager_t *mgr);

#endif /* OPENVPN_MANAGER_H */
