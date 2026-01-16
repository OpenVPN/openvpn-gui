/**
 * @file nic_detector.c
 * @brief NIC detection and enumeration implementation
 * 
 * TODO: Implement Windows API-based NIC detection
 */

#include "nic_detector.h"
#include <windows.h>
#include <iphlpapi.h>

int nic_detector_get_count(void)
{
    /* TODO: Implement NIC count retrieval using GetAdaptersInfo */
    return 0;
}

int nic_detector_enumerate(nic_info_t *nics, int max_count)
{
    /* TODO: Implement NIC enumeration using GetAdaptersInfo */
    return -1;
}

int nic_detector_get_by_name(const char *name, nic_info_t *nic)
{
    /* TODO: Implement NIC lookup by name */
    return -1;
}

int nic_detector_is_physical(const char *name)
{
    /* TODO: Implement physical NIC detection (filter out TAP, loopback, etc.) */
    return 0;
}
