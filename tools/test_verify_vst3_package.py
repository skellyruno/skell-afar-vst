"""Portable regression checks for the Windows VST3 post-build verifier."""
import json
import struct
import tempfile
import unittest
from pathlib import Path
from verify_vst3_package import remove_trailing_commas, verify_bundle


class TestVST3Packaging(unittest.TestCase):
    def test_quoted_commas_untouched(self):
        raw = '{"Name":"Example ,} string","List":[1,2,],"Nested":{"a":1,},}'
        self.assertEqual(json.loads(remove_trailing_commas(raw)),
                         {"Name":"Example ,} string", "List":[1, 2], "Nested":{"a":1}})

    def test_pe_identity_and_json_are_gated(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp) / "Skell-Afar.vst3"
            meta = root / "Contents/Resources/moduleinfo.json"
            dll = root / "Contents/x86_64-win/Skell-Afar.vst3"
            meta.parent.mkdir(parents=True)
            dll.parent.mkdir(parents=True)
            meta.write_text('{"Name":"Skell-Afar","Version":"0.6.0","Factory Info":{"Vendor":"Skell Audio",},"Classes":[{"CID":"ABCDEF019182FAEB536B417541666172","Name":"Skell-Afar"},{"CID":"ABCDEF011234ABCD536B417541666172","Name":"Skell-Afar"},],}')
            pe = bytearray(0x180)
            pe[:2] = b"MZ"
            struct.pack_into("<I", pe, 0x3C, 0x80)
            pe[0x80:0x84] = b"PE\x00\x00"
            struct.pack_into("<H", pe, 0x84, 0x8664)
            dll.write_bytes(pe)
            with self.assertRaises(ValueError):
                verify_bundle(root)
            self.assertTrue(verify_bundle(root, True)["moduleinfo_repaired"])
            self.assertFalse(verify_bundle(root)["moduleinfo_repaired"])
            self.assertEqual(json.loads(meta.read_text())["Name"], "Skell-Afar")
            struct.pack_into("<H", pe, 0x84, 0x014C)
            dll.write_bytes(pe)
            with self.assertRaises(ValueError):
                verify_bundle(root)


if __name__ == "__main__":
    unittest.main()
