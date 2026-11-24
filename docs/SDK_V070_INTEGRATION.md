# Skell Afar / SkellCore v0.7.0 dependency integration

- SkellCore source is already committed and under draft review in [PR #30](https://github.com/skellyruno/skellcore-vst-template/pull/30).
- Pin: `7694ffee5880aed050c33d713b310f853e13d096` (immutable source commit from PR #30; SDK main stays unchanged).
- Direct `#include <SkellCore/FDN.h>` through the plugin `Source/dsp/BandFDN.h` alias; no vendored DSP implementation.
- Tour: `GuidedTour.h` SHA `f72302d68605bb07a582784e53f04fa836f27363` is identical in the old overlay and pinned SDK, so no separate shadow include is needed.
- Real SDK source uses the same `SkellCore::DSP`, `SkellCore::UI`, `SkellCore::UIRenderer` target API; headless CI reads `SKELLCORE_REV`.
- Stub FDN implementation is used only in offline tests, not the released plugin. Compare real-core test results independently; no stub result is an acoustic certification.
- Release blockers: real Windows JUCE compile/FL Studio, preset/state/automation, CPU+latency, actual audio A/B, and issue #32 true-peak overshoots. PR must remain draft.
