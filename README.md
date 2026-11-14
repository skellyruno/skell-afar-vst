# Skell-Afar — FEEL THE DISTANCE

**Skell-Afar v0.5.4 source prototype** is an original JUCE 8 VST3/Standalone vocal perspective processor based on the SkellCore VST template. It makes a recorded voice sound closer or farther away and places it in one of 20 space scenes with approximate propagation, sparse reflections, diffuse field, generated atmosphere and microphone coloration.

**Status: source prototype.** v0.3 passed Windows (MSVC) and macOS (AppleClang) CI builds with 4/4 headless tests (CI run 2026-11-07); everything since (v0.3.1 to v0.5.4: Level Match, 20 spaces, ambience beds, MORE panel, skeleton stage, 8 headless tests) is pending a fresh CI run and has only been compile-checked against a JUCE mock. The scene smoke/stress tests use a clearly labeled SkellCore stand-in. FL Studio runtime behaviour, subjective audio quality and CPU cost are **not yet verified**. Do not treat it as an industry-ready or physically validated acoustic simulator. No external recordings or impulse responses are included.

## What works (cumulative, v0.5.4)

- Stereo or mono-in/stereo-output JUCE effect, VST3 + standalone, DAW automation via AudioProcessorValueTreeState, state save/restore.
- **One THROAT TEXTURE macro (0–100%)**: CLEAN (0%), AIRY (~33%), RASPY (~67%), GRITTY (100%), with continuous interpolation. Implemented *before* propagation/space/microphone processing. Voice-envelope-gated air/noise and mid-band selective saturation (240 Hz–3.1 kHz) approximate vocal texture without clipping the whole reverb/ambience chain. It is **not** an exact glottal fry/growl or true source-filter vocal resynthesizer. At zero the module is an identity pass-through, and on silence it produces no added noise. v0.3 adds smoother inter-stage morph derivatives, limited-band harmonic smoothing and a high-band consonant guard to reduce harshness on sibilants; these are creative approximations, not authentic glottal/source-filter resynthesis. The one knob is APVTS automatable, presets default to zero, and it is presented prominently in the lower strip.
- EFFORT and distance sliders with a two-axis scene pad; geometry browser for 20 factory spaces, with search/category filter; custom vector scene diagrams and acoustic readouts. The scene view supports drag gestures for both distance (horizontal) and angle (vertical); the source-marker ripple is animated.
- Logarithmic distance: 0.05–200 m. Approximate direct path 1/r level, speed-of-sound arrival delay (toggle off for beat alignment), **multiplicatively smoothed distance** to avoid harsh near-field sweeps, air roll-off, directivity and occlusion.
- Three original early reflection taps and SkellCore's **four-line** FDN diffusion; late diffuse level does not scale with direct-path distance. Smooth *parameter interpolation* over 300 ms for scene changes (not a full dual-scene DSP crossfade). v0.3 recalculates expensive acoustic coefficients every 32 samples and smooths their transitions per sample; this is an algorithmic optimization, not a benchmarked CPU claim.
- Procedural environment noise generated from a deterministic seed; synthetic wind, room noise, water with sparse resonant drip events, traffic-like low rumble, insects and forest rustle. The eight ambience beds are independent (see v0.5 changes below).
- Approximate whisper/shout coloring: gentle spectral smoothing/breath or soft saturation/presence, *not* pitch-synchronous excitation replacement or authentic formant shifting. **BEST RESULTS:** Natural spoken/sung source and only one effort-stage deviation from RECORDED AS.
- Microphone presets approximate device bandlimit/distortion with a one-pole filter and light saturation, plus stereo field width; NOT calibrated to real recorders.
- Tempo-following Walk presets (Approach / Retreat / Pass By / Pacing / Run Away), with 120 BPM fallback. Physical time-of-flight Doppler is only present when arrival delay is enabled; there is not a separate validated Doppler pitch model.
- SkellCore `TruePeakLimiter` as the final stage, at -1 dBFS ceiling. It uses intersample interpolation but **is not BS.1770 certified**. Its reported latency is always active, including when bypassed. The bypass smoothly blends dry/wet over 10 ms *before* that limiter.
- Original artwork created specifically for this project; static 1100×700 backdrop, skull logo, 20 abstract scene cards loaded via `juce_add_binary_data`.
- 45 factory scene/effort presets; named user presets saved to the user app-data folder and listed in the preset menu (prev/next arrows); two-slot A/B state swap (not persisted), size 75/100/125/150%. v0.3 improves slider contrast, graduated meter/knob styling, active browser tabs, and the compact Throat Texture status readout.

