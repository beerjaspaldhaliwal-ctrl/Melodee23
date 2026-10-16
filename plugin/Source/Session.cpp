#include "Session.h"

namespace
{
    juce::String notesToString (const std::vector<melodee::NoteEvent>& notes)
    {
        juce::String s;
        s.preallocateBytes (notes.size() * 20);
        for (const auto& n : notes)
            s << n.pitch << ',' << juce::String (n.start, 4) << ',' << juce::String (n.length, 4) << ',' << n.velocity << ';';
        return s;
    }

    std::vector<melodee::NoteEvent> notesFromString (const juce::String& text)
    {
        std::vector<melodee::NoteEvent> notes;
        for (const auto& item : juce::StringArray::fromTokens (text, ";", ""))
        {
            const auto parts = juce::StringArray::fromTokens (item, ",", "");
            if (parts.size() != 4)
                continue;

            melodee::NoteEvent n;
            n.pitch = juce::jlimit (0, 127, parts[0].getIntValue());
            n.start = juce::jmax (0.0, parts[1].getDoubleValue());
            n.length = juce::jmax (0.01, parts[2].getDoubleValue());
            n.velocity = juce::jlimit (1, 127, parts[3].getIntValue());
            notes.push_back (n);
        }
        return notes;
    }

    juce::String fileSafe (juce::String s)
    {
        s = s.replaceCharacters ("/\\:*?\"<>|#", "--------b-");
        return s.trim();
    }
} // namespace

Session::Session()
{
    const int minor = melodee::findScale ("Natural Minor");

    state.setProperty (IDs::key, 9, nullptr);                 // A
    state.setProperty (IDs::scale, minor >= 0 ? minor : 0, nullptr);
    state.setProperty (IDs::progression, "i VI III VII", nullptr);
    state.setProperty (IDs::beatsPerBar, 4, nullptr);
    state.setProperty (IDs::loops, 1, nullptr);
    state.setProperty (IDs::genre, "Pop", nullptr);
    state.setProperty (IDs::libraryIndex, -1, nullptr);
    state.setProperty (IDs::sevenths, false, nullptr);

    state.setProperty (IDs::style, static_cast<int> (melodee::MelodyStyle::Mixed), nullptr);
    for (auto* id : { &IDs::density, &IDs::syncopation, &IDs::leaps, &IDs::chordTones, &IDs::repetition,
                      &IDs::rests, &IDs::swing, &IDs::noteLength })
        state.setProperty (*id, 0.5, nullptr);
    state.setProperty (IDs::humanize, 0.15, nullptr);
    state.setProperty (IDs::contour, 0, nullptr);
    state.setProperty (IDs::form, 0, nullptr);
    state.setProperty (IDs::lowNote, 60, nullptr);
    state.setProperty (IDs::highNote, 84, nullptr);
    state.setProperty (IDs::endOnTonic, true, nullptr);
    state.setProperty (IDs::variationCount, 8, nullptr);
    state.setProperty (IDs::seed, 1, nullptr);
    state.setProperty (IDs::transform, 0, nullptr);
    state.setProperty (IDs::selected, 0, nullptr);

    state.setProperty (IDs::voicing, static_cast<int> (melodee::VoicingStyle::VoiceLed), nullptr);
    state.setProperty (IDs::chordRhythm, static_cast<int> (melodee::ChordRhythm::Sustain), nullptr);
    state.setProperty (IDs::chordCenter, 60, nullptr);
    state.setProperty (IDs::bass, static_cast<int> (melodee::BassStyle::Roots), nullptr);
    state.setProperty (IDs::bassCenter, 38, nullptr);

    state.setProperty (IDs::playMelody, true, nullptr);
    state.setProperty (IDs::playChords, true, nullptr);
    state.setProperty (IDs::playBass, true, nullptr);
    state.setProperty (IDs::soundOn, true, nullptr);
    state.setProperty (IDs::syncToHost, true, nullptr);
    state.setProperty (IDs::keyboardMode, static_cast<int> (KeyboardThru), nullptr);
    state.setProperty (IDs::outputMode, 0, nullptr);
    state.setProperty (IDs::previewBpm, 110.0, nullptr);

    reparse();
    generate (false);
}

//==============================================================================
juce::var Session::get (const juce::Identifier& id) const
{
    const juce::ScopedLock sl (lock);
    return state.getProperty (id);
}

