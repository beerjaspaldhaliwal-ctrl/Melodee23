# Installing and using Melodee in your DAW

Download the zip for your system (see *Download* in the [README](../README.md)) and unzip it. Inside you'll
find `VST3/`, `Standalone/`, `CLI/` and, depending on the system, `AU/` (macOS) or `LV2/` (Linux).

## 1. Install the plugin

### Windows
Copy **`VST3/Melodee.vst3`** (it's a folder; copy the whole folder) to:

```
C:\Program Files\Common Files\VST3\
```

Then rescan plugins in your DAW. The standalone app is `Standalone/Melodee.exe`.
Windows may show a SmartScreen warning because the build is not code-signed: choose *More info*, then *Run anyway*.

### macOS
Copy the plugins to your user plug-in folders:

| File | Copy to |
|---|---|
| `AU/Melodee.component` | `~/Library/Audio/Plug-Ins/Components/` |
| `VST3/Melodee.vst3` | `~/Library/Audio/Plug-Ins/VST3/` |

These builds are not notarised, so macOS quarantines them. Run this once in Terminal after copying:

```sh
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/Components/Melodee.component
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Melodee.vst3
```

Restart Logic or GarageBand so they rescan Audio Units. The standalone app is `Standalone/Melodee.app`. The
first time, right-click it and choose *Open*.

### Linux
```sh
mkdir -p ~/.vst3 ~/.lv2
cp -r VST3/Melodee.vst3 ~/.vst3/
cp -r LV2/Melodee.lv2 ~/.lv2/
```

## 2. Three ways to get the music into your project

1. **Drag and drop (works everywhere).** In the bottom-right of Melodee, drag **Melody**, **Chords**,
   **Bass** or **All** onto a track or into the arrangement. Click a button instead to save a `.mid` file.
2. **Built-in sound.** Melodee is an instrument: put it on an instrument track and press play in your DAW.
   The melody, chords and bass play in time with the song.
3. **MIDI out to your own instruments.** Turn off *Sound* in Melodee and route its MIDI output to other
   tracks. The melody is on **channel 1**, chords on **channel 2** and bass on **channel 3**. Choose
   *MIDI out: all on ch 1* if your DAW ignores channels.

## 3. DAW notes

**FL Studio.** Add Melodee to the Channel Rack. For MIDI out, open the plugin's wrapper settings (gear icon),
set a MIDI *Output port* number, then set the same number as the *Input port* on the instrument you want to
play. Drag the Melody/Chords buttons onto the Playlist or a channel.

**Ableton Live.** Put Melodee on a MIDI track. To play another instrument from it, create a second MIDI
track, set *MIDI From* to the Melodee track (and *Melodee* in the second menu), and set *Monitor* to *In*.
Drag the buttons into a clip slot or the Arrangement.

**Logic Pro / GarageBand.** Load Melodee as an Audio Unit instrument on a Software Instrument track and use the
built-in sound, or drag the buttons onto a track to create MIDI regions you can play with any Logic instrument.

**Cubase / Nuendo.** Add Melodee as a VST3 instrument. Other MIDI or instrument tracks can choose
*Melodee* as their MIDI input to receive its notes. Drag the buttons into the Project window.

**Studio One.** Add Melodee as an instrument. On another instrument track, choose Melodee as the input to
record or play its MIDI. Drag the buttons onto the arrangement.

**Reaper.** Add Melodee as track FX. To feed another track, add a send from the Melodee track with
*MIDI: All* enabled. Drag the buttons into the arrange view.

**Bitwig Studio.** Add Melodee as an instrument. Use a *Note Receiver* on another track to pick up its notes.
Drag the buttons into the arranger or a clip launcher slot.

## 4. Tips

- **Follow DAW** keeps Melodee locked to your song position. The loop starts at bar 1 of your project, so a
  4-bar progression repeats every 4 bars.
- **Play** previews the loop when the DAW is stopped (at the DAW tempo, or the preview tempo in standalone).
- **Keys** menu: *Lock to scale* snaps your keyboard to the scale; *Play chords* turns the white keys into the
  chords of your progression (C = chord 1, D = chord 2, ...).
- **Seed**: the same seed and settings always give the same melodies. **Dice** picks a new seed and generates.
- Everything (key, scale, chords, settings and all variations) is saved inside your DAW project.
