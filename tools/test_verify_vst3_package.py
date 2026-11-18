"""Portable regression checks for the Windows VST3 post-build verifier."""
import json
import struct
import tempfile
import unittest
from pathlib import Path
from verify_vst3_package import remove_trailing_commas, verify_bundle

CID_COMPONENT = "ABCDEF019182FAEB536B417541666172"
CID_CONTROLLER = "ABCDEF011234ABCD536B417541666172"


def make_bundle(root: Path, *, machine=0x8664, optional_magic=0x20B, version="0.6.0"):
    bundle = root / "Skell-Afar.vst3"
    metadata = bundle / "Contents/Resources/moduleinfo.json"
    module = bundle / "Contents/x86_64-win/Skell-Afar.vst3"
    metadata.parent.mkdir(parents=True)
    module.parent.mkdir(parents=True)
    info = {
        "Name": "Skell-Afar",
        "Version": version,
        "Factory Info": {"Vendor": "Skell Audio"},
        "Classes": [
            {"CID": CID_COMPONENT, "Name": "Skell-Afar"},
            {"CID": CID_CONTROLLER, "Name": "Skell-Afar"},
        ],
    }
    metadata.write_text(json.dumps(info, indent=2), encoding="utf-8")
    pe = bytearray(0x180)
    pe[:2] = b"MZ"
    struct.pack_into("<I", pe, 0x3C, 0x80)
    pe[0x80:0x84] = bytes((0x50, 0x45, 0, 0))
    struct.pack_into("<H", pe, 0x84, machine)
    struct.pack_into("<H", pe, 0x98, optional_magic)
    module.write_bytes(pe)
    return bundle, module


class TestVST3Packaging(unittest.TestCase):
    def test_quoted_commas_untouched(self):
        raw = '{"Name":"Example ,} string","List":[1,2,],"Nested":{"a":1,},}'
        self.assertEqual(
            json.loads(remove_trailing_commas(raw)),
            {"Name": "Example ,} string", "List": [1, 2], "Nested": {"a": 1}},
        )

    def test_repairs_trailing_commas_and_verifies_pe32_plus(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, _ = make_bundle(Path(temp))
            metadata = bundle / "Contents/Resources/moduleinfo.json"
            original = metadata.read_text()
            metadata.write_text(original[:-1] + "," + chr(10) + "}")
            with self.assertRaisesRegex(ValueError, "invalid strict JSON"):
                verify_bundle(bundle)
            result = verify_bundle(bundle, fix_json=True)
            self.assertTrue(result["moduleinfo_repaired"])
            self.assertEqual(result["architecture"], "windows-x86_64-PE32+")
            self.assertEqual(json.loads(metadata.read_text())["Name"], "Skell-Afar")
            self.assertFalse(verify_bundle(bundle)["moduleinfo_repaired"])

    def test_rejects_non_amd64_module(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, _ = make_bundle(Path(temp), machine=0x014C)
            with self.assertRaisesRegex(ValueError, "expected AMD64"):
                verify_bundle(bundle)

    def test_rejects_pe32_optional_header(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, _ = make_bundle(Path(temp), optional_magic=0x10B)
            with self.assertRaisesRegex(ValueError, "expected PE32\\+"):
                verify_bundle(bundle)

    def test_rejects_duplicate_architecture_module(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, module = make_bundle(Path(temp))
            duplicate = bundle / "Contents/x86_64-linux/Skell-Afar.vst3"
            duplicate.parent.mkdir(parents=True)
            duplicate.write_bytes(module.read_bytes())
            with self.assertRaisesRegex(ValueError, "exactly one VST3 module"):
                verify_bundle(bundle)

    def test_rejects_duplicate_json_keys_without_rewriting_even_with_fix_enabled(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, _ = make_bundle(Path(temp))
            metadata = bundle / "Contents/Resources/moduleinfo.json"
            original = metadata.read_text(encoding="utf-8")
            duplicate = original.replace(
                '"Name": "Skell-Afar",',
                '"Name": "Unexpected",\n  "Name": "Skell-Afar",',
                1,
            )
            metadata.write_text(duplicate, encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Duplicate JSON key: Name"):
                verify_bundle(bundle, fix_json=True)
            self.assertEqual(metadata.read_text(encoding="utf-8"), duplicate)

    def test_rejects_non_finite_json_numbers_even_with_fix_enabled(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, _ = make_bundle(Path(temp))
            metadata = bundle / "Contents/Resources/moduleinfo.json"
            original = metadata.read_text(encoding="utf-8")
            non_finite = original.replace(
                '"Version": "0.6.0",',
                '"Version": "0.6.0",\n  "Unexpected": NaN,',
                1,
            )
            metadata.write_text(non_finite, encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "Non-finite JSON number"):
                verify_bundle(bundle, fix_json=True)
            self.assertEqual(metadata.read_text(encoding="utf-8"), non_finite)

    def test_rejects_wrong_version(self):
        with tempfile.TemporaryDirectory() as temp:
            bundle, _ = make_bundle(Path(temp), version="0.5.5")
            with self.assertRaisesRegex(ValueError, "name, version, or vendor"):
                verify_bundle(bundle)


if __name__ == "__main__":
    unittest.main()