void Session::set (const juce::Identifier& id, const juce::var& value)
{
    {
        const juce::ScopedLock sl (lock);

        if (state.getProperty (id) == value)
            return;

        const int oldKey = static_cast<int> (state.getProperty (IDs::key));
        state.setProperty (id, value, nullptr);

        if (id == IDs::key)
        {
            reparse();
            int shift = melodee::mod12 (static_cast<int> (value) - oldKey);
            if (shift > 6)
                shift -= 12;
            transposeVariations (shift);
            setStatus ("Key changed: melodies were transposed to the new key.");
        }
        else if (id == IDs::scale || id == IDs::progression || id == IDs::beatsPerBar || id == IDs::loops)
        {
            reparse();
            if (! melodies.empty())
                setStatus ("Chords changed: press Generate for new melodies, or use \"Fit To Chords\".");
        }
    }

    sendChangeMessage();
}

//==============================================================================
melodee::Scale Session::scale() const
{
    const juce::ScopedLock sl (lock);
    return melodee::Scale (getInt (IDs::key), getInt (IDs::scale));
}

bool Session::prefersFlats() const
{
    return scale().keyContext().preferFlats;
}

melodee::MelodySettings Session::melodySettings() const
{
    const juce::ScopedLock sl (lock);

    melodee::MelodySettings s;
    s.beatsPerBar = getInt (IDs::beatsPerBar);
    s.lowNote = getInt (IDs::lowNote);
    s.highNote = juce::jmax (getInt (IDs::highNote), s.lowNote + 12);
    s.density = getDouble (IDs::density);
    s.syncopation = getDouble (IDs::syncopation);
    s.leaps = getDouble (IDs::leaps);
    s.chordTones = getDouble (IDs::chordTones);
    s.repetition = getDouble (IDs::repetition);
    s.rests = getDouble (IDs::rests);
    s.swing = getDouble (IDs::swing);
    s.noteLength = getDouble (IDs::noteLength);
    s.humanize = getDouble (IDs::humanize);
    s.contour = static_cast<melodee::Contour> (juce::jlimit (0, static_cast<int> (melodee::contourNames().size()) - 1, getInt (IDs::contour)));
    s.form = static_cast<melodee::PhraseForm> (juce::jlimit (0, static_cast<int> (melodee::phraseFormNames().size()) - 1, getInt (IDs::form)));
    s.endOnTonic = getBool (IDs::endOnTonic);
    return s;
}

melodee::AccompanimentSettings Session::accompanimentSettings() const
{
    const juce::ScopedLock sl (lock);

    melodee::AccompanimentSettings a;
    a.beatsPerBar = getInt (IDs::beatsPerBar);
    a.voicing = static_cast<melodee::VoicingStyle> (juce::jlimit (0, static_cast<int> (melodee::voicingNames().size()) - 1, getInt (IDs::voicing)));
    a.rhythm = static_cast<melodee::ChordRhythm> (juce::jlimit (0, static_cast<int> (melodee::chordRhythmNames().size()) - 1, getInt (IDs::chordRhythm)));
    a.chordCenter = getInt (IDs::chordCenter);
    a.bass = static_cast<melodee::BassStyle> (juce::jlimit (0, static_cast<int> (melodee::bassStyleNames().size()) - 1, getInt (IDs::bass)));
    a.bassCenter = getInt (IDs::bassCenter);
    a.swing = juce::jlimit (0.0, 1.0, (getDouble (IDs::swing) - 0.5) * 2.0);
    a.humanize = getDouble (IDs::humanize) * 0.6;
    return a;
}

melodee::MelodyStyle Session::style() const
{
    return static_cast<melodee::MelodyStyle> (juce::jlimit (0, melodee::numMelodyStyles() - 1, getInt (IDs::style)));
}

//==============================================================================
int Session::selectedIndex() const
{
    const juce::ScopedLock sl (lock);
    if (melodies.empty())
        return -1;
    return juce::jlimit (0, static_cast<int> (melodies.size()) - 1, getInt (IDs::selected));
}

void Session::select (int index)
{
    set (IDs::selected, index);
}

const melodee::Melody* Session::selectedMelody() const
{
    const juce::ScopedLock sl (lock);
    const int i = selectedIndex();
    return i >= 0 ? &melodies[static_cast<size_t> (i)] : nullptr;
}

void Session::generate (bool append)
{
    {
        const juce::ScopedLock sl (lock);

        if (loopedProgression.empty())
        {
            setStatus ("Type a chord progression first (for example: Am F C G, or vi IV I V).");
        }
        else
        {
            const auto count = juce::jlimit (1, 64, getInt (IDs::variationCount));
            const auto seed = static_cast<std::uint32_t> (getInt (IDs::seed)) + (append ? static_cast<std::uint32_t> (melodies.size()) * 7919u : 0u);
            auto fresh = melodee::generateVariations (loopedProgression, scale(), melodySettings(), style(), count, seed);

            if (! append)
                melodies.clear();

            int number = static_cast<int> (melodies.size());
            for (auto& m : fresh)
            {
                // Renumber so names stay unique when appending.
                const auto dot = m.name.find (". ");
                m.name = std::to_string (++number) + (dot != std::string::npos ? m.name.substr (dot) : ". " + m.name);
                melodies.push_back (std::move (m));
            }

            state.setProperty (IDs::selected, append ? static_cast<int> (melodies.size()) - count : 0, nullptr);
            setStatus (juce::String (count) + " variations written. Click one to hear it, then drag it into your DAW.");
        }
    }
    sendChangeMessage();
}

