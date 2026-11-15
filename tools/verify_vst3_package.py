#!/usr/bin/env python3
"""Verify Windows Skell-Afar VST3 identity and normalize JUCE moduleinfo JSON, never binary code."""
import argparse
import hashlib
import json
import struct
from pathlib import Path

EXPECTED_CIDS = {
    "ABCDEF019182FAEB536B417541666172",
    "ABCDEF011234ABCD536B417541666172",
}


def remove_trailing_commas(src: str) -> str:
    """Remove JSON trailing commas outside strings, preserving all quoted data."""
    result = []
    quoted = escape = False
    for i, ch in enumerate(src):
        if quoted:
            result.append(ch)
            if escape:
                escape = False
            elif ch == "\\":
                escape = True
            elif ch == '"':
                quoted = False
        elif ch == '"':
            quoted = True
            result.append(ch)
        elif ch == ",":
            j = i + 1
            while j < len(src) and src[j].isspace():
                j += 1
            if j >= len(src) or src[j] not in "}]":
                result.append(ch)
        else:
            result.append(ch)
    if quoted:
        raise ValueError("Unterminated JSON string")
    return "".join(result)


def verify_bundle(bundle: Path, fix_json: bool = False) -> dict:
    if not bundle.is_dir() or bundle.suffix.lower() != ".vst3":
        raise ValueError(f"Not a .vst3 bundle directory: {bundle}")
    info_path = bundle / "Contents/Resources/moduleinfo.json"
    bin_path = bundle / "Contents/x86_64-win/Skell-Afar.vst3"
    source = info_path.read_text(encoding="utf-8-sig")
    try:
        info = json.loads(source)
        normalized = False
    except json.JSONDecodeError:
        if not fix_json:
            raise ValueError("moduleinfo.json is invalid strict JSON; pass --fix-json")
        repaired = remove_trailing_commas(source)
        info = json.loads(repaired)  # Keep failing on syntax errors other than trailing commas.
        if repaired == source:
            raise ValueError("Unrelated JSON syntax error, not repaired")
        info_path.write_text(repaired, encoding="utf-8", newline="")
        normalized = True
    if info.get("Name") != "Skell-Afar" or info.get("Factory Info", {}).get("Vendor") != "Skell Audio":
        raise ValueError("Plugin name or vendor changed")
    cids = {x.get("CID") for x in info.get("Classes", []) if x.get("Name") == "Skell-Afar"}
    if cids != EXPECTED_CIDS:
        raise ValueError(f"VST3 classes changed: {sorted(str(v) for v in cids)}")
    binary = bin_path.read_bytes()
    if len(binary) < 0x100 or binary[:2] != b"MZ":
        raise ValueError("Missing Windows PE executable")
    pe_off = struct.unpack_from("<I", binary, 0x3C)[0]
    if pe_off + 6 > len(binary) or binary[pe_off:pe_off + 4] != b"PE\x00\x00":
        raise ValueError("Invalid PE signature")
    machine = struct.unpack_from("<H", binary, pe_off + 4)[0]
    if machine != 0x8664:
        raise ValueError(f"Expected Windows x64 PE (0x8664), got 0x{machine:04x}")
    return {
        "plugin": info["Name"],
        "version": info.get("Version"),
        "vendor": info["Factory Info"]["Vendor"],
        "class_ids": sorted(cids),
        "architecture": "windows-x86_64-PE",
        "binary_size": len(binary),
        "binary_sha256": hashlib.sha256(binary).hexdigest(),
        "moduleinfo_sha256": hashlib.sha256(info_path.read_bytes()).hexdigest(),
        "moduleinfo_repaired": normalized,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bundle", type=Path)
    parser.add_argument("--fix-json", action="store_true")
    parser.add_argument("--manifest", type=Path)
    args = parser.parse_args()
    summary = verify_bundle(args.bundle, args.fix_json)
    output = json.dumps(summary, indent=2, sort_keys=True) + "\n"
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(output, encoding="utf-8")
    print("PASS: Windows VST3 metadata, identity, binary and architecture")
    print(output)


if __name__ == "__main__":
    main()
