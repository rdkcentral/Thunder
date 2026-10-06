#!/usr/bin/env python3
"""Check extracted Apple snapshot ownership on Linux, not native Apple integration.

Run from any directory with Python 3 and g++ supporting AddressSanitizer.
The generated translation unit uses Linux ifaddrs; it does not define __APPLE__,
compile Thunder Core, or validate Darwin sockaddr/MAC/NodeId behavior.
"""

import argparse
import hashlib
import os
from pathlib import Path
import subprocess
import tempfile


def _extract(source):
    """Return unchanged snapshot declaration and loader from source text."""
    apple = source.split("#elif defined(__APPLE__)\n    // Each snapshot", 1)
    if len(apple) != 2:
        raise RuntimeError("Apple snapshot boundary changed; review extraction")
    branch = apple[1].split("#elif defined(__LINUX__)", 1)[0]
    declaration = branch[branch.index("    struct AdapterAddresses"):branch.index(
        "    inline void ConvertMACToString")]
    loader = branch[branch.index("    static uint8_t LoadAdapterInfo"):branch.index(
        "    IPV4AddressIterator::IPV4AddressIterator")]
    if declaration.count("struct AdapterAddresses") != 1 or loader.count(
            "static uint8_t LoadAdapterInfo") != 1:
        raise RuntimeError("Ambiguous source extraction")
    if "addresses.interfaces = owner;" not in loader:
        raise RuntimeError("Expected ownership retention changed; review negative control")
    return declaration, loader


PREFIX = r"""
#include <ifaddrs.h>
#include <netinet/in.h>
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>
using string = std::string;
#define REQUIRE(condition) do { if (!(condition)) { \
    std::cerr << "FAILED: " << #condition << '\n'; std::exit(2); } } while (0)
static unsigned live = 0;
static unsigned released = 0;
enum Mode { Synthetic, Empty, Failure, Real };
static Mode mode = Synthetic;
static std::map<ifaddrs*, bool> allocations;

/** Return a synthetic list or Linux libc list; output is tracked for release. */
static int fixture_getifaddrs(ifaddrs** output)
{
    *output = nullptr;
    if (mode == Failure) { return -1; }
    if (mode == Empty) { return 0; }
    if (mode == Real) {
        const int result = ::getifaddrs(output);
        if (result == 0 && *output != nullptr) {
            allocations[*output] = true;
            ++live;
        }
        return result;
    }
    // Duplicate names, a null name, and a null address exercise sparse grouping.
    const char* names[] = { "zeta", nullptr, "alpha", "alpha" };
    ifaddrs** next = output;
    for (unsigned index = 0; index < 4; ++index) {
        *next = static_cast<ifaddrs*>(std::calloc(1, sizeof(ifaddrs)));
        REQUIRE(*next != nullptr);
        if (names[index] != nullptr) {
            (*next)->ifa_name = ::strdup(names[index]);
            REQUIRE((*next)->ifa_name != nullptr);
        }
        if (index == 0 || index == 2) {
            auto* address = static_cast<sockaddr_in*>(std::calloc(1, sizeof(sockaddr_in)));
            REQUIRE(address != nullptr);
            address->sin_family = AF_INET;
            address->sin_addr.s_addr = 0x0100007f;
            (*next)->ifa_addr = reinterpret_cast<sockaddr*>(address);
        }
        next = &(*next)->ifa_next;
    }
    allocations[*output] = false;
    ++live;
    return 0;
}

/** Release exactly the matching Linux or synthetic supplying allocation. */
static void fixture_freeifaddrs(ifaddrs* head)
{
    if (head == nullptr) { return; }
    auto entry = allocations.find(head);
    REQUIRE(entry != allocations.end());
    const bool real = entry->second;
    allocations.erase(entry);
    --live;
    ++released;
    if (real) {
        ::freeifaddrs(head);
    } else {
        while (head != nullptr) {
            ifaddrs* next = head->ifa_next;
            std::free(head->ifa_name);
            std::free(head->ifa_addr);
            std::free(head);
            head = next;
        }
    }
}
#define getifaddrs fixture_getifaddrs
#define freeifaddrs fixture_freeifaddrs
"""

