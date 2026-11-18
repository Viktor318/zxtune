# Changes of the Hungarian edition

**English** | [Magyar](CHANGELOG-fork.hu.md)

Changes of this fork (`Viktor318/zxtune`, `hungarian-ui` branch) on top of
the `develop` branch of the original ZXTune
([vitamin-caig/zxtune](https://github.com/vitamin-caig/zxtune)). The
changes of ZXTune itself are in the original repository.

## Unreleased

### Hungarian interface

- **Complete Hungarian translation**: the Qt interface (`l10n/hu/*.ts`),
  the messages (`l10n/hu/*.po`) and Qt's own dialogs (`qt.ts`, the official
  Hungarian translation of Qt). It starts in the system language by
  default; the language can be chosen on the *Preferences > Interface* page.
- The wording follows the common glossary of the ZX forks.

### TurboSound

- **3-chip TurboSound** (9 channels, `03TS` footer, ZX Spectrum Next /
  MXTracker): so far it showed up as three separate PT3 modules.
- **Per-chip mixing**: every AY chip has its own 3-channel mixer, and the
  outputs of the chips are summed divided by the number of chips.

### Mixer

- **AY mixer** and **DAC mixer** pages in the *Preferences*. The 9 rows of
  the AY mixer (AY 1–3, 3 per chip) apply to every AY/YM tune (one chip:
  AY 1, TurboSound: AY 1–2 or 1–3); the DAC mixer applies to the 3-channel
  (DMM, DST, STR) and 4-channel (PDT, CHI, SQD, ET1) digital tunes.
- **Volume** and **panning** per channel (similar to the mixers of Fuse and
  ZEsarUX); a double click on a slider sets the default value.
- **Status line**: which tune is playing and which part of the page applies
  to it; the chips used by the current tune are highlighted.
- **Stereo mode** on the AY mixer (the same as on the toolbar); the rows are
  named after the mode (e.g. ACB: A1, C1, B1).
- **Channel muting** on the mixer too (a box before the channel name), in
  sync with the blue toolbar buttons; **Mute all** and **Enable all**
  buttons; *Reset all sliders* resets only the volume and panning.
- *Playback > Mixer…* menu item and a **Mixer** button next to the volume
  control: it opens the mixer page of the current tune.
- The *Preferences* window is modeless: the tune can be changed while it
  is open. If it was open at exit, it opens again at the next start, at the
  same place and on the same page.
- The minimum width of the main window follows the channel buttons (the
  volume part keeps at least 300 pixels).

### Sound

- The **default of the centre channel** of the 3-channel mixers is **71%**
  (instead of 60%, about -3 dB, constant power panning). Values saved
  earlier are kept.
- Fixed: the **CAB and CBA stereo modes** were swapped.
- Fixed: a **muted channel** was not silent, only its tone was gone; its
  volume changes were heard as clicks. Now it is silent. Muting **N**
  (noise) also silences the channels playing noise only.
- Fixed: muting **E** (envelope) set all channels to full volume; now the
  channels using the envelope are silent.

### Playback

- Fixed: **seeking** (the position slider) stopped the playback, sometimes
  with a "std::exception" error – in AY-based, `.ay` and DAC tunes.

### Taken over from the original ZXTune

- FastTracker, ASC Sound Master, ProTracker 2 and 3, Sound Tracker Pro,
  ProDigiTracker and SQ Digital Tracker playback fixes, AHX/HVL detection
  (2026-11-10 – 11-17).
