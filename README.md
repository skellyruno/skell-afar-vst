# Skell-Afar — FEEL THE DISTANCE

**Skell-Afar v0.3 source prototype** is an original JUCE 8 VST3/Standalone vocal perspective processor based on the SkellCore VST template. It makes a recorded voice sound closer or farther away and places it in one of 15 space scenes with approximate propagation, sparse reflections, diffuse field, generated atmosphere and microphone coloration.

**Status: source prototype.** The previous v0.2 GitHub Windows VST3 build failed in the settings-menu callback (`SafePointer` nested lambda capture); this source revision fixes that MSVC error. All four local headless C++ tests pass; the scene smoke/stress tests use a clearly labeled SkellCore stand-in. A **new v0.3 Windows VST3 and standalone build is required** before declaring build compatibility. Do not treat it as an industry-ready or physically validated acoustic simulator. No external recordings or impulse responses are included.

## What works in v0.2

- Stereo or mono-in/stereo-output JUCE effect, VST3 + standalone, DAW automation via AudioProcessorValueTreeState, state save/restore.
- **One THROAT TEXTURE macro (0–100%)**: CLEAN (0%), AIRY (~33%), RASPY (~67%), GRITTY (100%), with continuous interpolation. Implemented *before* propagation/space/microphone processing. Voice-envelope-gated air/noise and mid-band selective saturation (240 Hz–3.1 kHz) approximate vocal texture without clipping the whole reverb/ambience chain. It is **not** an exact glottal fry/growl or true source-filter vocal resynthesizer. At zero the module is an identity pass-through, and on silence it produces no added noise. v0.3 adds smoother inter-stage morph derivatives, limited-band harmonic smoothing and a high-band consonant guard to reduce harshness on sibilants; these are creative approximations, not authentic glottal/source-filter resynthesis. The one knob is APVTS automatable, presets default to zero, and it is presented prominently in the lower strip.
- EFFORT and distance sliders with a two-axis scene pad; geometry browser for 15 factory spaces, with search/category filter; custom vector scene diagrams and acoustic readouts. The scene view supports drag gestures for both distance (horizontal) and angle (vertical); the source-marker ripple is animated.
- Logarithmic distance: 0.05–200 m. Approximate direct path 1/r level, speed-of-sound arrival delay (toggle off for beat alignment), **multiplicatively smoothed distance** to avoid harsh near-field sweeps, air roll-off, directivity and occlusion.
- Three original early reflection taps and SkellCore's **four-line** FDN diffusion; late diffuse level does not scale with direct-path distance. Smooth *parameter interpolation* over 300 ms for scene changes (not a full dual-scene DSP crossfade). v0.3 recalculates expensive acoustic coefficients every 32 samples and smooths their transitions per sample; this is an algorithmic optimization, not a benchmarked CPU claim.
- Procedural environment noise generated from a deterministic seed; synthetic wind, room noise, water with sparse resonant drip events, traffic-like low rumble, insects and forest rustle. The eight ambience-bed parameters currently apply as an aggregate gain, not separate per-bed processing.
- Approximate whisper/shout coloring: gentle spectral smoothing/breath or soft saturation/presence, *not* pitch-synchronous excitation replacement or authentic formant shifting. **BEST RESULTS:** Natural spoken/sung source and only one effort-stage deviation from RECORDED AS.
- Microphone presets approximate device bandlimit/distortion with a one-pole filter and light saturation, plus stereo field width; NOT calibrated to real recorders.
- Tempo-following Walk presets (Approach / Retreat / Pass By / Pacing / Run Away), with 120 BPM fallback. Physical time-of-flight Doppler is only present when arrival delay is enabled; there is not a separate validated Doppler pitch model.
- SkellCore `TruePeakLimiter` as the final stage, at -1 dBFS ceiling. It uses intersample interpolation but **is not BS.1770 certified**. Its reported latency is always active, including when bypassed. The bypass smoothly blends dry/wet over 10 ms *before* that limiter.
- Original artwork created specifically for this project; static 1100×700 backdrop, skull logo, 16 abstract scene cards loaded via `juce_add_binary_data`.
- 24 factory scene/effort presets; XML save to user app data folder; A/B editor swap, size 75/100/125/150%. v0.3 improves slider contrast, graduated meter/knob styling, active browser tabs, and the compact Throat Texture status readout.

