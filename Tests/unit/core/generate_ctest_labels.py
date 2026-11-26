#!/usr/bin/env python3
"""Generate per-GTest CTest label assignments from Thunder's CSV inventory.

Usage:
  python3 generate_ctest_labels.py test_inventory.csv test-labels.cmake Thunder_test_core_TESTS

The third argument is the list variable that gtest_discover_tests() sets for
the target (<target>_TESTS). It is used to check that a test is registered,
because if(TEST ...) is not available when CTest evaluates the generated file.

Expected classification columns:
  feature, scope, purpose

The generated file is intended to be loaded by CTest through the directory
TEST_INCLUDE_FILES property after gtest_discover_tests() has registered the
individual GoogleTest cases.
"""

import argparse
import csv
import re
import sys
from pathlib import Path

REQUIRED = {
    "framework",
    "suite_or_fixture",
    "test_case",
    "feature",
    "scope",
    "purpose",
    "build_registration_status",
    "test_id",
    "source_file",
    "source_line",
}


def cmake_quote(value: str) -> str:
    """Escape a value for use inside a quoted CMake argument."""
    return (
        str(value)
        .replace("\\", "\\\\")
        .replace('"', '\\"')
        .replace(";", "\\;")
        .replace("$", "\\$")
    )


def split_values(value: str) -> list[str]:
    """Split semicolon-separated CSV classifications and remove empty values."""
    return [item.strip() for item in (value or "").split(";") if item.strip()]


def capitalize_first(value: str) -> str:
    """Upper-case only the first character, so 'unit' and 'Unit' give the same label."""
    return value[:1].upper() + value[1:]


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Generate CTest LABELS properties from a Thunder test inventory CSV."
    )
    parser.add_argument("csv_file", type=Path, help="Input test inventory CSV")
    parser.add_argument("output_cmake", type=Path, help="Generated CMake label file")
    parser.add_argument(
        "test_list_var",
        help="List variable set by gtest_discover_tests, for example Thunder_test_core_TESTS",
    )
    args = parser.parse_args()

    if not args.csv_file.is_file():
        print(f"ERROR: CSV file does not exist: {args.csv_file}", file=sys.stderr)
        return 1

    with args.csv_file.open(newline="", encoding="utf-8-sig") as csv_file:
        reader = csv.DictReader(csv_file)
        missing = REQUIRED - set(reader.fieldnames or [])
        if missing:
            print(
                "ERROR: CSV is missing required columns: " + ", ".join(sorted(missing)),
                file=sys.stderr,
            )
            return 1
        rows = list(reader)

    output = [
        "# Generated from the test inventory CSV. DO NOT EDIT MANUALLY.",
        "# Loaded by CTest through the directory TEST_INCLUDE_FILES property.",
        "",
    ]

    seen_names: set[str] = set()
    generated = 0
    skipped = 0
    missing_classification = []

    for row in rows:
        framework = (row.get("framework") or "").strip()
        status = (row.get("build_registration_status") or "").strip()

        if framework.lower() != "gtest":
            skipped += 1
            continue

        if re.search(
            r"Source-only|NOT COMPILED|not built by default|not compiled",
            status,
            re.IGNORECASE,
        ):
            skipped += 1
            continue

        suite = (row.get("suite_or_fixture") or "").strip()
        case = (row.get("test_case") or "").strip()
        if not suite or not case:
            skipped += 1
            continue

        # GTest's default CTest name is SuiteName.TestName unless
        # gtest_discover_tests() is configured with a custom naming scheme.
        # gtest_discover_tests() strips the DISABLED_ prefix from the CTest name.
        suite = suite.removeprefix("DISABLED_")
        case = case.removeprefix("DISABLED_")
        test_name = f"{suite}.{case}"
        if test_name in seen_names:
            print(f"ERROR: Duplicate expected CTest test name: {test_name}", file=sys.stderr)
            return 1
        seen_names.add(test_name)

        labels = []
        feature_values = split_values(row.get("feature", ""))
        scope_values = split_values(row.get("scope", ""))
        purpose_values = split_values(row.get("purpose", ""))

        labels.extend(f"feature:{value}" for value in feature_values)
        labels.extend(f"scope:{capitalize_first(value)}" for value in scope_values)
        labels.extend(f"purpose:{capitalize_first(value)}" for value in purpose_values)

        # Do not silently generate an unclassified test mapping.
        if not feature_values or not scope_values or not purpose_values:
            missing_classification.append(
                f"{test_name}: missing "
                + ", ".join(
                    name
                    for name, values in (
                        ("feature", feature_values),
                        ("scope", scope_values),
                        ("purpose", purpose_values),
                    )
                    if not values
                )
            )

        labels = list(dict.fromkeys(labels))
        if not labels:
            skipped += 1
            continue

        safe_name = cmake_quote(test_name)
        label_string = ";".join(cmake_quote(label) for label in labels)
        test_id = (row.get("test_id") or "").strip()
        source_file = (row.get("source_file") or "").strip()
        source_line = (row.get("source_line") or "").strip()

        output.extend(
            [
                f"# {test_id} | {source_file}:{source_line}",
                f'list(FIND {args.test_list_var} "{safe_name}" _label_idx)',
                "if(NOT _label_idx EQUAL -1)",
                f'    set_tests_properties("{safe_name}" PROPERTIES',
                f'        LABELS "{label_string}"',
                "    )",
                "endif()",
                "",
            ]
        )
        generated += 1

    args.output_cmake.parent.mkdir(parents=True, exist_ok=True)
    args.output_cmake.write_text("\n".join(output), encoding="utf-8")

    print(f"Generated {generated} GTest label mappings: {args.output_cmake}")
    print(f"Skipped {skipped} non-GTest, source-only, not-compiled, or incomplete rows.")

    if missing_classification:
        print(
            f"WARNING: {len(missing_classification)} test(s) have incomplete classification:",
            file=sys.stderr,
        )
        for item in missing_classification:
            print(f"  - {item}", file=sys.stderr)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