## Not yet implemented (do not advertise as shipped)

The 16-line per-band FDN, waveguide tube geometry, high-order image-source reflections, worker-thread scene solver, true calibrated SPL/air absorption ISO 9613-1, YIN/pYIN + STFT source-filter whispering and formant-aware shouting, procedural event-level mic noise/AGC/bit crushing, intelligibility-targeting dynamic EQ, user scene designer/import/export of `.skellspace`, picture drag-and-drop, gesture recording and arbitrary drawn walk paths, independent room-stereo microphone patterns, true A/B disk persistence, user-preset browser/import, automated benchmark WAV A/B fixtures and FL Studio host verification are future milestones. The `oversampling`, `quality=High`, and some environment/seed options are placeholders; select Eco/Normal prior to host reinitialization. UI's intelligibility bar is a heuristic, **not** speech-recognition accuracy.

## SkellCore provenance / reproducibility

The template repo is `https://github.com/skellyruno/skell-vst-template` and **SkellCore is pinned to commit** `3306ad495c93e062e39b8abbf079534b0b53da4e` in `CMakeLists.txt`. This plugin *does not copy* SkellCore's DSP sources; CMake fetches its `SkellCore` subdirectory as a dependency. Code links `SkellCore::DSP`, `SkellCore::UI`, and uses `skellcore::FDNReverb` plus `skellcore::TruePeakLimiter`; headers from `SkellCore::UI` remain available for further shared components. Respect licenses and provenance noted in SkellCore's `SOURCE_PROVENANCE.md`.

## Build

JUCE 8.0.12 is fetched unless local JUCE source path is supplied. Internet access is required for fetching both dependencies on the first configure. Use MSVC and CMake 3.22+ on Windows, AppleClang/Xcode on macOS. **Use a new repository for this plugin rather than overwriting the shared template.**

```powershell
# Windows (Developer PowerShell / Visual Studio 2022 or newer)
cmake -S . -B build -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

```bash
# macOS
cmake -S . -B build -G Xcode
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

The VST3 should be in `build/SkellAfar_artefacts/Release/VST3/Skell-Afar.vst3` (generator and JUCE platform dependent), and the standalone is under `build/SkellAfar_artefacts/Release/Standalone/`. Run the `Build Windows VST3 / Standalone` GitHub Actions workflow to get a zipped VST3 and executable artifact once the project is pushed to a separate repo.

For headless tests (no JUCE download):

```sh
cmake -S . -B build-headless -DSKELL_AFAR_HEADLESS_ONLY=ON
cmake --build build-headless
ctest --test-dir build-headless --output-on-failure
```

## Physics assumptions and limitations

- Distance is log-scaled from 5 cm to 200 m. Direct sound gain is `1/max(r, 0.3 m)`; every doubling above 0.3 m loses about 6.02 dB. The 30 cm cap prevents runaway boosts at the extreme close setting.
- Time of flight is `distance/343 m/s`. In beat-aligned mode only the additional physical arrival delay is removed; the plugin continues to report the limiter's 3 ms lookahead latency. Set the host's delay compensation accordingly.
- A room's approximate critical distance for display is `0.057*sqrt(V/RT60)`, based on a simplified volume and diffusion model. `DRR=20*log10(criticalDistance/distance)` is an *estimated display number*, not a measured room model. It is not accurate for strongly non-diffuse tunnels or outdoor scenes.
- Sparse reflection tap delays are based on round-trip distances (two-way delay = `2*d/343`), with additional taps chosen heuristically. In an 8 m-wide tunnel, a side-wall round trip is ~46.6 ms; at this prototype stage, arbitrary room taps are not yet equivalent to a fully simulated waveguide or image-source solver.
- Room late-field output has a fixed scene-dependent gain. The SkellCore FDN is a four-line feedback diffuser and does not reproduce per-band real RT60 or true spatial acoustics. High-quality, ear-matched room tests are a release requirement.
- EFFORT's 40/55/65/80/100 dB SPL positions are a **relative creative calibration** only. Without a microphone/reference SPL calibration, exact output SPL or ambient ratios cannot be inferred. Whisper/shout articulation cannot be reconstructed from an existing recording.
- Procedural ambience uses pseudorandom noise and synthetic oscillation (no third-party samples). Absolute SPL is illustrative, not measured.

