#!/usr/bin/env python3
"""Materialize reviewed, local Skell-Afar source snapshot into source files.

The bundle is staged through the GitHub contents API to avoid dropping
binary files or overwriting SkellCore in the template checkout.
No network fetch happens here; extracted paths are strictly validated.
"""
from pathlib import Path
import base64
import hashlib
import io
import lzma
import tarfile

root = Path(__file__).resolve().parents[1]
source = root / "bootstrap" / "source.b85"
encoded = source.read_bytes().strip()
expected = "33fabb328c0738bb158803930c0ab3b11a67a41586c11612ff5709d39ba856eb"
if hashlib.sha256(encoded).hexdigest() != expected:
    raise SystemExit("Skell-Afar source bundle SHA256 mismatch; refusing to unpack")
raw = lzma.decompress(base64.b85decode(encoded))
with tarfile.open(fileobj=io.BytesIO(raw), mode="r:") as archive:
    count = 0
    for entry in archive:
        if not entry.isfile():
            continue
        relative = Path(entry.name)
        if relative.is_absolute() or ".." in relative.parts:
            raise SystemExit(f"Unsafe archive member: {entry.name!r}")
        target = root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        with archive.extractfile(entry) as src:
            target.write_bytes(src.read())
        count += 1
print(f"Materialized {count} Skell-Afar source and scene files.")
