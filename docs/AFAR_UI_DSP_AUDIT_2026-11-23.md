# Afar UI/DSP maintenance audit — 2026-11-23

Baseline: PR #13, 21253616d695fa3675becd1cb217fd6a1473ddc6. Prior Windows run 44700087770 and real-core headless run 44700087660 succeeded. Not evidence for this revision.

## Implemented
- Fractional delay reads split integer and fractional offsets before ring addressing, avoiding cursor-dependent sub-sample quantization. Non-finite tap arguments safely use zero delay.
- Sanitize audio samples at the SceneDSP boundary, before throat and loudness-match state can be contaminated.
- Voice envelope, clarity, and DRR ballistics scale with host sample rate, retaining the 48 kHz reference time constants.
- Linear slider zero fill and tiny-width highlights, truthful empty clarity bar, explicitly estimated clarity caption.
- Regression cases for long-ring precision, wrap, invalid delay and NaN/Inf recovery at three rates.

## Preserved
Parameter layout, saved state, preset identities, JUCE 8.0.12, experimental SkellCore pin, factory artwork/layout, limiter latency and ceiling, installer and package format. No change to shared core or main.

## Remaining gates
- Exact-head Windows and real-core headless tests, local actual-editor capture and visual review at 75/100/150%, FL Studio scan/PDC/state recall/automation, real vocal A/B, CPU profiling and commercial licensing.
- Local source checkout blocked by DNS; JUCE/Quick4 not present in this session. No new screenshot or executable claimed.
- Remaining DSP audit findings: procedural bed filter poles/event probability are sample-rate-dependent; distance/arrival switches need listening validation; bypass still applies output trim and delta; stereo-width behavior includes direct field. These need targeted compatibility/listening tests before behavior changes.
- Experimental true-peak detector remains non-certified. No stable release or merge authorized.
