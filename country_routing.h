/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef COUNTRY_ROUTING_H
#define COUNTRY_ROUTING_H

#include "options.h"

INT_PTR CALLBACK CountryRoutingDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
BOOL PrepareCountryRouting(connection_t *c);
void CleanupCountryRouting(connection_t *c);

#endif
