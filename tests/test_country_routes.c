/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "country_routes.h"
#include <stdlib.h>
#include <string.h>

#define CHECK(expr)                                                 \
    do                                                              \
    {                                                               \
        if (!(expr))                                                \
        {                                                           \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expr); \
            exit(1);                                                \
        }                                                           \
    } while (0)

static bool
read_bytes(country_routes_t *routes, const char *data, size_t size)
{
    FILE *input = tmpfile();
    CHECK(input);
    CHECK(fwrite(data, 1, size, input) == size);
    rewind(input);
    bool result = CountryRoutesRead(input, routes);
    fclose(input);
    return result;
}

static bool
read_text(country_routes_t *routes, const char *data)
{
    return read_bytes(routes, data, strlen(data));
}

int
main(int argc, char **argv)
{
    country_routes_t routes = { 0 };
    /* Also acts as an offline converter for real-provider and property tests. */
    if (argc == 2)
    {
        FILE *input = fopen(argv[1], "rb");
        if (!input)
        {
            return 2;
        }
        bool result = CountryRoutesRead(input, &routes);
        fclose(input);
        result = result && CountryRoutesWrite(stdout, &routes);
        CountryRoutesFree(&routes);
        return result ? 0 : 1;
    }
    CHECK(read_text(&routes, "8.8.9.0/24\n8.8.8.0/24\n8.8.8.0/25\n"));
    CHECK(read_text(&routes, "  1.1.1.1/32\r\n\r\n1.1.1.1/32"));
    FILE *output = tmpfile();
    CHECK(output && CountryRoutesWrite(output, &routes));
    rewind(output);
    char result[512] = { 0 };
    CHECK(fread(result, 1, sizeof(result) - 1, output) > 0);
    CHECK(strcmp(result,
                 "route 1.1.1.1 255.255.255.255 net_gateway\n"
                 "route 8.8.8.0 255.255.254.0 net_gateway\n")
          == 0);
    fclose(output);
    size_t previous = routes.count;
    const char *invalid[] = {
        "",
        "\r\n \t\n",
        "route 8.8.8.0 255.255.255.0 net_gateway\n",
        "8.8.8.0/24 up malicious\n",
        "8.8.8.0/24\nscript-security 2\n",
        "8.8.8.0/24\n9.0.0.0/8\ninvalid\n",
        "8.8.8.1/24\n",
        "256.0.0.0/8\n",
        "-1.0.0.0/8\n",
        "8.0.0.0/33\n",
        "8.0.0.0/-1\n",
        "0.0.0.0/0\n",
        "128.0.0.0/1\n",
        "0.0.0.0/2\n",
        "224.0.0.0/4\n",
        "10.0.0.0/8\n",
        "172.16.0.0/12\n",
        "192.168.0.0/16\n",
        "127.0.0.1/32\n",
        "169.254.0.0/16\n",
        "100.64.0.0/10\n",
        "2001:db8::/32\n",
        "<html>server error</html>\n",
        "8.8.8.0/24#comment\n",
        "9999999999999999999999999999999999999999999999999999999999999999999999999999999999\n"
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
    {
        CHECK(!read_text(&routes, invalid[i]));
        CHECK(routes.count == previous); /* failed input must roll back */
    }
    const char hidden[] = "8.8.8.0/24\0up malicious\n";
    CHECK(!read_bytes(&routes, hidden, sizeof(hidden) - 1));
    CHECK(routes.count == previous);
    CountryRoutesFree(&routes);

    CHECK(read_text(&routes, "223.255.255.255/32\n192.0.2.0/31\n8.0.0.0/7\n"));
    CHECK(routes.ranges[0].first == UINT32_C(0xdfffffff));
    output = tmpfile();
    CHECK(output && CountryRoutesWrite(output, &routes));
    fclose(output);
    CountryRoutesFree(&routes);

    FILE *large = tmpfile();
    CHECK(large);
    for (size_t i = 0; i <= COUNTRY_ROUTE_LIMIT; ++i)
    {
        fputs("8.8.8.0/24\n", large);
    }
    rewind(large);
    CHECK(!CountryRoutesRead(large, &routes));
    CHECK(routes.count == 0);
    fclose(large);
    CountryRoutesFree(&routes);
    puts("Country route validation, rollback, merging, and limits passed.");
    return 0;
}
