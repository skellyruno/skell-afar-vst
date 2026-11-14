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
assert re.search(r'set\(SKELLCORE_REV \"[a-f0-9]{40}\"', cmake)
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
assert 'VERSION 0.5.4' in cmake and 'version `0.5.4`' in (root/'README.md').read_text()
print('PASS: 20 metadata spaces, 20 covers, 45 preset declaration, dependency pin, parameter declarations, CI config')

assert 'SkellCoreTourStyle' in cmake and 'GuidedTourController' in editor
assert 'addAndMakeVisible(detail)' in editor