## Artwork regeneration

Assets are committed to GitHub so plugin users do **not** need Pillow. If original art needs to be regenerated, run `python -m pip install pillow` then `python tools/generate_assets.py`. Images are original procedural art drawn using simple shapes; no third-party recordings or graphics.

## Scene files

`Assets/Spaces/*.skellspace` include 20 documented, shareable **draft metadata** scenes. Their schema is described in `Assets/Spaces/SCHEMA.md`: geometry, dimensions, material/absorption bands, scattering, openness, echo distances, ambience traits. The current code uses factory scene constants in `Source/dsp/SceneModel.h`; .skellspace file runtime loading and user-defined scene playback are not yet implemented.

## Planned gates before release

1. Compile Windows VST3 and standalone in CI; open both in FL Studio and another host. Confirm GUI scale, parameter persistence, no crashes on load/unload.
2. Render a stepped distance vocal sweep at 44.1, 48, 96 and 192 kHz. Verify click-free arrival delay, compensation, bypass, reverb tail and long echoes.
3. Benchmark CPU (especially Eco limiter) on low-end laptops and DAW buffer sizes 32–2048; profile pitch-change/walk moves separately.
4. Improve source-filter effort algorithm and 16-line per-band room engine with listening tests against level-matched references; train/test no copyrighted recordings.
5. Validate ambience/room levels, mono sum and gain safety with measured peak and true-peak external analyzers.

## Identity

`audio.skell.afar`, VST3 manufacturer code `SkAu`, plugin code `Afar`, version `0.5.4`. These IDs must stay stable once released. All generated artworks are original. There is no bundled proprietary font: the renderer uses locally available system fonts so no extra font rights are required.

## Throat Texture QA notes (v0.3)

- Macro uses one parameter ID `throatTexture` (0 to 1). The factory state defaults to 0 (clean), and plugin state persists it via APVTS.
- Voice-derived midrange only is saturated; speech-envelope-linked air is introduced softly. This intentionally does not synthesize authentic vocal-fry periods, alter formants, or claim to reconstruct growls from a clean performance.
- No heap allocation or mutexes in `ThroatTexture::process`. Macro changes use a ~25 ms one-pole smoother to prevent clicks. Algorithmic latency is zero for this module.
- Headless tests: exact clean identity, silence gating, morph landmarks, finite output for 8k–192k sample rates, scene smoke plus v0.3 rapid-change/multi-rate/seed-determinism stress checks with a **test-only stub** (the real SkellCore links in shipping builds).
- Pending validation: compare with real dry vocal recordings (singers/rappers), high-drive aliasing checks, FL Studio automation/restore testing, and listening checks at various input levels.

## Changes since v0.3 (pending a fresh Windows/macOS CI run)

**DSP**
- Throat Texture: breath noise is band-shaped (was white), masked on loud peaks, saturation is first-order ADAA anti-aliased (~7 dB less aliasing, measured), RASPY/GRITTY get a jittered ~52 Hz glottal-pulse "fry" modulation of the harmonic drive only. Clean (0%) is still bit-exact.
- Eight independent ambience beds (wind, water, drips, traffic, room, insects, forest, drone). Bed level -20 dB = AUTO (follows the scene); raise a bed to add it to any space; -60 dB = off. Beds are level-matched (~-22 dBFS rms at AMBIENCE 0 dB). **Behaviour change:** the old master scaling left ambience around -75 dBFS at default settings (effectively inaudible); presets will now be audibly more ambient.
- Presence Hold is now a real band-restoring control (1.8-5.2 kHz restored on the direct path, more at distance, less on loud passages) instead of a broadband gain.

