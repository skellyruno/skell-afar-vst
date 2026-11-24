# Afar consolidated development line — 2026-11-23

Canonical draft PR #14, branch fix/afar-ui-dsp-audit-20261123, retargeted to main only after confirming all stacked heads are ancestors. No merge, tag, or release.
- PRs 4,5,6,8,9,10,11,12,13: exact heads already contained in 8937835. Retain branches and PR history; mark superseded by #14.
- PR 7: diverged by one commit, CMake-only Standalone removal; canonical branch already uses FORMATS VST3. Retain historical branch.
- Parent 8937835: Windows workflow 45319057822 and real-core ASan/UBSan 45319057835 succeeded. Preview artifact 14886434484, GitHub digest sha256:c8e12e6b59b2add09cfb73f26cb29c3f66db06b1f6fe21ba4735d6181f236dc8. This is the preceding preview, not the new revision. UI workflow 45319057834 was still running at inspection.

## Second DSP batch
- RoomCharacter's fractional taps now use integer cursor math like the main scene ring, avoiding quantized modulation and flutter at large offsets.
- Arrival mode crossfades aligned and physical direct/early/late excitation over 20 ms instead of abruptly jumping delay position. Saved mode starts immediately; limiter latency remains unchanged. Brief parallel paths may produce comb coloration during the transition; real listening required.
- Host-rate matched ambience filter poles; low/band-limited noise compensates approximately for energy per Hz; 48 kHz retains original bed pole/gain calibration. Drip probability scales with rate while retaining the exact 48 kHz event predicate.
- Whisper coloration uses independent deterministic PRNG, so silent whisper processing no longer advances the environment seed stream. Whisper stochastic output changes intentionally.
- Reset peak and clarity values when reinitializing DSP.
- Add wind/traffic multirate RMS, silent whisper isolation and arrival transition regressions.
- No public parameter, preset, identity, state schema, dependency pin or approved layout changes.

## Verification
New exact-head CI required; no full local checkout due DNS, no local JUCE/Quick4 path. Real-core tests, native Windows and real-editor screenshot inspection, host recall, vocal A/B and measured CPU remain release gates. No claim of subjective improvement or true-peak certification. Preserve previous preview downloads and all historical PR evidence.
