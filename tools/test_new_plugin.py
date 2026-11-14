#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("new_plugin", ROOT / "tools" / "new_plugin.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class GeneratorTests(unittest.TestCase):
    def test_plugin_checkout_rejected(self):
        # This repository contains Afar-specific DSP. Copying it as a quickstart
        # would produce invalid dependent plugins, so direct users upstream.
        with tempfile.TemporaryDirectory() as root:
            dest = Path(root) / "skell-compressor"
            with self.assertRaisesRegex(FileNotFoundError, "shared SkellCore template checkout"):
                module.make_project("Skell Compressor", dest, "SC01")
            self.assertFalse(dest.exists())
    def test_invalid_identity(self):
        with tempfile.TemporaryDirectory() as root:
            dest = Path(root) / "bad"
            for invalid in ["123", "abcde", "x#01", "😼😼😼😼"]:
                with self.assertRaises(ValueError):
                    module.make_project("Skell Test", dest, invalid)
            self.assertFalse(dest.exists())
            with self.assertRaises(ValueError):
                module.make_project("Skell Test", dest, "ST01", core_rev="main")
            self.assertFalse(dest.exists())

if __name__ == "__main__":
    unittest.main()
