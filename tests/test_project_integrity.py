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
assert len(spaces) == 15, len(spaces)
for i, file in enumerate(spaces):
    assert file.name.startswith(f'{i:02d}_')
    j = json.loads(file.read_text())
    assert j['$schema'] == 'skellspace/0.1-draft'
    assert len(j['surfaces']) >= 4
    assert all(len(s['absorptionOctaveBands125_8k']) == 7 for s in j['surfaces'])
    assert all(0 <= a <= 1 for s in j['surfaces'] for a in s['absorptionOctaveBands125_8k'])
for i in range(16):
    assert (root/f'Assets/space_{i:02d}.png').is_file()
    assert f'Assets/space_{i:02d}.png' in cmake
assert cmake.count('GIT_TAG ${SKELLCORE_REV}') == 1
assert 'e701a6a1e05d923f61c17275609c95122d456223' in cmake
assert re.search(r'std::array<Preset,24>',editor)
assert re.search(r'std::array<SpaceSpec, 15>',model)
for param in ['effort','distance','power','space','micType','walkOn','mix','recordedAs','airAbsorption','throatTexture']:
    assert f'"{param}"' in processor
assert 'attach(throatTexture,"throatTexture")' in editor
assert 'throat.setAmount(p.throatTexture)' in (root/'Source/dsp/SceneDSP.h').read_text()
assert (root/'.github/workflows/build-windows.yml').is_file()
print('PASS: 15 metadata spaces, 16 covers, 24 preset declaration, dependency pin, parameter declarations, CI config')
