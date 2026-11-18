#!/usr/bin/env python3
"""Regression tests for the ScratchPad release-stage VST3 metadata gate."""
from __future__ import annotations

import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from normalize_vst3_moduleinfo import (
    EXPECTED_CLASSES,
    normalize_file,
    normalize_text,
    strict_loads,
    strip_trailing_commas,
    validate_metadata,
)


def fixture() -> dict:
    classes = []
    for cid, category in EXPECTED_CLASSES.items():
        classes.append({
            "CID": cid,
            "Category": category,
            "Name": "ScratchPad",
            "Vendor": "Skell Audio",
            "Version": "1.19.0",
            "Snapshots": [],
        })
    return {
        "Name": "ScratchPad",
        "Version": "1.19.0",
        "Factory Info": {
            "Vendor": "Skell Audio",
            "URL": "",
            "E-Mail": "",
            "Flags": {"Unicode": True, "Classes Discardable": False},
        },
        "Classes": classes,
    }


class ModuleInfoTests(unittest.TestCase):
    def test_trailing_commas_removed_only_outside_strings(self):
        source = '{"text":"literal ,} and ,]","nested":{"items":[1,2,],},}'
        self.assertEqual(
            strip_trailing_commas(source),
            '{"text":"literal ,} and ,]","nested":{"items":[1,2]}}',
        )

    def test_escaped_quotes_and_backslashes_are_preserved(self):
        text_value = 'escaped " ,} and slash ' + "\\"
        source = json.dumps({"text": text_value, "items": [True]})[:-1] + ",}"
        source = source.replace("[true]", "[true,]")
        cleaned = strip_trailing_commas(source)
        parsed = strict_loads(cleaned)
        self.assertEqual(parsed["items"], [True])
        self.assertEqual(parsed["text"], text_value)

    def test_duplicate_keys_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "Duplicate JSON key"):
            strict_loads('{"Name":"ScratchPad","Name":"Other"}')

    def test_non_finite_numbers_are_rejected(self):
        with self.assertRaisesRegex(ValueError, "Non-finite"):
            strict_loads('{"gain":NaN}')

    def test_exact_product_and_class_identity_pass(self):
        identity = validate_metadata(fixture())
        self.assertEqual(identity["product"], "ScratchPad")
        self.assertEqual(identity["version"], "1.19.0")
        self.assertEqual(len(identity["class_ids"]), 2)

    def test_wrong_product_is_rejected(self):
        data = fixture()
        data["Name"] = "Other Plugin"
        with self.assertRaisesRegex(ValueError, "product"):
            validate_metadata(data)

    def test_wrong_version_or_vendor_is_rejected(self):
        data = fixture()
        data["Classes"][0]["Version"] = "1.18.0"
        with self.assertRaisesRegex(ValueError, "version"):
            validate_metadata(data)
        data = fixture()
        data["Factory Info"]["Vendor"] = "Unknown"
        with self.assertRaisesRegex(ValueError, "vendor"):
            validate_metadata(data)

    def test_duplicate_or_unexpected_class_id_is_rejected(self):
        data = fixture()
        data["Classes"][1]["CID"] = data["Classes"][0]["CID"]
        with self.assertRaisesRegex(ValueError, "Duplicate"):
            validate_metadata(data)
        data = fixture()
        data["Classes"][1]["CID"] = "0" * 32
        with self.assertRaisesRegex(ValueError, "protected ScratchPad identity"):
            validate_metadata(data)

    def test_class_version_must_match_module_version(self):
        data = fixture()
        data["Classes"][0]["Version"] = "1.19.1"
        with self.assertRaisesRegex(ValueError, "does not match"):
            validate_metadata(data)

    def test_normalize_text_emits_strict_json_and_identity(self):
        source = json.dumps(fixture(), indent=2).replace('"Snapshots": []', '"Snapshots": [],')
        normalized, identity = normalize_text(source)
        self.assertTrue(normalized.endswith("\n"))
        self.assertEqual(strict_loads(normalized)["Name"], "ScratchPad")
        self.assertEqual(identity["vendor"], "Skell Audio")

    def test_file_normalization_reports_hashes(self):
        source = json.dumps(fixture(), indent=2).replace('"Snapshots": []', '"Snapshots": [],')
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "moduleinfo.json"
            report_path = Path(tmp) / "moduleinfo-verification.json"
            raw = source.encode("utf-8")
            path.write_bytes(raw)
            report = normalize_file(path, report_path)
            normalized = path.read_bytes()
            self.assertEqual(report["original_sha256"], hashlib.sha256(raw).hexdigest())
            self.assertEqual(report["normalized_sha256"], hashlib.sha256(normalized).hexdigest())
            self.assertTrue(report["changed"])
            self.assertTrue(report_path.is_file())
            strict_loads(normalized.decode("utf-8"))

    def test_invalid_file_is_not_modified(self):
        source = json.dumps(fixture()).replace('"Name": "ScratchPad"', '"Name": "Wrong"')
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "moduleinfo.json"
            raw = source.encode("utf-8")
            path.write_bytes(raw)
            with self.assertRaises(ValueError):
                normalize_file(path)
            self.assertEqual(path.read_bytes(), raw)


if __name__ == "__main__":
    unittest.main(verbosity=2)
