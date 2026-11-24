# Skell-Afar: vision

**Skell-Afar is a voice *perspective* instrument, not a reverb.** You do not add an effect to a vocal; you
decide *where the voice is, how it is being delivered, and how it moves*, and the plugin makes the recording sound like it.

## Three pillars
1. **Place**: 20 distinct environments, each with its own acoustic character (flutter, resonance, tilt, slap), not one reverb with different numbers.
2. **Performance**: Effort (whisper to shout), Throat Texture (one knob: clean, airy, raspy, gritty), Recorded As, microphone.
3. **Movement**: distance as depth, the skeleton walking toward and away from the mic, Walk paths locked to the host tempo.

## What it can accomplish
- Make a dry studio vocal sound like it was shouted down a tunnel, whispered from the far end of a crypt, or called across a frozen lake, with level kept sensible by Level Match.
- Cinematic / horror / game audio: one plugin to place dialogue, ghost whispers, announcements, radio voices.
- Hip-hop / pop production: ad-lib throws, "far away" doubles, intimate in-your-ear verses, automated approach and retreat.
- Fast, readable workflow: pick a space, drag the skeleton, move one Effort slider.

## Design rules
- Every control must change something you can *hear* and *see* (the stage shows it).
- Perspective changes character, not loudness (Level Match keeps the voice usable).
- Honest modelling: creative approximations, labelled as such; measurable tests for every claim.

## Next milestones
1. Verify in FL Studio (load/unload, automation, recall, delay compensation) and profile CPU.
2. Sound-design pass on each space against real references; per-space tuning from listening, not just measurements.
3. Space Designer (runtime `.skellspace` import/export) and user-drawn Walk paths.
4. Story/genre preset packs (horror, cinematic, rap, podcast) and A/B persistence.
5. Real oversampling behind the Quality option; worker-thread scene solver for higher-order reflections.
