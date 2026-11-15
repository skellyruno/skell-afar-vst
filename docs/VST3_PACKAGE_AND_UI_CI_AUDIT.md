# Skell Afar VST3 metadata and real-editor coverage — 2026-11-15

- Audited Windows CI artifact from commit `9b506563` (SkellCore v0.7 pin). Confirmed x64 PE binary SHA-256 `ab2ed40b8f4ebd7f9c75de04e7b2105b504cdccf2e8a8b179b4e6611b133fd5a`.
- Found `Contents/Resources/moduleinfo.json` in the original JUCE VST3 bundle contains trailing commas. Strict JSON parsing fails, even though the binary compiles.
- Added a post-build verifier that removes only trailing commas outside strings and refuses unexpected metadata syntax, name/vendor/class-ID changes or a missing/non-x64 PE. The normalized JSON is parseable; the VST3 executable bytes and SHA do not change. A verification manifest is shipped next to the bundle.
- Real JUCE renderer now captures 6 Pro/tour scenes **and 3 Simple scenes** at 825x525, 1100x700, 1650x1050. The Simple state is set in the real `AudioProcessor` prior to constructing its editor; no UI code or DSP state format is rewritten.
- Upgraded tested PR workflow actions checkout/upload from Node20 v4 to Node24-compatible v6; GitHub-hosted runners must pass the new exact-head CI.
- Source test-only/offline behavior, VST3 identity, automation IDs and SkellCore pin remain identical.
- **Unverified:** FL Studio host import, listening A/B, old preset recall, 16× reconstructed true peaks (SkellCore issue #32). Never confuse valid moduleinfo metadata with mastering true-peak certification.