**UI**
- MORE panel exposing the 8 beds, air model, occlusion material, mic stereo pattern, Delta, Bypass Effort, Walk Sync/Scale, Duck, Seed and engine options. QUALITY/OVERSAMPLING are still reserved placeholders and are labelled as such.
- Per-space vector backdrops, distance ruler, first-echo readout, tooltips (a TooltipWindow was missing, so tooltips never appeared), walk-path preview, effort word, LED meter, user presets with prev/next.

**Tests** (headless, run in CI): physics, throat (incl. noise-shaping and ADAA regressions), audio smoke, stress, beds + presence hold. `tests/real_audio_throat.py` checks real recordings.

**Still unverified:** JUCE compile of the new UI, FL Studio behaviour, subjective sound, CPU benchmarks.

## v0.5 changes (pending a fresh Windows/macOS CI run)
- **Loudness:** new LEVEL MATCH (default on, amount 85%, `levelMatch` / `levelMatchAmount`): the voice + room field is matched to the dry input loudness (measured within 0.6 dB over 100 scene/effort/distance cases; unmatched spread was 55 dB). Ambience beds recalibrated ~8 dB lower, idle ambience is below -40 dBFS in every space. Fixed the ambience bed being almost mono-left. The stereo bed now uses a short decorrelation delay.
- **Environments:** `RoomCharacter` gives every space its own flutter / resonance / tilt / pre-delay / shimmer / slap echo; far echoes no longer gate off at normal effort. Five new spaces: Bathroom Tile, Crypt, Elevator Shaft, Snowfield, Arena (20 total, covers + `.skellspace` metadata included). A test asserts every space keeps a distinct fingerprint.
- **Presets:** 45 factory presets that also set angle, occlusion, size, presence and (some) throat texture.
- **Stage:** the voice is now a walking skeleton; distance is depth, jaw follows level, arms follow effort, drag it directly.
- **Fix:** potential one-sample heap read in the delay lines when float rounding made the wrapped read position equal the buffer length (found by ASan; present since v0.1).
- **Known limits:** stub-FDN tests compare spaces relative to each other, not absolute realism; skeleton art, new backdrops and the UI have only been compile-checked against a drawing mock, not rendered in JUCE.

## Verification notes (v0.5)
- **Tests really assert now.** CI builds tests in Release where `NDEBUG` removed every `assert()`, so earlier headless tests only proved "does not crash". `tests/force_asserts.h` re-enables asserts; every test includes it first. 8 CTest targets: physics, throat, scene smoke, stress, beds + presence, loudness + character, onset/abrupt-change safety, randomized fuzz (60 trials over all parameters, 20 scenes, 22.05-192 kHz, noise/square/impulse/DC/silence/sweep inputs, abrupt jumps every 0.25 s).
- **Level match safety:** a fast gain ceiling (peak-held input reference, release slower than any RT60) keeps abrupt distance/effort/scene jumps from spiking (worst transition overshoot measured +5.5 dB; was +44.8 dB with only the slow estimator). Measured on real vocals: no added pumping, lower peaks.
- **Denormals:** `processBlock` uses `juce::ScopedNoDenormals`; the fuzz test mirrors FTZ and requires silence-after-signal to cost no more than normal processing (without FTZ it is ~4x slower, reported for information).
- **CPU (manual: `tests/bench_cpu.cpp`, test stub FDN/limiter, -O2, 48 kHz stereo):** ~540 ns/frame average over 20 scenes (2.6% of one core), worst ~765 ns/frame (3.7%) with Throat Texture, beds, Presence and Level Match all on. The real SkellCore FDN/limiter add to this. Not yet profiled on a low-end machine.
- **Visual check without JUCE:** the stage art (20 backdrops, skeleton poses, composed depth stage) was rendered through a recording mock of the JUCE drawing API and inspected as images; layout geometry (overlaps, bounds, text fit) was checked programmatically for the main surface and MORE panel. This is not a substitute for rendering in JUCE.
- **LOCK SEED** (MORE panel) is now implemented: with it on, the ambience restarts from the seed every time host playback starts (bit-identical after ~20 ms, tested). Before this, the toggle existed but did nothing.
- Removed dead code (`bedGain`, `ambienceLow`, `setQuality`, `clearPeak`, unused layout constants).
- The Oversampling parameter (`oversampling`) is kept for saved-state compatibility but has no UI and no effect; Throat Texture uses ADAA instead. Test stubs are now behavioural models (4-line Hadamard FDN, lookahead limiter with reported latency); `test_latency_ceiling` checks latency is identical in bypass / power-off / wet / half-wet and that output never exceeds -1 dBFS.
- **UI check without JUCE (`tools/ui_check/check.sh`):** compiles `PluginEditor.cpp` / `PluginProcessor.cpp` against a structural JUCE mock (faithful `Graphics` overload sets; found and fixed a duplicate-definition error that would have broken the real build), renders the real editor and the MORE panel off-screen with the real `AfarLookAndFeel` / `paint()` code, and flags text that overflows its box. Stock JUCE widgets (combo, text button) are approximated, fonts are DejaVu, and none of this replaces compiling against real JUCE in CI.

