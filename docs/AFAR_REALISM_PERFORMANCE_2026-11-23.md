# Afar realism, UI and CPU pass — 2026-11-23

Baseline exact PR #14 head 29d0e1c: all four CI workflows passed; Windows preview artifact 14887612904 (ZIP digest 5597f3118b58b080cce5f02d07d4ade61b81e85ceb27eed4f18b308e77e3f689); real UI artifact 14888132082 (nine images). ZIP CRC and digests verified; screenshots visually inspected at 75%, 100%, 150% Pro/Tour/Simple. Baseline layout intact; stage distance markings low-contrast, knobs small with relatively strong glow.

## Changes
- Bounded distance-dependent HF shelf on direct voice; independent of existing air cutoff and Presence Hold. Not ISO calibrated; preserve near-field signal and latency.
- Use existing scatter metadata in normalized early reflection weights. Modest distance loss on diffuse field (maximum 22% open / 8% closed); smooth late gain. First echo geometry and 20 scene identities preserved.
- Smooth effort color through neutral/bypass; band-limit aspirated noise with peak masking; blend nonlinear shout coloration from clean. Throat ADAA reuses exact cached antiderivative value instead of computing twice. These are approximations, not resynthesis.
- Control-rate scene geometry, cache static RoomCharacter and high-band decay calculations. Repeatable release benchmark compares 12 cases (48/96 kHz, 3 engines, static/moving) against exact baseline using pinned real core on the same runner, 3 repetitions/median. Report CPU time per frame and single-instance real-time fraction, not DAW CPU.
- Refine existing hardware control bevel, glare, stage distance contrast, haze and backdrop crossfade. Show size-adjusted RT60; pause hidden stage animation. No layout/branding replacement.
- Model regressions cover near/disabled air identity, distance monotonicity, bounded gains and normalized scatter weights.

## Gates
Current source still requires exact-head CTest, Windows/pluginval, actual screenshot inspection/revision and real vocal listening. Local clone fails DNS and no JUCE/Quick4 kit found in local or library search. UI workflow may trigger automatically; do not manually launch routine screenshot CI. Preserve all historical presets/automation IDs/state and preview packages. No claims of measured sound or CPU improvement before results. No main merge or release.
