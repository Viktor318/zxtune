# ZXTune – Hungarian edition

**English** | [Magyar](README-fork.hu.md)

This is a fork of the [ZXTune](https://github.com/vitamin-caig/zxtune)
chiptune player for Windows, with a Hungarian user interface. ZXTune plays
the music of the ZX Spectrum and many other old computers (PT3, STC, ASC,
TurboSound, AY, DAC, SID, NSF…). This edition is based on the `develop`
branch of ZXTune (the state of 17 November 2026, after r5112), and adds a
Hungarian user interface, 3-chip TurboSound, a new mixer and some fixes.

The original ZXTune documentation is in [README.md](README.md). The changes of
this fork are listed in [CHANGELOG-fork.md](CHANGELOG-fork.md).

## Download

Download `ZXTune-r5112-hu.1-windows.zip` from the
[Releases](https://github.com/Viktor318/zxtune/releases) page, unpack it into a
folder of your choice and start `zxtune-qt.exe`. Nothing has to be installed;
each program is a single file with everything in it.

- `zxtune-qt.exe`: the graphical player.
- `zxtune123.exe`: command line player and converter (e.g. `zxtune123 --wav
  filename=tune.wav tune.pt3`).

## What this edition adds

### Hungarian user interface
- Every menu, dialog and message in Hungarian, including Qt's own dialogs.
  By default it follows the Windows display language; the language can be
  chosen on the *Preferences > Interface* page.

### TurboSound
- **3-chip TurboSound** (9 channels, ZX Spectrum Next / MXTracker, `03TS`)
  and per-chip mixing of the 2-chip `.ts` tunes.

### Mixer
- **AY mixer** and **DAC mixer** (*Playback > Mixer…*, or the **Mixer**
  button next to the volume): volume and panning per channel, stereo mode,
  channel muting (in sync with the blue toolbar buttons), *Mute all*,
  *Enable all*, *Reset all sliders*. The status line shows which tune is
  playing and which part of the page applies to it.
- The *Preferences* window can stay open during playback; if it was open at
  exit, it opens again at the same place at the next start.

### Sound and playback
- The default of the centre channel is 71% (instead of 60%).
- Fixed: seeking (the position slider) stopped the playback; a muted channel
  clicked; muting E (envelope) made everything louder; the CAB and CBA stereo
  modes were swapped.

## Settings

The program keeps its settings at the same place as the original ZXTune (in
the Windows registry), so it uses the settings of an earlier ZXTune too.

## Source code and building

Source code: the [`hungarian-ui`](https://github.com/Viktor318/zxtune/tree/hungarian-ui)
branch. The Windows programs are cross-compiled on Linux (WSL2) with
llvm-mingw, static Qt 5.15.2 and boost 1.82, like the original Windows
releases of ZXTune.

## License

Under the license of ZXTune (see [LICENSE.md](LICENSE.md)).