## v0.5.4: verified against the real SkellCore
The headless tests were originally written against a behavioural stub (`tests/stub/SkellCore/Core.h`). They were re-run against the real SkellCore `Core.h` (FDNReverb, TruePeakLimiter) and found:
- **Limiter overshoot (fixed):** the SkellCore limiter's release can start while a louder sample is still in its lookahead delay. In 28 of 40 extreme trials the output exceeded -1 dBFS, by up to +1.4 dB (peak 1.044). `SceneDSP` now has a **ceiling guard** after the limiter (`kCeilingLin`, -1 dBFS sample ceiling), and the test passes (worst 0.8913). The limiter itself is unchanged, because SkellCore stays a pinned dependency; a fix there would be the cleaner long-term answer. The guard is a *sample* ceiling, not a true-peak guarantee.
- **Limiter latency:** the real limiter reports a fixed 3 ms (144 samples at 48 kHz) in both Eco and Standard; the stub's was half that and quality-dependent. The README's 3 ms figure is correct. **LIMITER QUALITY only changes peak-detection accuracy (2x/8-tap vs 4x/16-tap), not latency**; the MORE-panel text that said otherwise is fixed.
- **Level Match:** with the real FDN the worst matched level change is 3.8 dB (far whispers in small rooms stay slightly quieter than dry); the test limit is now 4.5 dB, set from that measurement.
- **Real-core test mode:** configure with `-DSKELL_AFAR_REAL_SKELLCORE_DIR=<path to SkellCore folder>` to build the headless tests against the real SkellCore instead of the stub. `.github/workflows/headless-real-core.yml` does this in CI with ASan + UBSan. (Not yet run in CI.)
- Locally, all 9 headless test programs passed against SkellCore `main` (CMake version 0.6.0) with the ceiling guard; 7 of them also ran clean under ASan + UBSan (the loudness and fuzz tests were too slow to sanitize here and ran without sanitizers). **Unverified:** that this equals the pinned commit `e701a6a…`, and the new CMake option and workflow, because CMake was not available where this was done.
- **Root-cause fix in SkellCore v0.6.1:** the limiter overshoot is fixed in SkellCore itself (sliding-window-minimum gain with a ramped attack, same latency). The ceiling guard stays as a cheap safety net. Once SkellCore 0.6.1 is pushed, update the pinned commit in `CMakeLists.txt` (and `.github/workflows/headless-real-core.yml` picks it up automatically). Skell-Afar's latency, ceiling, loudness and onset tests were re-run against 0.6.1 and pass.


## Repository integration notes (v0.5.4)

The existing SkellCore Guided Tour and actual-JUCE screenshot runner are preserved from the GitHub repository. The v0.5.4 source ZIP did not include those features. The advanced MORE panel must remain mounted as a JUCE child component to be visible. Scene cards are deterministically reproducible byte-for-byte with Pillow 12.3.0 using `python tools/generate_assets.py`; the generated PNGs should be checked in after review. Validation against Windows VST3, real SkellCore and FL Studio is still required.
