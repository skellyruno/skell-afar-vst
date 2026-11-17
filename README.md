# Skell-Afar — FEEL THE DISTANCE

**Skell-Afar v0.6.0** is a Windows x64 VST3 private preview of an original JUCE 8 vocal-perspective processor based on the SkellCore VST template. This release line ships VST3 only; it does not ship a standalone application. It makes a recorded voice sound closer or farther away and places it in one of 20 space scenes with approximate propagation, sparse reflections, diffuse field, generated atmosphere and microphone coloration.

**Release status: private preview only.** The Windows VST3 release line has a successful exact-head CI run for build, CTest, package validation and pluginval strictness 5 ([PR #9 run](https://github.com/skellyruno/skell-afar-vst/actions/runs/44008363338)). A separate follow-up adds a checksum-verified per-user installer and rollback test; that follow-up remains pending its own exact-head run. CI does not prove FL Studio compatibility, subjective audio quality, real-world CPU/latency or commercial licensing. The acoustic model is creative/approximate, not a physically validated room simulator. No external recordings or impulse responses are included.

## What works (cumulative, v0.6.0)

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

The shared SDK repo is `https://github.com/skellyruno/skellcore-vst-template` and **SkellCore is pinned to a v0.7.0 draft revision** `7694ffee5880aed050c33d713b310f853e13d096` in `CMakeLists.txt`. This plugin *does not copy* SkellCore's DSP sources; CMake fetches its `SkellCore` subdirectory as a dependency. Code links `SkellCore::DSP`, `SkellCore::UI`, and uses `skellcore::FDNReverb` plus `skellcore::TruePeakLimiter`; headers from `SkellCore::UI` remain available for further shared components. Respect licenses and provenance noted in SkellCore's `SOURCE_PROVENANCE.md`.

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

The VST3 should be in `build/SkellAfar_artefacts/Release/VST3/Skell-Afar.vst3` (generator and JUCE platform dependent). The Windows release workflow stages one VST3 bundle, validates metadata and AMD64 PE32+, runs pluginval, and uploads a private-preview artifact. The package includes `Install-Skell-Afar.ps1` for checksum-verified per-user install/update and rollback; read `INSTALL-WINDOWS.txt` and run the installer with `-WhatIf` first.

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

1. Compile and validate the Windows VST3 in CI; then scan/load it in FL Studio and another host. Confirm GUI scale, parameter persistence, no crashes on load/unload, and test the package installer/rollback on a disposable user profile.
2. Render a stepped distance vocal sweep at 44.1, 48, 96 and 192 kHz. Verify click-free arrival delay, compensation, bypass, reverb tail and long echoes.
3. Benchmark CPU (especially Eco limiter) on low-end laptops and DAW buffer sizes 32–2048; profile pitch-change/walk moves separately.
4. Improve source-filter effort algorithm and 16-line per-band room engine with listening tests against level-matched references; train/test no copyrighted recordings.
5. Validate ambience/room levels, mono sum and gain safety with measured peak and true-peak external analyzers.

## Identity

`audio.skell.afar`, VST3 manufacturer code `SkAu`, plugin code `Afar`, version `0.6.0`. These IDs must stay stable once released. All generated artworks are original. There is no bundled proprietary font: the renderer uses locally available system fonts so no extra font rights are required.

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

## v0.5.5: selectable reverb engine (Standard / Dense / HD)
New MORE-panel option **REVERB ENGINE** (parameter `reverbEngine`, default Standard so existing sessions sound as before):
- **Standard:** the SkellCore 4-line FDN used until now.
- **Dense:** 8-line band-split FDN. **HD:** 16-line band-split FDN (directly supplied by SkellCore v0.7.0 `FDN.h`, with a one-line adapter in `Source/dsp/BandFDN.h`).
- Dense/HD take a separate **low-band and 4 kHz decay time per space**: the low band is the space's RT60 (times SIZE); the 4 kHz decay is derived from the same absorption the 4-line engine's damping applies, then scaled by the space's own upper roll-off (`SceneDSP::highBandDecay`), so dark rooms (Cave, Cellar) get darker tails and bright ones (Field, Canyon) brighter.
- Switching is **runtime and crossfaded (~80 ms)**; the new engine starts from silence, so switching mid-tail drops the old tail's later part. No re-prepare needed. All three engines are allocated at prepare (about 1 MB extra at 192 kHz).
- **Measured (real SkellCore, tests/test_reverb_engine.cpp):** loudness vs Standard within 1.1 dB (Dense) / 1.5 dB (HD) in all 20 spaces with Level Match off; no click when switching; deterministic; latency unchanged. Echo density at 150 ms in SkellCore's own test: 4-line 0.06, 8-line 0.16, 16-line 0.23 (noise = 0.317). **Smoother tail in the plugin is only modestly demonstrated** (envelope fluctuation of the Cathedral tail 2.5 dB std Standard vs 2.3 HD); listen before trusting it.
- **CPU (tests/bench_cpu.cpp, shared container, real SkellCore 0.7.0, 20-scene average):** Standard ~820 ns/frame (~4% of one core at 48 kHz stereo), Dense ~920, HD ~960, so HD costs about +0.6% of a core.
- **Not done:** per-space delay-length ranges (the 24-80 ms range is fixed at prepare; scene changes keep the same lines), and no listening test.

## v0.5.6: visual redesign
Design language: deep neutral charcoal surfaces, **hairline neutral borders** (the old neon-green outlines are gone), **one accent** (phosphor green) reserved for state and signal, depth from light rather than outlines (top highlight, darker lower edge, recessed wells for read-outs).
- **Theme.h:** refined palette; shared helpers `panel` (raised), `inset` (recessed well), `glow` (14-step soft light, no blur needed), `hairline`, `caption`, `sectionTitle`.
- **AfarLookAndFeel:** knobs with a tick scale, glowing value arc, bevelled rim, brushed cap with a lit edge and ivory pointer; groove-style sliders with a dimmed full scale, lit active range and a machined puck thumb; refined toggle; gradient buttons and combo boxes with chevron; value boxes drawn as recessed wells (`drawLabel`).
- **Space cards:** art regenerated (`tools/generate_assets.py`): smooth category-tinted scenes (teal underground, night-green outdoor, warm indoor), one distinct motif per space, no baked-in logo, dithered gradients. Category pills take the category colour; selected card gets an accent bar instead of a heavy frame; the list fades out at the bottom as a scroll hint.
- **Stage and scene pad:** glass-edged art frame, inset read-out chips, softer silhouettes with rim light instead of flat green fills, a proper moon in the Graveyard, crosshair + glow puck on the scene pad.
- **Top bar and meters:** gradient bar with a cast shadow, softer group underlines, segmented meter with glass highlights, inset clarity bar, group dividers in the bottom strip.
- **Verification:** compiled and rendered through `tools/ui_check` (the mock now also exercises the real button, combo, label and value-box drawing). **Not verified:** a real JUCE build (the new `drawLabel`, `drawButtonBackground`, `drawComboBox` and `getTextButtonFont` overrides use the standard JUCE 8 signatures but have only been compiled against the mock), the real UI font (previews use DejaVu), HiDPI/scaled rendering, and how it feels in a host.

## v0.6.0: SIMPLE mode, new environments and skeleton
- **SIMPLE | PRO switch** (top bar, remembered per session in the editor state): SIMPLE shows a large stage (skeleton and environment, drag the skeleton for distance and angle), a scrollable strip of all 20 environment cards (click, drag or mouse-wheel; the chosen card eases to the centre; edge arrows), and one slim row with EFFORT and DISTANCE. DICE and the preset arrows stay; everything else (browser, scene pad, walk path, throat texture, knobs, mic, toggles, MORE) is hidden, not removed, and keeps working in PRO. Sound is identical in both modes.
- **Environments (`Source/ui/Backdrops.h`):** all 20 stage backdrops rebuilt from shared layers (sky or ceiling gradient, light sources, depth layers far to near, ground plane, horizon fog, vignette) with a perspective-room builder for tunnels, halls, alleys and shafts. Rendered once per scene into the cached backdrop image.
- **Skeleton (`SceneView::drawSkeleton`):** tapered bones with joint heads and a lit edge, vertebrae, paired ribs, sternum, clavicles, pelvis, hands and feet, and a modelled skull with glowing sockets (brightness follows the output level) and a mandible that drops with effort and level. Shout spreads the arms, whisper tucks them. It scales with the stage size. New studio-condenser mic.
- **Tooling:** the preview mock now honours clipping (images, rounded-card paths, save/restore), stores ValueTree properties, and renders SIMPLE mode (`simple=1`). `tools/generate_assets.py` also regenerates the card art.
- **Verified:** compiled and rendered through the mock (all 20 environments, both modes). **Not verified:** a real JUCE build (new: `mouseWheelMove`, `reduceClipRegion`, `drawLabel`/button/combo overrides, radial gradients), real fonts, HiDPI, redraw cost of the larger stage, or how the backdrop renders on a real display. The environment art is stylised; judge it by eye.


## v0.6.0 DSP integration / release gate (2026-11-15)

The new Simple/Pro presentation, 20 scene artwork, and Standard/Dense/HD reverb engines remain in place. The v0.5.5 fixes have been re-applied: quiet idle ambience (new default -34 dB, internal bed gain 0.22), no factory preset auto-selection, beat-aligned vocal anchor, physical propagation applied to reflections and diffuse field, and sample-continuous, tempo-following Walk paths with smooth toggling. Saved parameter IDs and the four-character plugin identity remain unchanged.

Restored regression tests: `SkellAfarStartupArrival` and `SkellAfarWalkMotion`. Run `cmake -S . -B build-headless -DSKELL_AFAR_HEADLESS_ONLY=ON -DCMAKE_BUILD_TYPE=Release`, `cmake --build build-headless`, then `ctest --test-dir build-headless --output-on-failure`. The default headless configuration uses a test-only SkellCore stub: it does not verify Windows VST3, the real SkellCore, FL Studio, plugin delay compensation, or listening quality.

**Shared SDK integration note (draft):** this branch pins the exact source import from SkellCore draft PR #30 (`7694ffee5880aed050c33d713b310f853e13d096`), whose `FDN.h` and limiter changes passed GitHub CI. The pin references an immutable commit that is not merged to SDK main. The generic v0.7.0 limiter has known intersample over-ceiling findings (SkellCore issue #32); keep this integration in draft pending actual DSP/host verification.

## SkellCore v0.7.0 dependency integration (draft; release gates pending)
- Pinned immutable commit `7694ffee5880aed050c33d713b310f853e13d096` from [SkellCore draft PR #30](https://github.com/skellyruno/skellcore-vst-template/pull/30). This branch holds the user-supplied v0.7.0 FDN and limiter rewrite; SDK `main` is unchanged.
- `BandFDN.h` is now a thin include alias for `<SkellCore/FDN.h>`. No copied production 8/16-line FDN remains in Afar.
- Dropped the obsolete SkellCoreTourStyle shadow include path. The pinned v0.7.0 commit has the same `GuidedTour.h` blob SHA, so the old tour remains available without overriding newer `Core.h`.
- Offline headless tests retain a *test-only* FDN stand-in in `tests/stub/SkellCore/FDN.h`; only real-core CI tests the actual new DSP.
- Known release blocker: [issue #32](https://github.com/skellyruno/skellcore-vst-template/issues/32) documents generic limiter intersample overshoot. Afar's sample ceiling guard is not a true-peak certificate. Never merge before real host/signal acceptance.