void Session::applyTransform (int transformIndex)
{
    {
        const juce::ScopedLock sl (lock);
        const int sel = selectedIndex();
        if (sel < 0 || transformIndex < 0 || transformIndex >= melodee::numTransforms())
            return;

        const auto seed = melodee::mixSeed (static_cast<std::uint32_t> (getInt (IDs::seed)), static_cast<std::uint32_t> (melodies.size() * 31 + static_cast<size_t> (transformIndex)));
        auto m = melodee::transformMelody (melodies[static_cast<size_t> (sel)], static_cast<melodee::MelodyTransform> (transformIndex),
                                           loopedProgression, scale(), melodySettings(), seed);

        // Keep names short: "12. Pop Hook > Inversion".
        auto base = melodies[static_cast<size_t> (sel)].name;
        const auto dot = base.find (". ");
        if (dot != std::string::npos)
            base = base.substr (dot + 2);
        m.name = std::to_string (melodies.size() + 1) + ". " + base + " > " + melodee::transformNames()[static_cast<size_t> (transformIndex)];

        melodies.push_back (std::move (m));
        state.setProperty (IDs::selected, static_cast<int> (melodies.size()) - 1, nullptr);
        setStatus ("Added a \"" + juce::String (melodee::transformNames()[static_cast<size_t> (transformIndex)]) + "\" variation.");
    }
    sendChangeMessage();
}

void Session::removeSelected()
{
    {
        const juce::ScopedLock sl (lock);
        const int sel = selectedIndex();
        if (sel < 0)
            return;
        melodies.erase (melodies.begin() + sel);
        state.setProperty (IDs::selected, juce::jmax (0, sel - 1), nullptr);
    }
    sendChangeMessage();
}

void Session::clearVariations()
{
    {
        const juce::ScopedLock sl (lock);
        melodies.clear();
        state.setProperty (IDs::selected, 0, nullptr);
        setStatus ("Cleared. Press Generate to write new melodies.");
    }
    sendChangeMessage();
}

void Session::randomiseSeed()
{
    set (IDs::seed, juce::Random::getSystemRandom().nextInt ({ 1, 999999 }));
}

void Session::loadLibraryProgression (int libraryIndex)
{
    const auto& lib = melodee::progressionLibrary();
    if (libraryIndex < 0 || libraryIndex >= static_cast<int> (lib.size()))
        return;

    const auto& def = lib[static_cast<size_t> (libraryIndex)];
    {
        const juce::ScopedLock sl (lock);
        const int s = melodee::findScale (def.scale);
        if (s >= 0)
            state.setProperty (IDs::scale, s, nullptr);
        state.setProperty (IDs::progression, juce::String (def.numerals), nullptr);
        state.setProperty (IDs::libraryIndex, libraryIndex, nullptr);
        state.setProperty (IDs::genre, juce::String (def.genre), nullptr);
        reparse();
        setStatus (juce::String (def.genre) + " / " + def.name + ": " + def.description + "  Press GENERATE for melodies.");
    }
    sendChangeMessage();
}

void Session::appendChord (const juce::String& token)
{
    auto text = get (IDs::progression).toString().trim();
    set (IDs::progression, text.isEmpty() ? token : text + " " + token);
}

//==============================================================================
void Session::reparse()
{
    const auto bpb = juce::jlimit (1, 12, static_cast<int> (state.getProperty (IDs::beatsPerBar)));
    const auto key = melodee::Scale (static_cast<int> (state.getProperty (IDs::key)), static_cast<int> (state.getProperty (IDs::scale)));
    parseResult = melodee::parseProgression (state.getProperty (IDs::progression).toString().toStdString(), key.keyContext(), bpb);
    loopedProgression = melodee::repeatProgression (parseResult.progression, juce::jlimit (1, 8, static_cast<int> (state.getProperty (IDs::loops))));
}

void Session::transposeVariations (int semitones)
{
    if (semitones == 0)
        return;

    const int lo = static_cast<int> (state.getProperty (IDs::lowNote)) - 6;
    const int hi = static_cast<int> (state.getProperty (IDs::highNote)) + 6;

    for (auto& m : melodies)
    {
        // Move the whole melody by the same amount, choosing the octave that keeps it in range.
        int shift = semitones;
        int mn = 127, mx = 0;
        for (const auto& n : m.notes)
        {
            mn = juce::jmin (mn, n.pitch + shift);
            mx = juce::jmax (mx, n.pitch + shift);
        }
        if (mx > hi && mn - 12 >= lo - 6)
            shift -= 12;
        else if (mn < lo && mx + 12 <= hi + 6)
            shift += 12;

        for (auto& n : m.notes)
            n.pitch = juce::jlimit (0, 127, n.pitch + shift);
    }
}

