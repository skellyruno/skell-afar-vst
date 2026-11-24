# Skell Afar experimental true-peak integration

**Status:** draft validation only; not a commercial release and not a BS.1770 certification.

- Parent: Afar installer/update/rollback PR #10 exact head `642747244270380dfe09bc3d9f8673ab73779b7d`.
- SkellCore dependency: experimental PR #43 head `e972e49ef93e021a3993110a0a9603d9774e9def`, based on SkellCore v0.7.0 PR #30.
- This branch does not modify the SkellCore PR, Afar PR #10, or `main`.

## What the new regression covers

`SkellAfarTruePeakIntegration` instantiates Afar's real `SceneDSP` and fully bypasses the creative path while retaining the final shared limiter and sample guard. It feeds the same high-frequency/transient vector used to motivate SkellCore issue #32 and checks:

- 44.1, 48 and 96 kHz;
- 7 kHz, 13.517 kHz and 0.435 × sample-rate stress tones;
- Economy and Standard limiter profiles;
- finite, stereo-identical output;
- sample peak at or below the configured -1 dBFS ceiling;
- unchanged 3 ms reported latency;
- independent 16-phase / 64-tap Blackman reconstruction with regression budgets of 0.12 dB (Economy) and 0.04 dB (Standard).

The test is built only when `SKELL_AFAR_REAL_SKELLCORE_DIR` points at a real SkellCore checkout. This prevents the older test stub from hiding a production dependency regression.

## Performance evidence and limitations

SkellCore PR #43's Ubuntu runner benchmark reported 0.892% (Economy) and 1.665% (Standard) of one 48 kHz stereo real-time stream for the limiter. These are non-gating measurements on a hosted runner, not Afar's full-chain CPU or target-PC performance. The detector remains explicitly uncertified. Recorded-vocal A/B, FL Studio scan/load, automation/state recall, PDC in a real host, and target-hardware CPU/latency are still required before adoption.

Do not merge this integration branch until its exact-head Windows VST3, real-core sanitizer and plugin-level inter-sample tests pass, and the separate listening/host gates are completed.
