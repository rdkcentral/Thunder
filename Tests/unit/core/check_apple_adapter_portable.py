#!/usr/bin/env python3
"""Check portable Apple iterator excerpts, not native Apple integration.

Run with Linux Python 3 and g++/AddressSanitizer. Reuse the ownership harness's
allocation fixtures; do not define __APPLE__, emulate Darwin socket layouts,
or link Thunder Core. ToString is a narrow test shim, not Serialization code.
"""

import argparse
import hashlib
import os
from pathlib import Path
import tempfile

import check_apple_adapter_ownership as ownership


def _between(text, start, end):
    """Return a unique bounded excerpt from text; reject ambiguous markers."""
    if text.count(start) != 1:
        raise RuntimeError("Missing or ambiguous extraction start: " + start)
    tail = text.split(start, 1)[1]
    if end not in tail:
        raise RuntimeError("Missing extraction end: " + end)
    return start + tail.split(end, 1)[0]


def _extract(source, header):
    """Return unchanged portable classes and Apple bodies from source strings."""
    branch = _between(
        source, "#elif defined(__APPLE__)\n    // Each snapshot",
        "#elif defined(__LINUX__)",
    )
    # Exclude the separate Linux classes without activating platform macros.
    header = _between(
        header, "#if defined(__WINDOWS__) || defined(__APPLE__)",
        "\n#else\n\n    class EXTERNAL IPV4AddressIterator",
    )
    shared = _between(
        header, "    class EXTERNAL IPV4AddressIterator {",
        "#if defined(__WINDOWS__)\n    class EXTERNAL IPV6AddressIterator",
    )
    # Only the common Apple/Windows IPv4 class is selected, never Windows IPv6.
    adapters = _between(
        header + "\nPORTABLE_HEADER_END\n",
        "    class EXTERNAL AdapterIterator {\n    public:\n        static uint8_t constexpr MacSize",
        "\nPORTABLE_HEADER_END\n",
    )
    constructor = _between(
        branch, "    IPV4AddressIterator::IPV4AddressIterator",
        "    IPNode IPV4AddressIterator::Address",
    )
    consumers = _between(
        branch, "    uint16_t AdapterIterator::Count() const",
        "    string AdapterIterator::MACAddress",
    )
    formatter = _between(
        branch, "    inline void ConvertMACToString",
        "    static uint8_t LoadAdapterInfo",
    )
    return (
        shared + "\nusing IPV6AddressIterator = IPV4AddressIterator;\n" + adapters,
        constructor, consumers, formatter,
    )


SHIMS = r"""
#define EXTERNAL
#define ASSERT(condition) REQUIRE(condition)
#define _T(value) value
class IPNode;
// Unused network-control signatures need declarations, not mock definitions.
class NodeId;
namespace Core { class NodeId; }
// PUBLIC_INTERFACE
/** Test-only narrow conversion: copy input text to output; returns void. */
static void ToString(const char* input, string& output) { output = input; }
"""

