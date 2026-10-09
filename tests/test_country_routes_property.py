"""Compare generated route coverage against Python's independent IP network oracle."""
import ipaddress
import pathlib
import random
import subprocess
import sys
import tempfile

rng = random.Random(76231)
converter = pathlib.Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory() as directory:
    source = pathlib.Path(directory) / "test.zone"
    for case in range(100):
        networks = []
        for _ in range(rng.randrange(1, 500)):
            prefix = rng.randrange(12, 33)
            address = (8 << 24) | rng.getrandbits(24)
            networks.append(ipaddress.IPv4Network((address, prefix), strict=False))
        # Inject duplicates and contained subnets; they must never broaden coverage.
        networks += networks[:10]
        source.write_text("\n".join(map(str, networks)) + "\n", encoding="ascii")
        output = subprocess.check_output([str(converter), str(source)], text=True)
        actual = []
        for line in output.splitlines():
            directive, address, mask, gateway = line.split()
            assert directive == "route" and gateway == "net_gateway"
            actual.append(ipaddress.IPv4Network(f"{address}/{mask}"))
        expected = list(ipaddress.collapse_addresses(networks))
        assert actual == expected, f"coverage mismatch in case {case}"
print("100 randomized route coverage cases passed.")
