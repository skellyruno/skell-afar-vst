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
EXPECTED_VERSION = "0.6.0"


def remove_trailing_commas(src: str) -> str:
    """Remove JSON trailing commas outside strings, preserving all quoted data."""
    result = []
    quoted = escaped = False
    for i, ch in enumerate(src):
        if quoted:
            result.append(ch)
            if escaped:
                escaped = False
            elif ch == "\\":
                escaped = True
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


def require_windows_x64_pe32_plus(binary: bytes, label: str) -> None:
    """Require a structurally valid AMD64 PE32+ image, not merely an MZ prefix."""
    if len(binary) < 64 or binary[:2] != b"MZ":
        raise ValueError(f"{label}: missing Windows DOS MZ header")
    pe_off = struct.unpack_from("<I", binary, 0x3C)[0]
    if pe_off < 64 or pe_off + 26 > len(binary):
        raise ValueError(f"{label}: invalid or truncated PE header")
    if binary[pe_off:pe_off + 4] != b"PE\\x00\\x00":
        raise ValueError(f"{label}: invalid PE signature")
    machine = struct.unpack_from("<H", binary, pe_off + 4)[0]
    if machine != 0x8664:
        raise ValueError(f"{label}: expected AMD64 PE machine 0x8664, got 0x{machine:04x}")
    optional_magic = struct.unpack_from("<H", binary, pe_off + 24)[0]
    if optional_magic != 0x20B:
        raise ValueError(f"{label}: expected PE32+ optional header 0x20b, got 0x{optional_magic:04x}")


def verify_bundle(bundle: Path, fix_json: bool = False) -> dict:
    if not bundle.is_dir() or bundle.suffix.lower() != ".vst3":
        raise ValueError(f"Not a .vst3 bundle directory: {bundle}")
    contents = bundle / "Contents"
    info_path = contents / "Resources" / "moduleinfo.json"
    bin_path = contents / "x86_64-win" / "Skell-Afar.vst3"
    if not info_path.is_file():
        raise ValueError(f"Missing VST3 metadata: {info_path}")
    if not bin_path.is_file():
        raise ValueError(f"Missing Windows x86_64 VST3 module: {bin_path}")
    modules = sorted(p for p in contents.rglob("*.vst3") if p.is_file())
    if modules != [bin_path]:
        raise ValueError(
            "Expected exactly one VST3 module under Contents at Contents/x86_64-win/Skell-Afar.vst3; "
            f"found {[p.relative_to(contents).as_posix() for p in modules]}"
        )

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

    if not isinstance(info, dict):
        raise ValueError("moduleinfo.json root must be an object")
    factory = info.get("Factory Info")
    if (
        info.get("Name") != "Skell-Afar"
        or info.get("Version") != EXPECTED_VERSION
        or not isinstance(factory, dict)
        or factory.get("Vendor") != "Skell Audio"
    ):
        raise ValueError("Plugin name, version, or vendor changed")
    classes = info.get("Classes")
    if not isinstance(classes, list) or len(classes) != 2:
        raise ValueError("Expected exactly two VST3 classes")
    cids = {x.get("CID") for x in classes if isinstance(x, dict) and x.get("Name") == "Skell-Afar"}
    if cids != EXPECTED_CIDS:
        raise ValueError(f"VST3 classes changed: {sorted(str(v) for v in cids)}")

    binary = bin_path.read_bytes()
    require_windows_x64_pe32_plus(binary, str(bin_path))
    return {
        "plugin": info["Name"],
        "version": info["Version"],
        "vendor": factory["Vendor"],
        "class_ids": sorted(cids),
        "architecture": "windows-x86_64-PE32+",
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
    output = json.dumps(summary, indent=2, sort_keys=True) + "\\n"
    if args.manifest:
        args.manifest.parent.mkdir(parents=True, exist_ok=True)
        args.manifest.write_text(output, encoding="utf-8")
    print("PASS: Windows VST3 metadata, identity, module inventory and AMD64 PE32+")
    print(output)


if __name__ == "__main__":
    main()
