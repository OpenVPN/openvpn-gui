/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef COUNTRY_ROUTES_H
#define COUNTRY_ROUTES_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define COUNTRY_ROUTE_LIMIT 100000

typedef struct
{
    uint32_t first, last;
} country_range_t;

typedef struct
{
    country_range_t *ranges;
    size_t count, capacity;
} country_routes_t;

/* Input must contain canonical, public IPv4 CIDRs, one per line. */
bool CountryRoutesRead(FILE *input, country_routes_t *routes);
bool CountryRoutesWrite(FILE *output, country_routes_t *routes);
void CountryRoutesFree(country_routes_t *routes);

#endif
