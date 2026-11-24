> **Historical status note (superseded for v0.7 integration):** this document records why an earlier v0.6.0 draft kept the old SkellCore pin and vendored FDN. The separate draft [SkellCore v0.7 integration](SDK_V070_INTEGRATION.md) pins the verified PR #30 commit and removes the vendored production FDN. Keep both records for auditability.

# Skell-Afar v0.6.0 integration notes — 2026-11-15

## Scope
- Source baseline: user-supplied `Skell-Afar-v0_6_0.zip`. Preserve Simple/Pro, 20 environment scenes and 45 presets, new asset generator, Standard/Dense/HD engines.
- DSP patch baseline: `Skell-Afar-v0.5.5-DSP-fixed-source.zip`, already proposed in draft PR #5. Restore quiet ambience, unselected preset label, beat-aligned direct anchor, physical arrival routing, and WalkMotion with smooth looping/enable behavior.
- Compatibility: keep all 36 existing APVTS parameter IDs, `audio.skell.afar`, `SkAu`, `Afar` and company `Skell Audio`. Keep previous repo Guided Tour (header-only SDK tour) via gated include and optional screenshot target.
- Fix additional MORE detail panel component-tree attachment.

## Local checks executed
- v0.6.0 baseline CMake `Release` headless: 10/10 passed against test-only SkellCore stub.
- Integrated source CMake `Release` headless: 12/12 passed against test-only SkellCore stub.
- Project-integrity script: passed. Structural mock-JUCE editor/processor compile and 3 preview renders: passed; **not a real JUCE build or real GUI screenshot**.
- New standalone tests for Walk and arrival delay completed under ASan + UBSan; do not extrapolate to the full plug-in.
- Default idle scenes: worst -65.12 dBFS RMS (headless stub), below -48 dBFS regression threshold.
- Walk motion seam peak: 0.000942 metres in the focused test; transport timing/host automation still needs FL Studio verification.
- At 120 m: beat aligned first detected output sample 1034, physical-delay first detected output sample 17827 (48 kHz test-only stub; limiter reports 34 samples here, not the production 144 samples).
- Asset generator matches all 22 uploaded PNGs byte-for-byte with Pillow 12.3.0, NumPy 2.3.5.

## Remaining gates
- The requested SkellCore v0.7.0 archive was not attached. The connected shared GitHub repo currently declares v0.6.0 and lacks `SkellCore/FDN.h`; do not change or publish the shared SDK from this branch. Preserve existing GitHub SkellCore revision pin `3306ad...` and vendored `BandFDN.h` until a real v0.7 commit is available.
- Native JUCE 8 Windows VST3/Standalone compile, real SkellCore sanitizers, real editor Quick4 screenshots, FL Studio state recall and automation, measured CPU/latency, and level-matched vocal A/B are **unverified** locally.
- Do not merge draft PR until all gates pass; user asked another contributor to lead visual redesign.