CHECKS = r"""
#undef getifaddrs
#undef freeifaddrs

/** Assert empty iterator state for the supplied iterator; returns void. */
static void empty_state(IPV4AddressIterator& iterator)
{
    REQUIRE(iterator.Count() == 0);
    REQUIRE(!iterator.IsValid());
    REQUIRE(!iterator.Next());
    iterator.Reset();
    REQUIRE(!iterator.IsValid());
}

// PUBLIC_INTERFACE
/** Execute five portable scenarios without changing host networking; return 0. */
int main()
{
    {
        IPV4AddressIterator initial;
        IPV4AddressIterator indexed(0);
        empty_state(initial);
        empty_state(indexed);
        IPV4AddressIterator copied(indexed);
        IPV4AddressIterator assigned;
        assigned = indexed;
        IPV4AddressIterator moved(std::move(copied));
        IPV4AddressIterator move_assigned;
        move_assigned = std::move(assigned);
        empty_state(moved);
        empty_state(move_assigned);
        empty_state(copied);
        empty_state(assigned);
        indexed = indexed;
        indexed = std::move(indexed);
        empty_state(indexed);
    }
    std::cout << "PASS empty address iterator/copy/move/reset/self-assignment\n";
    {
        mode = Synthetic;
        AdapterIterator iterator;
        REQUIRE(iterator.Count() == 2 && !iterator.IsValid());
        REQUIRE(iterator.Next() && iterator.Index() == 0);
        REQUIRE(iterator.Name() == "alpha");
        AdapterIterator copy(iterator);
        AdapterIterator moved(std::move(copy));
        REQUIRE(!copy.IsValid() && moved.Name() == "alpha");
        AdapterIterator assigned;
        assigned = moved;
        REQUIRE(assigned.Name() == "alpha");
        AdapterIterator move_assigned;
        move_assigned = std::move(assigned);
        REQUIRE(!assigned.IsValid() && move_assigned.Name() == "alpha");
        REQUIRE(iterator.Next() && iterator.Name() == "zeta");
        REQUIRE(!iterator.Next() && !iterator.Next());
        iterator.Reset();
        REQUIRE(!iterator.IsValid() && iterator.Next());
        REQUIRE(AdapterIterator(string("zeta")).Index() == 1);
        REQUIRE(AdapterIterator(uint16_t(1)).Name() == "zeta");
        REQUIRE(!AdapterIterator(string("absent")).IsValid());
        REQUIRE(!AdapterIterator(uint16_t(99)).IsValid());
        REQUIRE(live == 0 && allocations.empty());
    }
    std::cout << "PASS adapter traversal/lookup/copy/move/name lifetime\n";
    {
        mode = Empty;
        AdapterIterator iterator;
        REQUIRE(iterator.Count() == 0 && !iterator.Next());
        mode = Failure;
        REQUIRE(iterator.Count() == 0 && !iterator.Next());
        REQUIRE(live == 0 && allocations.empty());
    }
    std::cout << "PASS adapter empty/failure enumeration\n";
    {
        // Formatting alone is portable; this deliberately does not extract MACs.
        const uint8_t bytes[] = { 0x00, 0x01, 0x0a, 0x7f, 0x80, 0xff };
        string output;
        ConvertMACToString(bytes, 6, ':', output);
        REQUIRE(output == "00:01:0A:7F:80:FF");
        output.clear();
        ConvertMACToString(bytes, 6, '\0', output);
        REQUIRE(output == "00010A7F80FF");
        output = "prefix";
        ConvertMACToString(nullptr, 0, ':', output);
        REQUIRE(output == "prefix");
    }
    std::cout << "PASS MAC formatter vectors (not MAC extraction)\n";
    {
        mode = Real;
        for (unsigned attempt = 0; attempt < 20; ++attempt) {
            AdapterIterator iterator;
            unsigned seen = 0;
            while (iterator.Next()) {
                REQUIRE(!iterator.Name().empty());
                AdapterIterator copied(iterator);
                REQUIRE(copied.Name() == iterator.Name());
                ++seen;
            }
            REQUIRE(seen > 0 && seen == iterator.Count());
            REQUIRE(live == 0 && allocations.empty());
        }
    }
    std::cout << "PASS real Linux name traversal (20 repetitions)\n";
    std::cout << "5 portable scenarios passed; NOT native Apple integration\n";
}
"""


# PUBLIC_INTERFACE
def main():
    """Run extracted portable scenarios and two negative controls; return zero."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()
    if not os.sys.platform.startswith("linux"):
        parser.error("Run in Linux/WSL; this is not an Apple integration runner")
    root = Path(__file__).resolve().parents[3]
    source_bytes = (root / "Source/core/NetworkInfo.cpp").read_bytes()
    header_bytes = (root / "Source/core/NetworkInfo.h").read_bytes()
    source = source_bytes.decode("utf-8").replace("\r\n", "\n")
    header = header_bytes.decode("utf-8").replace("\r\n", "\n")
    declaration, loader = ownership._extract(source)
    classes, constructor, consumers, formatter = _extract(source, header)
    excerpts = declaration + classes + formatter + loader + constructor + consumers
    print("SOURCE_SHA256:", hashlib.sha256(source_bytes).hexdigest())
    print("HEADER_SHA256:", hashlib.sha256(header_bytes).hexdigest())
    print("EXTRACT_SHA256:", hashlib.sha256(excerpts.encode()).hexdigest())
    print("PLATFORM:", os.uname())
    base = ownership.PREFIX + SHIMS + excerpts
    with tempfile.TemporaryDirectory(prefix="oi03-portable-") as directory:
        def run(label, code):
            """Compile code with ASan, report captured diagnostics; return result."""
            result = ownership._compile_and_run(
                args.compiler, code, Path(directory) / label,
            )
            print(result.stdout, end="")
            print(result.stderr, end="")
            print(label.upper() + "_EXIT:", result.returncode)
            return result

        fixed = run("portable", base + CHECKS)
        if fixed.returncode != 0 or fixed.stderr:
            raise RuntimeError("Portable check failed or emitted diagnostics")
        # The first state assertion must reject a deliberately wrong count.
        if constructor.count(", _section3(0)") != 1:
            raise RuntimeError("Constructor mutation boundary changed")
        wrong_state = base.replace(constructor, constructor.replace(
            ", _section3(0)", ", _section3(1)", 1), 1)
        state = run("state_negative", wrong_state + CHECKS)
        if state.returncode != 2 or "FAILED: iterator.Count() == 0" not in state.stderr:
            raise RuntimeError("State negative control was not detected")
        # Actual extracted Name must read freed storage when retention is removed.
        broken = base.replace(loader, loader.replace(
            "addresses.interfaces = owner;", "", 1), 1)
        lifetime = run("name_negative", broken + CHECKS)
        if lifetime.returncode != 86 or "ERROR: AddressSanitizer: heap-use-after-free" not in lifetime.stderr:
            raise RuntimeError("Name lifetime negative control was not detected")
    print("PORTABLE_CHECK_PASS: excerpts/shims only; native Apple test NOT executed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
