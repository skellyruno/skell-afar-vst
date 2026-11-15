#!/usr/bin/env python3
"""Portable source + asset consistency test. Does not substitute for JUCE compile."""
from pathlib import Path
import json
import re

root = Path(__file__).resolve().parents[1]
model = (root/'Source/dsp/SceneModel.h').read_text()
editor = (root/'Source/PluginEditor.cpp').read_text()
cmake = (root/'CMakeLists.txt').read_text()
processor = (root/'Source/PluginProcessor.cpp').read_text()
spaces = sorted((root/'Assets/Spaces').glob('*.skellspace'))
assert len(spaces) == 20, len(spaces)
for i, file in enumerate(spaces):
    assert file.name.startswith(f'{i:02d}_')
    j = json.loads(file.read_text())
    assert j['$schema'] == 'skellspace/0.1-draft'
    assert len(j['surfaces']) >= 4
    assert all(len(s['absorptionOctaveBands125_8k']) == 7 for s in j['surfaces'])
    assert all(0 <= a <= 1 for s in j['surfaces'] for a in s['absorptionOctaveBands125_8k'])
for i in range(20):
    assert (root/f'Assets/space_{i:02d}.png').is_file()
    assert f'Assets/space_{i:02d}.png' in cmake
assert cmake.count('GIT_TAG ${SKELLCORE_REV}') == 1
assert '7694ffee5880aed050c33d713b310f853e13d096' in cmake
assert re.search(r'std::array<Preset,45>',editor)
assert re.search(r'std::array<SpaceSpec, 20>',model)
for param in ['effort','distance','power','space','micType','walkOn','mix','recordedAs','airAbsorption','throatTexture']:
    assert f'"{param}"' in processor
assert 'attach(throatTexture,"throatTexture")' in editor
assert 'throat.setAmount(p.throatTexture)' in (root/'Source/dsp/SceneDSP.h').read_text()
assert (root/'.github/workflows/build-windows.yml').is_file()
assert (root/'.github/workflows/headless-real-core.yml').is_file()
assert 'SKELL_AFAR_REAL_SKELLCORE_DIR' in cmake and cmake.count('${AFAR_CORE_INC}')>=5
assert 'kCeilingLin' in (root/'Source/dsp/SceneDSP.h').read_text()
assert 'VERSION 0.6.0' in cmake and 'version `0.6.0`' in (root/'README.md').read_text()
assert (root/'Source/ui/Backdrops.h').is_file() and (root/'Source/ui/SimpleMode.h').is_file() and 'EnvironmentStrip' in (root/'Source/PluginEditor.cpp').read_text() and 'drawBackdrop(' in (root/'Source/ui/SceneView.h').read_text()
assert (root/'Source/dsp/BandFDN.h').is_file() and (root/'tests/test_reverb_engine.cpp').is_file() and 'reverbEngine' in (root/'Source/PluginProcessor.cpp').read_text() and 'Reverb_engine' in cmake
print('PASS: 20 metadata spaces, 20 covers, 45 preset declaration, dependency pin, parameter declarations, CI config')

# v0.5.5 DSP regression safeguards integrated into v0.6.0.
assert (root/'Source/dsp/WalkMotion.h').is_file()
assert (root/'tests/test_walk_motion.cpp').is_file()
assert (root/'tests/test_startup_and_arrival.cpp').is_file()
assert 'SkellAfarStartupArrival' in cmake and 'SkellAfarWalkMotion' in cmake
assert '"NO PRESET"' in processor
assert '"NO PRESET"' in editor and 'setSelectedId(1,juce::dontSendNotification)' not in editor
assert '"ambience","Ambience dB",-60.f,12.f,-34.f' in processor
assert 'dbGain(p.ambienceDb)*.22f' in (root/'Source/dsp/SceneDSP.h').read_text()
assert 'SkellCoreTourStyle' not in cmake
assert 'GuidedTourController' in editor
assert 'addAndMakeVisible(detail)' in editor

# Shared SkellCore v0.7.0 is the sole production FDN implementation.
fdn = (root/'Source/dsp/BandFDN.h').read_text()
assert '#include <SkellCore/FDN.h>' in fdn and 'using skellcore::BandFDN' in fdn
assert 'class BandFDN' not in fdn
assert 'skellcore-vst-template.git' in cmake
