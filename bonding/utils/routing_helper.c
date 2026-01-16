/**
 * @file routing_helper.c
 * @brief Windows routing API helper functions
 * 
 * TODO: Implement routing table manipulation
 */

#include <windows.h>
#include <iphlpapi.h>

int routing_helper_bind_to_nic(const char *nic_name, const char *destination)
{
    /* TODO: Implement route creation to bind traffic to specific NIC */
    return -1;
}

int routing_helper_remove_binding(const char *nic_name, const char *destination)
{
    /* TODO: Implement route removal */
    return -1;
}

int routing_helper_restore_routes(void)
{
    /* TODO: Implement route restoration on shutdown */
    return -1;
}