POSITIVE = r"""
#undef getifaddrs
#undef freeifaddrs

/** Inspect borrowed name/address bytes after loader-local owners are gone. */
static void consume(const AdapterAddresses& addresses)
{
    REQUIRE(!addresses.empty());
    REQUIRE(addresses.interfaces != nullptr);
    for (const auto* entry : addresses) {
        REQUIRE(entry->ifa_name != nullptr);
        REQUIRE(std::strlen(entry->ifa_name) > 0);
        if (entry->ifa_addr != nullptr && entry->ifa_addr->sa_family == AF_INET) {
            const auto* address = reinterpret_cast<const sockaddr_in*>(entry->ifa_addr);
            volatile uint32_t value = address->sin_addr.s_addr;
            (void)value;
        }
    }
}

// PUBLIC_INTERFACE
/** Execute seven ownership scenarios; return zero only if all assertions hold. */
int main()
{
    {
        AdapterAddresses selected;
        REQUIRE(LoadAdapterInfo(0, selected) == 2);
        REQUIRE(selected.size() == 2);
        REQUIRE(string(selected[0]->ifa_name) == "alpha");
        REQUIRE(selected[1]->ifa_addr == nullptr);
        consume(selected);
        REQUIRE(live == 1 && released == 0);
    }
    REQUIRE(live == 0 && released == 1);
    std::cout << "PASS grouping/sparse records/post-return consumption\n";
    {
        AdapterAddresses original;
        REQUIRE(LoadAdapterInfo(0, original) == 2);
        AdapterAddresses copied(original);
        AdapterAddresses assigned;
        assigned = original;
        AdapterAddresses moved(std::move(copied));
        original.clear();
        original.interfaces.reset();
        consume(assigned);
        consume(moved);
        REQUIRE(live == 1);
        assigned.clear();
        assigned.interfaces.reset();
        consume(moved);
    }
    REQUIRE(live == 0 && released == 2);
    std::cout << "PASS copy/assignment/move/last-owner release\n";
    {
        AdapterAddresses selected;
        REQUIRE(LoadAdapterInfo(0, selected) == 2);
        REQUIRE(LoadAdapterInfo(1, selected) == 2);
        REQUIRE(live == 1 && released == 3);
        REQUIRE(string(selected[0]->ifa_name) == "zeta");
        consume(selected);
    }
    REQUIRE(live == 0 && released == 4);
    std::cout << "PASS output reuse releases old allocation\n";
    {
        AdapterAddresses selected;
        REQUIRE(LoadAdapterInfo(99, selected) == 2);
        REQUIRE(selected.empty() && !selected.interfaces);
    }
    REQUIRE(live == 0 && released == 5);
    std::cout << "PASS invalid index releases unselected allocation\n";
    {
        AdapterAddresses selected;
        REQUIRE(LoadAdapterInfo(0, selected) == 2);
        mode = Failure;
        REQUIRE(LoadAdapterInfo(0, selected) == 0);
        REQUIRE(selected.empty() && !selected.interfaces && live == 0);
    }
    REQUIRE(released == 6);
    std::cout << "PASS failure clears previous output\n";
    {
        mode = Empty;
        AdapterAddresses selected;
        REQUIRE(LoadAdapterInfo(0, selected) == 0);
        REQUIRE(selected.empty() && !selected.interfaces && live == 0);
    }
    std::cout << "PASS empty successful list\n";
    {
        mode = Real;
        for (unsigned attempt = 0; attempt < 20; ++attempt) {
            AdapterAddresses selected;
            REQUIRE(LoadAdapterInfo(0, selected) > 0);
            consume(selected);
            REQUIRE(live == 1);
            AdapterAddresses copied(selected);
            consume(copied);
        }
    }
    REQUIRE(live == 0 && allocations.empty());
    std::cout << "PASS real Linux getifaddrs ownership (20 repetitions)\n";
    std::cout << "7 ownership scenarios passed; NOT native Apple integration\n";
}
"""

NEGATIVE = r"""
#undef getifaddrs
#undef freeifaddrs
// PUBLIC_INTERFACE
/** Deliberately read a freed entry; ASan must report heap-use-after-free. */
int main()
{
    AdapterAddresses selected;
    LoadAdapterInfo(0, selected);
    // Ownership assignment alone is removed from the extracted loader.
    // This models the original premature release, not a native Apple build.
    volatile char value = selected[0]->ifa_name[0];
    (void)value;
}
"""


def _compile_and_run(compiler, code, executable):
    """Compile stdin with ASan and run with unsuppressed, fail-fast diagnostics."""
    command = [
        compiler, "-x", "c++", "-", "-std=c++11", "-O0", "-g",
        "-fsanitize=address", "-fno-omit-frame-pointer", "-fno-pie",
        "-no-pie", "-o", str(executable),
    ]
    print("COMPILE:", " ".join(command), flush=True)
    subprocess.run(command, input=code, text=True, check=True, timeout=60)
    environment = os.environ.copy()
    # Override inherited sanitizer settings to prevent masking the negative control.
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1:abort_on_error=0:exitcode=86"
    environment.pop("LSAN_OPTIONS", None)
    return subprocess.run(
        [str(executable)], env=environment, text=True, capture_output=True, timeout=30
    )


# PUBLIC_INTERFACE
def main():
    """Run Linux-only fixed and negative-control checks; return a process exit code."""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()
    if not os.sys.platform.startswith("linux"):
        parser.error("Run in Linux/WSL; this is not an Apple integration runner")
    source_path = Path(__file__).resolve().parents[3] / "Source/core/NetworkInfo.cpp"
    source_bytes = source_path.read_bytes()
    source = source_bytes.decode("utf-8").replace("\r\n", "\n")
    declaration, loader = _extract(source)
    print("SOURCE:", source_path)
    print("SOURCE_SHA256:", hashlib.sha256(source_bytes).hexdigest())
    print("EXTRACT_SHA256:", hashlib.sha256((declaration + loader).encode()).hexdigest())
    print("PLATFORM:", os.uname())
    # Generated files exist only in a cleaned temporary directory, never the checkout.
    with tempfile.TemporaryDirectory(prefix="oi03-ownership-") as directory:
        fixed = _compile_and_run(
            args.compiler, PREFIX + declaration + loader + POSITIVE,
            Path(directory) / "fixed",
        )
        print(fixed.stdout, end="")
        print(fixed.stderr, end="")
        print("FIXED_EXIT:", fixed.returncode)
        if fixed.returncode != 0 or fixed.stderr:
            raise RuntimeError("Fixed ownership check failed or emitted diagnostics")
        # Make one controlled mutation; require the specific sanitizer failure.
        broken = loader.replace("addresses.interfaces = owner;", "", 1)
        negative = _compile_and_run(
            args.compiler, PREFIX + declaration + broken + NEGATIVE,
            Path(directory) / "negative",
        )
        print(negative.stdout, end="")
        print(negative.stderr, end="")
        print("NEGATIVE_EXIT:", negative.returncode)
        if negative.returncode != 86 or "ERROR: AddressSanitizer: heap-use-after-free" not in negative.stderr:
            raise RuntimeError("Negative control did not demonstrate ASan detection")
    print("ALTERNATIVE_CHECK_PASS: ownership only; native Apple test NOT executed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