//==============================================================================
std::vector<melodee::NoteEvent> Session::chordNotes() const
{
    const juce::ScopedLock sl (lock);
    return melodee::renderChords (loopedProgression, accompanimentSettings(), static_cast<std::uint32_t> (getInt (IDs::seed)));
}

std::vector<melodee::NoteEvent> Session::bassNotes() const
{
    const juce::ScopedLock sl (lock);
    return melodee::renderBass (loopedProgression, accompanimentSettings(), static_cast<std::uint32_t> (getInt (IDs::seed)));
}

std::vector<std::vector<int>> Session::chordVoicings() const
{
    const juce::ScopedLock sl (lock);
    const auto a = accompanimentSettings();
    return melodee::voiceProgression (parseResult.progression, a.voicing, a.chordCenter);
}

juce::MemoryBlock Session::createMidi (int parts, double bpm) const
{
    const juce::ScopedLock sl (lock);

    std::vector<melodee::MidiTrack> tracks;
    if ((parts & MelodyPart) != 0)
        if (const auto* m = selectedMelody())
            tracks.push_back ({ "Melody - " + m->name, 0, -1, m->notes });
    if ((parts & ChordsPart) != 0)
        tracks.push_back ({ "Chords - " + parseResult.progression.toChordNames(), 1, -1, chordNotes() });
    if ((parts & BassPart) != 0)
    {
        auto bass = bassNotes();
        if (! bass.empty())
            tracks.push_back ({ "Bass", 2, -1, bass });
    }

    const auto data = melodee::writeMidiFile (tracks, bpm, getInt (IDs::beatsPerBar));
    return juce::MemoryBlock (data.data(), data.size());
}

juce::String Session::suggestedFileName (int parts) const
{
    const juce::ScopedLock sl (lock);

    juce::String partName;
    if (parts == AllParts)
        partName = "Full";
    else if (parts == MelodyPart)
        partName = "Melody";
    else if (parts == ChordsPart)
        partName = "Chords";
    else if (parts == BassPart)
        partName = "Bass";
    else
        partName = "Parts";

    juce::String name = "Melodee " + juce::String (scale().name (prefersFlats()));
    if ((parts & MelodyPart) != 0)
        if (const auto* m = selectedMelody())
            name << " " << juce::String (m->name).upToFirstOccurrenceOf (">", false, false).trim();
    name << " " << partName;

    return fileSafe (name).replaceCharacter ('.', ' ').replace ("  ", " ") + ".mid";
}

//==============================================================================
std::unique_ptr<juce::XmlElement> Session::toXml() const
{
    const juce::ScopedLock sl (lock);

    auto copy = state.createCopy();
    juce::ValueTree vars (IDs::Variations);
    for (const auto& m : melodies)
    {
        juce::ValueTree v (IDs::Variation);
        v.setProperty (IDs::name, juce::String (m.name), nullptr);
        v.setProperty (IDs::description, juce::String (m.description), nullptr);
        v.setProperty (IDs::styleName, juce::String (m.style), nullptr);
        v.setProperty (IDs::seed, static_cast<juce::int64> (m.seed), nullptr);
        v.setProperty (IDs::notes, notesToString (m.notes), nullptr);
        vars.appendChild (v, nullptr);
    }
    copy.appendChild (vars, nullptr);
    return copy.createXml();
}

void Session::fromXml (const juce::XmlElement& xml)
{
    auto loaded = juce::ValueTree::fromXml (xml);
    if (! loaded.hasType (IDs::MelodeeState))
        return;

    {
        const juce::ScopedLock sl (lock);

        for (int i = 0; i < loaded.getNumProperties(); ++i)
        {
            const auto id = loaded.getPropertyName (i);
            state.setProperty (id, loaded.getProperty (id), nullptr);
        }

        melodies.clear();
        for (const auto& v : loaded.getChildWithName (IDs::Variations))
        {
            melodee::Melody m;
            m.name = v.getProperty (IDs::name).toString().toStdString();
            m.description = v.getProperty (IDs::description).toString().toStdString();
            m.style = v.getProperty (IDs::styleName).toString().toStdString();
            m.seed = static_cast<std::uint32_t> (static_cast<juce::int64> (v.getProperty (IDs::seed)));
            m.notes = notesFromString (v.getProperty (IDs::notes).toString());
            melodies.push_back (std::move (m));
        }

        reparse();
        setStatus ({});
    }

    sendChangeMessage();
}
