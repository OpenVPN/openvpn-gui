/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "country_routes.h"
#include <stdlib.h>
#include <string.h>

static bool
public_range(uint32_t first, uint32_t last)
{
    static const country_range_t reserved[] = {
        { 0x00000000, 0x00ffffff }, /* unspecified */
        { 0x0a000000, 0x0affffff }, /* private */
        { 0x64400000, 0x647fffff }, /* shared address space */
        { 0x7f000000, 0x7fffffff }, /* loopback */
        { 0xa9fe0000, 0xa9feffff }, /* link-local */
        { 0xac100000, 0xac1fffff }, /* private */
        { 0xc0a80000, 0xc0a8ffff }, /* private */
        { 0xe0000000, 0xffffffff }  /* multicast and reserved */
    };
    for (size_t i = 0; i < sizeof(reserved) / sizeof(reserved[0]); ++i)
    {
        if (first <= reserved[i].last && last >= reserved[i].first)
        {
            return false;
        }
    }
    return true;
}

static bool
parse_cidr(const char *line, country_range_t *range)
{
    unsigned int octets[4], prefix = 0;
    const char *p = line;
    for (unsigned int i = 0; i < 5; ++i)
    {
        unsigned int value = 0, digits = 0;
        while (*p >= '0' && *p <= '9')
        {
            value = value * 10 + (unsigned int)(*p++ - '0');
            if (++digits > 3 || value > (i == 4 ? 32u : 255u))
            {
                return false;
            }
        }
        if (!digits)
        {
            return false;
        }
        if (i < 4)
        {
            octets[i] = value;
            if (*p++ != (i == 3 ? '/' : '.'))
            {
                return false;
            }
        }
        else
        {
            prefix = value;
        }
    }
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
    {
        ++p;
    }
    if (*p || prefix < 2)
    {
        return false;
    }
    uint32_t mask = UINT32_MAX << (32 - prefix);
    range->first = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    range->last = range->first | ~mask;
    return !(range->first & ~mask) && public_range(range->first, range->last);
}

bool
CountryRoutesRead(FILE *input, country_routes_t *routes)
{
    char line[80];
    size_t initial_count = routes->count;
    size_t total = 0;
    for (;;)
    {
        size_t length = 0;
        int ch;
        while ((ch = fgetc(input)) != EOF && ch != '\n')
        {
            if (!ch || length == sizeof(line) - 1 || ++total > 4 * 1024 * 1024)
            {
                routes->count = initial_count;
                return false;
            }
            line[length++] = (char)ch;
        }
        if (ch == EOF && !length)
        {
            break;
        }
        if (++total > 4 * 1024 * 1024)
        {
            routes->count = initial_count;
            return false;
        }
        line[length] = '\0';
        const char *p = line;
        while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
        {
            ++p;
        }
        if (!*p)
        {
            continue;
        }
        country_range_t range;
        if (!parse_cidr(p, &range) || routes->count == COUNTRY_ROUTE_LIMIT)
        {
            routes->count = initial_count;
            return false;
        }
        if (routes->count == routes->capacity)
        {
            size_t capacity = routes->capacity ? routes->capacity * 2 : 256;
            country_range_t *ranges = realloc(routes->ranges, capacity * sizeof(*ranges));
            if (!ranges)
            {
                routes->count = initial_count;
                return false;
            }
            routes->ranges = ranges;
            routes->capacity = capacity;
        }
        routes->ranges[routes->count++] = range;
    }
    if (ferror(input) || routes->count == initial_count)
    {
        routes->count = initial_count;
        return false;
    }
    return true;
}

static int
compare_range(const void *a, const void *b)
{
    const country_range_t *ra = a, *rb = b;
    return ra->first < rb->first ? -1 : ra->first > rb->first ? 1 : 0;
}

bool
CountryRoutesWrite(FILE *output, country_routes_t *routes)
{
    if (!routes->count)
    {
        return false;
    }
    qsort(routes->ranges, routes->count, sizeof(*routes->ranges), compare_range);
    size_t merged = 0;
    for (size_t i = 0; i < routes->count; ++i)
    {
        country_range_t r = routes->ranges[i];
        if (merged && (uint64_t)r.first <= (uint64_t)routes->ranges[merged - 1].last + 1)
        {
            if (r.last > routes->ranges[merged - 1].last)
            {
                routes->ranges[merged - 1].last = r.last;
            }
        }
        else
        {
            routes->ranges[merged++] = r;
        }
    }
    routes->count = merged;
    for (size_t i = 0; i < merged; ++i)
    {
        uint64_t first = routes->ranges[i].first, last = routes->ranges[i].last;
        while (first <= last)
        {
            uint64_t size = 1;
            while (size < (UINT64_C(1) << 32) && !(first & (size * 2 - 1))
                   && first + size * 2 - 1 <= last)
            {
                size *= 2;
            }
            uint32_t ip = (uint32_t)first, mask = ~(uint32_t)(size - 1);
            if (fprintf(output,
                        "route %u.%u.%u.%u %u.%u.%u.%u net_gateway\n",
                        ip >> 24,
                        (ip >> 16) & 255,
                        (ip >> 8) & 255,
                        ip & 255,
                        mask >> 24,
                        (mask >> 16) & 255,
                        (mask >> 8) & 255,
                        mask & 255)
                < 0)
            {
                return false;
            }
            first += size;
        }
    }
    return !ferror(output);
}

void
CountryRoutesFree(country_routes_t *routes)
{
    free(routes->ranges);
    memset(routes, 0, sizeof(*routes));
}
