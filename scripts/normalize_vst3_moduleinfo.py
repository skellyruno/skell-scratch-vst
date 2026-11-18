#!/usr/bin/env python3
"""Normalize and validate the staged JUCE VST3 moduleinfo.json for ScratchPad.

Only the release-stage metadata copy is rewritten. The compiled VST3 module is
never modified. JUCE currently emits trailing commas that strict JSON readers
reject; this scanner removes only commas immediately before a closing object
or array token, while respecting quoted strings and escapes.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import sys
from pathlib import Path
from typing import Any

PRODUCT = "ScratchPad"
VENDOR = "Skell Audio"
VERSION_RE = re.compile(r"^\d+\.\d+\.\d+(?:[-+][0-9A-Za-z.-]+)?$")
EXPECTED_CLASSES = {
    "ABCDEF019182FAEB536B417553635031": "Audio Module Class",
    "ABCDEF011234ABCD536B417553635031": "Component Controller Class",
}


def strip_trailing_commas(source: str) -> str:
    """Remove JSON trailing commas outside strings without touching string data."""
    output: list[str] = []
    in_string = False
    escaped = False
    index = 0
    while index < len(source):
        char = source[index]
        if in_string:
            output.append(char)
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                in_string = False
            index += 1
            continue

        if char == '"':
            in_string = True
            output.append(char)
            index += 1
            continue

        if char == ",":
            lookahead = index + 1
            while lookahead < len(source) and source[lookahead].isspace():
                lookahead += 1
            if lookahead < len(source) and source[lookahead] in "}]":
                index += 1
                continue

        output.append(char)
        index += 1

    if in_string:
        raise ValueError("Unterminated JSON string")
    return "".join(output)


def _unique_object(pairs: list[tuple[str, Any]]) -> dict[str, Any]:
    result: dict[str, Any] = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"Duplicate JSON key: {key}")
        result[key] = value
    return result


def _reject_constant(value: str) -> None:
    raise ValueError(f"Non-finite JSON number is not allowed: {value}")


def strict_loads(source: str) -> Any:
    return json.loads(
        source,
        object_pairs_hook=_unique_object,
        parse_constant=_reject_constant,
    )


def validate_metadata(document: Any) -> dict[str, Any]:
    if not isinstance(document, dict):
        raise ValueError("VST3 moduleinfo root must be a JSON object")
    if document.get("Name") != PRODUCT:
        raise ValueError(f"Expected VST3 product {PRODUCT!r}, got {document.get('Name')!r}")

    version = document.get("Version")
    if not isinstance(version, str) or not VERSION_RE.fullmatch(version):
        raise ValueError(f"Invalid VST3 semantic version: {version!r}")

    factory = document.get("Factory Info")
    if not isinstance(factory, dict) or factory.get("Vendor") != VENDOR:
        raise ValueError(f"Expected VST3 vendor {VENDOR!r}")

    classes = document.get("Classes")
    if not isinstance(classes, list) or len(classes) != len(EXPECTED_CLASSES):
        raise ValueError(f"Expected exactly {len(EXPECTED_CLASSES)} VST3 classes")

    found: dict[str, str] = {}
    for item in classes:
        if not isinstance(item, dict):
            raise ValueError("Every VST3 class entry must be an object")
        cid = item.get("CID")
        if not isinstance(cid, str) or not re.fullmatch(r"[0-9A-Fa-f]{32}", cid):
            raise ValueError(f"Invalid VST3 class ID: {cid!r}")
        normalized_cid = cid.upper()
        if normalized_cid in found:
            raise ValueError(f"Duplicate VST3 class ID: {normalized_cid}")
        found[normalized_cid] = str(item.get("Category", ""))
        if item.get("Name") != PRODUCT or item.get("Vendor") != VENDOR:
            raise ValueError("VST3 class product/vendor identity mismatch")
        if item.get("Version") != version:
            raise ValueError("VST3 class version does not match module version")

    if found != EXPECTED_CLASSES:
        raise ValueError(f"VST3 class IDs/categories do not match the protected ScratchPad identity: {found}")

    return {
        "product": PRODUCT,
        "vendor": VENDOR,
        "version": version,
        "class_ids": sorted(found),
    }


def normalize_text(source: str) -> tuple[str, dict[str, Any]]:
    cleaned = strip_trailing_commas(source.lstrip("\ufeff"))
    document = strict_loads(cleaned)
    identity = validate_metadata(document)
    normalized = json.dumps(document, ensure_ascii=False, indent=2, allow_nan=False) + "\n"
    strict_loads(normalized)
    return normalized, identity


def normalize_file(path: Path, report_path: Path | None = None) -> dict[str, Any]:
    original = path.read_bytes()
    source = original.decode("utf-8-sig")
    normalized, identity = normalize_text(source)
    normalized_bytes = normalized.encode("utf-8")
    report = {
        **identity,
        "original_sha256": hashlib.sha256(original).hexdigest(),
        "normalized_sha256": hashlib.sha256(normalized_bytes).hexdigest(),
        "changed": original != normalized_bytes,
        "strict_json": True,
    }
    # Validate everything before rewriting the staged copy.
    path.write_bytes(normalized_bytes)
    if report_path is not None:
        report_path.parent.mkdir(parents=True, exist_ok=True)
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("moduleinfo", type=Path, help="staged Contents/Resources/moduleinfo.json")
    parser.add_argument("--report-path", type=Path, help="write verification JSON report")
    args = parser.parse_args()
    try:
        report = normalize_file(args.moduleinfo, args.report_path)
    except (OSError, UnicodeError, ValueError, json.JSONDecodeError) as exc:
        print(f"moduleinfo verification failed: {exc}", file=sys.stderr)
        return 1
    print(json.dumps(report, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
