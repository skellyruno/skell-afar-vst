# Skell-Afar v0.3 release gate

This branch exists to run an independent pull-request Windows and macOS JUCE build of the full v0.3 source, not just the offline test stub.

## Required checks
- Windows MSVC: compile VST3 + standalone, then CTest (physics, texture, scene, rapid-scene stress)
- macOS AppleClang: compile VST3 + standalone and run CTest
- Packaging: verify a real .vst3 bundle and standalone executable exist
- FL Studio: host insert/removal, APVTS automation and plugin-state recall must be tested before declaring release-ready

The Windows v0.2 build failed compiling the nested settings-menu SafePointer capture. v0.3 moves the SafePointer capture outside the nested lambda, and updates source texture, visual controls and DSP coefficient cadence. CTest's headless scene path uses a lightweight SkellCore stub and does not replace the full JUCE verification.
