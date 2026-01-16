/**
 * @file service_ipc.c
 * @brief IPC communication between GUI and service
 * 
 * TODO: Implement named pipe communication
 */

#include <windows.h>
#include "bonding_config.h"

int service_ipc_init(void)
{
    /* TODO: Implement named pipe server initialization */
    return -1;
}

int service_ipc_receive_config(bonding_profile_t *profile)
{
    /* TODO: Implement configuration reception from GUI */
    return -1;
}

int service_ipc_send_status(tunnel_state_t *states, int tunnel_count)
{
    /* TODO: Implement status transmission to GUI */
    return -1;
}

void service_ipc_cleanup(void)
{
    /* TODO: Implement IPC cleanup */
}