## Not yet implemented (do not advertise as shipped)

The 16-line per-band FDN, waveguide tube geometry, high-order image-source reflections, worker-thread scene solver, true calibrated SPL/air absorption ISO 9613-1, YIN/pYIN + STFT source-filter whispering and formant-aware shouting, procedural event-level mic noise/AGC/bit crushing, intelligibility-targeting dynamic EQ, user scene designer/import/export of `.skellspace`, picture drag-and-drop, gesture recording and arbitrary drawn walk paths, full ambience bed mixer, independent room-stereo microphone patterns, true A/B disk persistence, user-preset browser/import, automated benchmark WAV A/B fixtures and FL Studio host verification are future milestones. The `oversampling`, `quality=High`, and some environment/seed options are placeholders; select Eco/Normal prior to host reinitialization. UI's intelligibility bar is a heuristic, **not** speech-recognition accuracy.

## SkellCore provenance / reproducibility

The template repo is `https://github.com/skellyruno/skell-vst-template` and **SkellCore is pinned to commit** `e701a6a1e05d923f61c17275609c95122d456223` in `CMakeLists.txt`. This plugin *does not copy* SkellCore's DSP sources; CMake fetches its `SkellCore` subdirectory as a dependency. Code links `SkellCore::DSP`, `SkellCore::UI`, and uses `skellcore::FDNReverb` plus `skellcore::TruePeakLimiter`; headers from `SkellCore::UI` remain available for further shared components. Respect licenses and provenance noted in SkellCore's `SOURCE_PROVENANCE.md`.

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

`Assets/Spaces/*.skellspace` include 15 documented, shareable **draft metadata** scenes. Their schema is described in `Assets/Spaces/SCHEMA.md`: geometry, dimensions, material/absorption bands, scattering, openness, echo distances, ambience traits. The current code uses factory scene constants in `Source/dsp/SceneModel.h`; .skellspace file runtime loading and user-defined scene playback are not yet implemented.

## Planned gates before release

1. Compile Windows VST3 and standalone in CI; open both in FL Studio and another host. Confirm GUI scale, parameter persistence, no crashes on load/unload.
2. Render a stepped distance vocal sweep at 44.1, 48, 96 and 192 kHz. Verify click-free arrival delay, compensation, bypass, reverb tail and long echoes.
3. Benchmark CPU (especially Eco limiter) on low-end laptops and DAW buffer sizes 32–2048; profile pitch-change/walk moves separately.
4. Improve source-filter effort algorithm and 16-line per-band room engine with listening tests against level-matched references; train/test no copyrighted recordings.
5. Validate ambience/room levels, mono sum and gain safety with measured peak and true-peak external analyzers.

## Identity

`audio.skell.afar`, VST3 manufacturer code `SkAu`, plugin code `Afar`, version `0.3.0`. These IDs must stay stable once released. All generated artworks are original. There is no bundled proprietary font: the renderer uses locally available system fonts so no extra font rights are required.

## Throat Texture QA notes (v0.3)

- Macro uses one parameter ID `throatTexture` (0 to 1). The factory state defaults to 0 (clean), and plugin state persists it via APVTS.
- Voice-derived midrange only is saturated; speech-envelope-linked air is introduced softly. This intentionally does not synthesize authentic vocal-fry periods, alter formants, or claim to reconstruct growls from a clean performance.
- No heap allocation or mutexes in `ThroatTexture::process`. Macro changes use a ~25 ms one-pole smoother to prevent clicks. Algorithmic latency is zero for this module.
- Headless tests: exact clean identity, silence gating, morph landmarks, finite output for 8k–192k sample rates, scene smoke plus v0.3 rapid-change/multi-rate/seed-determinism stress checks with a **test-only stub** (the real SkellCore links in shipping builds).
- Pending validation: compare with real dry vocal recordings (singers/rappers), high-drive aliasing checks, FL Studio automation/restore testing, and listening checks at various input levels.
