#ifndef NIC_DETECTOR_H
#define NIC_DETECTOR_H

#include "bonding_types.h"

/**
 * @file nic_detector.h
 * @brief Physical NIC detection and enumeration
 */

/* Forward declaration */
struct nic_info;

/**
 * @brief NIC information structure
 */
typedef struct nic_info {
    char *name;                  /* NIC name */
    char *description;           /* NIC description */
    char *guid;                  /* NIC GUID */
    nic_type_t type;             /* NIC type */
    nic_status_t status;          /* Connection status */
    unsigned long speed;          /* Link speed (Mbps) */
    int index;                   /* Interface index */
} nic_info_t;

/* Detection functions (to be implemented) */
int nic_detector_get_count(void);
int nic_detector_enumerate(nic_info_t *nics, int max_count);
int nic_detector_get_by_name(const char *name, nic_info_t *nic);
int nic_detector_is_physical(const char *name);

#endif /* NIC_DETECTOR_H */
