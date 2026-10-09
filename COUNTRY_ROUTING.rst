Country routing
===============

Settings -> Routing adds an optional, app-wide IPv4 VPN bypass. Check the
countries whose destination IP ranges should use the normal internet gateway,
then choose "Update selected ranges". Press OK to save. Disconnect and connect
again to use the new selection. The setting is off by default.

Enter a country name or two-letter code and click Search to jump to a match.
Names are matched case-insensitively; exact codes such as IR take precedence.
Click Search again to move to the next matching name, wrapping at the end.
Search highlights a row without changing any country checkboxes.

The selection is stored once in HKCU\Software\OpenVPN-GUI, independently of
connection profiles. No .ovpn file is modified. Existing profile routing still
determines how traffic outside the selected countries is handled.

Country data
------------

The update button fetches aggregated IPv4 CIDR files over HTTPS from IPdeny:

    https://www.ipdeny.com/ipblocks/data/aggregated/ir-aggregated.zone

Replace ``ir`` with the selected country's lower-case ISO code. The catalogue
contains the 233 country/territory entries listed by IPdeny at implementation
time. No IP database is bundled. Updates happen only on request; connecting
uses the offline cache in ``%LOCALAPPDATA%\OpenVPN-GUI\country-routes``.
Settings shows cache coverage and the oldest local download date, which is
not the provider's publication date. Update periodically to follow changes.

These lists reflect regional registry allocations, not authoritative physical
locations. An Iranian website hosted abroad may not bypass the VPN.

Provider links:

* Country data: https://www.ipdeny.com/ipblocks/
* Copyright: https://www.ipdeny.com/copyright.php
* Terms: https://www.ipdeny.com/tos.php
* Usage limits: https://www.ipdeny.com/usagelimits.php

Downloads are sequential with 500 ms spacing, following fair-use guidance.
Downloaded data is not part of the GPL source distribution. Preserve the
provider's Copyrights.txt notice if redistributing its zone files.

Behavior and limits
-------------------

* OpenVPN 2 engine only. OpenVPN 3 agent mode is rejected while bypass is on.
* IPv4 only. IPv6 and DNS retain the profile's original behavior. IPv6
  destinations in a selected country may still use the VPN. Generic Windows
  IPv6 routes do not support the core's IPv4 ``net_gateway`` bypass mechanism.
* Applies to new OpenVPN processes started by the GUI. A reconnect within an
  existing process retains its original selection. Service-started persistent
  profiles and pre-logon connections are outside this setting.
* One active VPN at a time when enabled. Another GUI connection is refused to
  avoid gateway/route ownership conflicts. Also disconnect other VPN apps.
* The interactive service requires normal OpenVPN administrator-group
  authorization for the generated user-owned configuration. The existing
  authorization dialog is used; service access checks are not bypassed.
* More specific profile/server routes and firewall or kill-switch policies
  can override or prevent bypass. Large lists can slow Windows route operations.

Implementation
--------------

``country_routes.c`` accepts canonical IPv4 CIDRs only. It rejects embedded
NULs, malformed input, default/half-default routes, private/local address space,
and OpenVPN directives. Limits are 4 MiB per file and 100,000 input CIDRs across
the selection. Overlapping/adjacent ranges are merged without expanding their
union and emitted as numeric ``route`` directives using ``net_gateway``.
Data is parsed both on update and on connect.

The WinHTTP updater runs in a worker thread with normal TLS validation, finite
timeouts, and no redirects. Each country is validated before atomic cache
replacement. A failed country's previous cache is retained; other countries
in the request may already have updated. Cancelling Settings requests worker
cancellation and does not save the draft selection.

The GUI supplies a separate temporary routes-only config alongside the original
profile. A handle allows readers but denies modification/deletion while the
OpenVPN process may use it. The file is closed and deleted after exit or a
failed launch. OpenVPN manages route installation, reconnects, and removal.
Missing/invalid cache files prevent connection with an explicit update message.
No online country lookup occurs per packet.

Validation
----------

With MSVC/CMake, run the offline tests:

.. code-block:: powershell

    cmake -S . -B out/build/local -G "Visual Studio 17 2022" -A x64
    cmake --build out/build/local --config RelWithDebInfo
    ctest --test-dir out/build/local -C RelWithDebInfo --output-on-failure

Tests cover CIDR parsing, rollback, masks, merging, boundary addresses, NULs,
and limits. When Python is available, 100 deterministic randomized cases are
compared against ``ipaddress.collapse_addresses``. A hidden native Windows
dialog test checks country checkboxes, global preference serialization, route
file locking/cleanup, invalid data, and unsupported modes. Test cache/registry
are isolated from the user's real settings.

To exercise a live HTTPS download (excluded from automated tests):

.. code-block:: powershell

    .\out\build\local\RelWithDebInfo\test_country_routing_win.exe --download

For a live tunnel test, import a working VPN profile and compare a known Iranian
IPv4 destination and a destination outside the selection with bypass off/on.
Inspect the route log and Windows routing table during the tunnel, after
reconnect, and after disconnect. Confirm the original profile contents stay
unchanged. The offline suite does not establish a tunnel or change system routes.
