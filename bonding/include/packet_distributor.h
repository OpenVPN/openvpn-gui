#ifndef PACKET_DISTRIBUTOR_H
#define PACKET_DISTRIBUTOR_H

#include "bonding_types.h"

/**
 * @file packet_distributor.h
 * @brief Packet distribution across tunnels
 */

/* Forward declaration */
struct packet_distributor;

/**
 * @brief Packet distributor handle
 */
typedef struct packet_distributor packet_distributor_t;

/* Distribution functions (to be implemented) */
packet_distributor_t* packet_distributor_create(bonding_mode_t mode, int tunnel_count);
int packet_distributor_select_tunnel(packet_distributor_t *dist, int *tunnel_id);
int packet_distributor_set_tunnel_weight(packet_distributor_t *dist, int tunnel_id, int weight);
int packet_distributor_set_tunnel_health(packet_distributor_t *dist, int tunnel_id, int healthy);
void packet_distributor_destroy(packet_distributor_t *dist);

#endif /* PACKET_DISTRIBUTOR_H */
