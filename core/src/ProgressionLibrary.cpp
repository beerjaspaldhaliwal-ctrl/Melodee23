#include "melodee/ProgressionLibrary.h"

#include "melodee/Theory.h"

#include <sstream>

namespace melodee
{

namespace
{
    struct Entry
    {
        const char* genre;
        const char* name;
        const char* numerals;
        const char* scale;
        const char* description;
    };

    // Numerals in a major scale context use major-scale degrees (bVII, bVI, iv = borrowed).
    // Numerals in a minor scale context use natural-minor degrees (VI, VII, III; V = harmonic).
    const Entry entries[] = {
        // ---------------------------------------------------------------- Pop
        { "Pop", "Four Chords (Axis)", "I V vi IV", "Major", "The most used pop progression: bright and anthemic." },
        { "Pop", "Emotional Pop", "vi IV I V", "Major", "The same four chords starting on vi: bittersweet and modern." },
        { "Pop", "Optimistic Pop", "I IV vi V", "Major", "Warm, hopeful loop for verses and choruses." },
        { "Pop", "Hopeful Lift", "IV I V vi", "Major", "Starts away from home for an open, lifting feel." },
        { "Pop", "Pachelbel Canon", "I V vi iii IV I IV V", "Major", "Baroque sequence still everywhere in pop ballads." },
        { "Pop", "Doo-Wop / 50s", "I vi IV V", "Major", "The classic 1950s 'ice cream' changes." },
        { "Pop", "Descending Bass Ballad", "I V/7 vi I/5 IV I/3 ii V", "Major", "Stepwise falling bass under ballad chords." },
        { "Pop", "Pop Turnaround", "I vi ii V", "Major", "Turnaround that always pulls back home." },
        { "Pop", "Pre-Chorus Climb", "ii iii IV V", "Major", "Stepwise climb that builds energy into a chorus." },
        { "Pop", "Rising Pop", "I iii IV V", "Major", "Gentle rise through the scale." },
        { "Pop", "Minor-Leaning Pop", "vi V IV V", "Major", "Pulsing loop around the relative minor." },
        { "Pop", "Nostalgic iv", "I IV iv I", "Major", "Major IV turning minor: a nostalgic sigh." },
        { "Pop", "Mixolydian Pop", "I bVII IV I", "Major", "Borrowed bVII for a classic-rock flavoured pop sound." },
        { "Pop", "Dreamy Major 7ths", "Imaj7 IVmaj7", "Major", "Two-chord shimmer for laid-back pop." },
        { "Pop", "Bridge Builder", "IV V iii vi ii V", "Major", "Long tension builder for bridges." },
        { "Pop", "Deceptive Chorus", "I IV V vi", "Major", "Ends on vi instead of I: keeps the loop moving." },
        { "Pop", "Sad Pop Ballad", "vi IV I V/7", "Major", "Emotional loop with a passing bass note." },

        // ---------------------------------------------------------------- Rock
        { "Rock", "Three-Chord Rock", "I IV V IV", "Major", "Rock and roll basics." },
        { "Rock", "Classic Rock Mixolydian", "I bVII IV I", "Mixolydian", "The bVII-IV-I rock cadence." },
        { "Rock", "Mixolydian Vamp", "I bVII", "Mixolydian", "Two-chord riff vamp." },
        { "Rock", "Aeolian Rock", "i VI VII i", "Natural Minor", "Minor rock anthem staple." },
        { "Rock", "Epic Aeolian", "i VI III VII", "Natural Minor", "Huge minor progression for choruses." },
        { "Rock", "Heroic Cadence", "bVI bVII I", "Major", "Triumphant 'Mario cadence'." },
        { "Rock", "Fifths Cycle Rock", "bVI bIII bVII IV I", "Major", "Backwards cycle of fifths into the tonic." },
        { "Rock", "Hard Rock Riff", "i VII VI VII", "Natural Minor", "Driving minor riff progression." },
        { "Rock", "Classic Turnaround", "I bIII IV I", "Major", "Bluesy borrowed bIII." },
        { "Rock", "Southern Rock", "I V bVII IV", "Major", "Open, swaggering major sound." },
        { "Rock", "Power Ballad", "I V vi iii IV I ii V", "Major", "Arena ballad changes." },
        { "Rock", "Major III to Minor iv", "I III IV iv", "Major", "Alternative-rock classic with a major III and minor iv." },
        { "Rock", "Stadium Anthem", "vi IV I V", "Major", "Big singalong loop." },

        // ---------------------------------------------------------------- Punk / Pop-Punk
        { "Punk / Pop-Punk", "Pop-Punk Classic", "I V vi IV", "Major", "Fast power-chord anthem." },
        { "Punk / Pop-Punk", "Punk Drive", "I IV V IV", "Major", "Three chords and the truth." },
        { "Punk / Pop-Punk", "Pop-Punk Minor Start", "vi IV I V", "Major", "Angsty start on vi." },
        { "Punk / Pop-Punk", "Skate Punk", "I IV vi V", "Major", "Upbeat skate punk loop." },
        { "Punk / Pop-Punk", "Punk Descend", "I V/7 vi IV", "Major", "Falling bass power chords." },

        // ---------------------------------------------------------------- Emo / Indie / Alternative
        { "Indie / Alternative", "Indie Dream", "Imaj7 iii7 vi7 IVmaj7", "Major", "Soft, dreamy indie changes." },
        { "Indie / Alternative", "Indie Major 7 Sway", "Imaj7 IVmaj7", "Major", "Two-chord bedroom-pop sway." },
        { "Indie / Alternative", "Midwest Emo", "IVmaj9 iii7 vi9 Imaj9", "Major", "Twinkly extended chords." },
        { "Indie / Alternative", "Alt Minor", "i VI III VII", "Natural Minor", "Moody minor loop." },
        { "Indie / Alternative", "Shoegaze Haze", "Imaj7 bVIImaj7", "Major", "Washy major 7ths a whole step apart." },
        { "Indie / Alternative", "Grunge Power", "I5 bIII5 IV5 bVI5", "Major", "Heavy borrowed power chords." },
        { "Indie / Alternative", "Indie Folk Walk", "I IV vi IV", "Major", "Simple strummed indie folk." },
        { "Indie / Alternative", "Emo Minor Loop", "vi IV V iii", "Major", "Melancholy emo progression." },

        // ---------------------------------------------------------------- Metal
        { "Metal", "Phrygian Chug", "I5 bII5", "Phrygian", "Dark half-step riffing." },
        { "Metal", "Aeolian Gallop", "i VI VII i", "Natural Minor", "Classic heavy metal minor." },
        { "Metal", "Harmonic Minor Metal", "i VI iv V", "Harmonic Minor", "Neo-classical shred progression." },
        { "Metal", "Power Metal Epic", "i VI III VII", "Natural Minor", "Soaring power-metal chorus." },
        { "Metal", "Doom Descent", "i VII VI V", "Harmonic Minor", "Slow descending doom line." },
        { "Metal", "Tritone Doom", "I5 bV5", "Locrian", "The 'devil's interval' between power chords." },
        { "Metal", "Melodic Death", "i VI iv VII", "Natural Minor", "Melodic death metal harmony." },
        { "Metal", "Neo-Classical Cadence", "i iv V i", "Harmonic Minor", "Baroque-style cadence for shred." },

        // ---------------------------------------------------------------- Blues
        { "Blues", "12-Bar Blues", "I7 I7 I7 I7 IV7 IV7 I7 I7 V7 IV7 I7 V7", "Blues", "The standard 12-bar blues." },
        { "Blues", "12-Bar Quick Change", "I7 IV7 I7 I7 IV7 IV7 I7 I7 V7 IV7 I7 V7", "Blues", "12-bar with the quick change to IV in bar 2." },
        { "Blues", "8-Bar Blues", "I7 V7 IV7 IV7 I7 V7 I7 V7", "Blues", "Shorter 8-bar blues form." },
        { "Blues", "Minor Blues", "i7 i7 i7 i7 iv7 iv7 i7 i7 VI7 V7 i7 V7", "Natural Minor", "12-bar minor blues." },
        { "Blues", "Blues Turnaround", "I7 IV7 I7 V7", "Blues", "Classic turnaround." },
        { "Blues", "Slow Blues", "| I7 IV7 | I7 I7 | IV7 IV7 | I7 VI7 | ii7 V7 | I7 V7 |", "Blues", "Slow blues with a ii-V turnaround." },
        { "Blues", "Blues Rock Shuffle", "I IV I V IV I", "Blues", "Shuffle progression for blues rock." },

        // ---------------------------------------------------------------- Jazz
        { "Jazz", "ii-V-I Major", "ii7 V7 Imaj7 Imaj7", "Major", "The foundation of jazz harmony." },
        { "Jazz", "ii-V-i Minor", "iiø7 V7 i7 i7", "Harmonic Minor", "Minor ii-V-i with a half-diminished ii." },
        { "Jazz", "I-vi-ii-V Turnaround", "Imaj7 vi7 ii7 V7", "Major", "The standard jazz turnaround." },
        { "Jazz", "iii-vi-ii-V-I", "iii7 vi7 ii7 V7 Imaj7", "Major", "Chain of fifths into the tonic." },
        { "Jazz", "Rhythm Changes (A)", "| I VI7 | ii7 V7 | iii7 VI7 | ii7 V7 | I I7 | IV #iv°7 | I/5 V7 | I |", "Major", "The A section of rhythm changes." },
        { "Jazz", "Fifths Cycle (Autumn-style)", "ii7 V7 Imaj7 IVmaj7 viiø7 III7 vi7 vi7", "Major", "Diatonic cycle of fifths through relative minor." },
        { "Jazz", "Coltrane Changes", "Imaj7 bIII7 bVImaj7 VII7 IIImaj7 V7 Imaj7", "Major", "Major-third cycle substitutions." },
        { "Jazz", "Backdoor ii-V", "iv7 bVII7 Imaj7 Imaj7", "Major", "Resolves to I from bVII7." },
        { "Jazz", "Tritone Substitution", "ii7 bII7 Imaj7 Imaj7", "Major", "bII7 replaces V7 for a chromatic bass." },
        { "Jazz", "Jazz Blues", "| I7 | IV7 | I7 | v7 I7 | IV7 | #iv°7 | I7 | VI7 | ii7 | V7 | I7 VI7 | ii7 V7 |", "Mixolydian", "12-bar blues with jazz substitutions." },
        { "Jazz", "Modal Vamp (So What-style)", "i7:4 bii7:2 i7:2", "Dorian", "Dorian modal jazz vamp moving up a half step." },
        { "Jazz", "Tadd Dameron Turnaround", "Imaj7 bIIImaj7 bVImaj7 bIImaj7", "Major", "Lush major 7th turnaround." },
        { "Jazz", "Minor Line Cliché", "i imaj7 i7 i6", "Melodic Minor", "Chromatic inner line in a minor chord." },
        { "Jazz", "Major Line Cliché", "I I+ I6 I+", "Major", "Chromatic inner line in a major chord." },
        { "Jazz", "Altered Dominant ii-V", "ii7 V7alt Imaj9 Imaj9", "Major", "Tense altered V into a rich tonic." },
        { "Jazz", "Minor ii-V Chain", "iiø7 V7b9 i7 iv7 VII7 IIImaj7 VImaj7 V7", "Harmonic Minor", "Minor key cycle of fifths." },
        { "Jazz", "Bossa Jazz", "Imaj7 II7 ii7 bII7 Imaj7", "Major", "Bossa-style chromatic changes." },

        // ---------------------------------------------------------------- R&B / Neo-Soul
        { "R&B / Neo-Soul", "Just The Two Of Us", "| IVmaj7 | III7 | vi7 | v7 I7 |", "Major", "The famous neo-soul / J-pop loop." },
        { "R&B / Neo-Soul", "Neo-Soul ii-iii", "ii9 iii7", "Major", "Two-chord neo-soul sway." },
        { "R&B / Neo-Soul", "Neo-Soul Descent", "IVmaj9 iii7 ii9 Imaj9", "Major", "Smooth descending extended chords." },
        { "R&B / Neo-Soul", "Slow Jam", "Imaj9 vi9 ii9 V13", "Major", "90s slow-jam changes." },
        { "R&B / Neo-Soul", "Dorian Vamp", "i9 IV9", "Dorian", "Funky minor Dorian vamp." },
        { "R&B / Neo-Soul", "Minor Neo-Soul", "i9 iv9 VImaj9 V7#9", "Natural Minor", "Dark minor neo-soul with a #9 dominant." },
        { "R&B / Neo-Soul", "Borrowed iv R&B", "Imaj7 iii7 IVmaj7 iv7", "Major", "Major to minor iv for a soulful turn." },
        { "R&B / Neo-Soul", "Contemporary R&B", "vi9 IVmaj9 Imaj9 V9sus4", "Major", "Modern R&B ballad loop." },
        { "R&B / Neo-Soul", "ii-V Vamp", "ii9 V13", "Major", "Endless ii-V groove." },
        { "R&B / Neo-Soul", "Soulful Royal Road", "IVmaj7 V7 iii7 vi7", "Major", "Royal road with 7ths." },

        // ---------------------------------------------------------------- Gospel
        { "Gospel", "Gospel 1-4 Walkup", "I I7/3 IV #iv°7 I/5 VI7 ii7 V7", "Major", "Walking bass from I to IV and back." },
        { "Gospel", "Gospel 2-5-1", "ii9 V13 Imaj9 Imaj9", "Major", "Rich extended ii-V-I." },
        { "Gospel", "Gospel 3-6-2-5-1", "iii7 VI7 ii7 V7 Imaj7", "Major", "Secondary-dominant chain home." },
        { "Gospel", "Backdoor Gospel", "IV iv7 bVII9 Imaj7", "Major", "Minor iv and bVII9 back to I." },
        { "Gospel", "Minor Circle of Fifths", "i7 iv7 VII7 IIImaj7 VImaj7 iiø7 V7 i7", "Harmonic Minor", "The full minor cycle of fifths." },
        { "Gospel", "Gospel Tritone Sub", "ii7 bII7 Imaj7 VI7", "Major", "Chromatic gospel approach chords." },
        { "Gospel", "Worship Shout", "I IV/1 I V/1", "Major", "Pedal-point shout chords over the tonic." },

        // ---------------------------------------------------------------- Soul / Motown / Funk / Disco
        { "Soul / Motown", "Motown Turnaround", "I vi ii V", "Major", "Classic Motown changes." },
        { "Soul / Motown", "Soul Ballad", "I I7 IV iv", "Major", "Bluesy I7 into a minor iv." },
        { "Soul / Motown", "Stax Soul", "I IV I V", "Major", "Simple and punchy." },
        { "Soul / Motown", "Northern Soul", "I iii IV V", "Major", "Uptempo soul lift." },
        { "Funk", "Funk Dorian Vamp", "i7 IV7", "Dorian", "Minor 7 to dominant IV: the funk vamp." },
        { "Funk", "One-Chord Funk", "I9:4", "Mixolydian", "Stay on one dominant 9 and groove." },
        { "Funk", "Funk ii-V", "ii9 V9", "Major", "Slick two-chord funk." },
        { "Funk", "Minor Funk", "i7 iv7 i7 VII7", "Dorian", "Minor funk with a bVII7 kick." },
        { "Disco", "Disco Minor 7", "i7 iv7", "Dorian", "Four-on-the-floor minor groove." },
        { "Disco", "Disco Cycle", "vi7 ii7 V7 Imaj7", "Major", "Cycle of fifths disco strings." },
        { "Disco", "Disco Lift", "ii7 iii7 IVmaj7 V", "Major", "Rising disco changes." },
        { "Disco", "Nu-Disco", "i9 VImaj7 VII III", "Natural Minor", "Modern disco minor loop." },

        // ---------------------------------------------------------------- Hip-Hop
        { "Hip-Hop", "Boom Bap Minor", "i7 iv7", "Dorian", "Dusty minor sample-style loop." },
        { "Hip-Hop", "Jazzy Boom Bap", "IVmaj7 iii7 ii7 Imaj7", "Major", "Jazz-sample descending loop." },
        { "Hip-Hop", "Dark Two-Chord", "i VI", "Natural Minor", "Moody two-chord beat." },
        { "Hip-Hop", "G-Funk Dorian", "i7 IV9", "Dorian", "West Coast G-funk vamp." },
        { "Hip-Hop", "Soul Sample Flip", "IVmaj7 iii7 vi7 V7", "Major", "Chopped soul-sample feel." },
        { "Hip-Hop", "Old School Minor", "i iv v i", "Natural Minor", "Classic minor hip-hop loop." },
        { "Hip-Hop", "Melancholy Piano", "i VI III VII", "Natural Minor", "Emotional rap piano loop." },
        { "Hip-Hop", "Cinematic Rap", "i iv VI V", "Harmonic Minor", "Dramatic orchestral hip-hop." },

        // ---------------------------------------------------------------- Trap
        { "Trap", "Trap Minor Loop", "i VI III VII", "Natural Minor", "Staple minor trap progression." },
        { "Trap", "Phrygian Trap", "i bII", "Phrygian", "Menacing half-step trap loop." },
        { "Trap", "Harmonic Minor Trap", "i VI V i", "Harmonic Minor", "Dark trap with the raised 7th." },
        { "Trap", "Sad Trap", "VI VII i i", "Natural Minor", "Emotional trap rising into i." },
        { "Trap", "Dark Drift", "i iv", "Natural Minor", "Minimal two-chord darkness." },
        { "Trap", "Evil Trap", "i ii° VI V", "Harmonic Minor", "Diminished tension for villain beats." },
        { "Trap", "Melodic Trap", "iv VI VII i", "Natural Minor", "Melodic rap / emo trap loop." },
        { "Trap", "Rage", "i VII VI VII", "Natural Minor", "Synth-heavy rage loop." },

        // ---------------------------------------------------------------- Drill
        { "Drill", "UK Drill Classic", "i VI VII V", "Harmonic Minor", "Sliding-808 drill harmony." },
        { "Drill", "Drill Phrygian", "i bII i VII", "Phrygian", "Dark half-step drill movement." },
        { "Drill", "Drill Cadence", "i iv V i", "Harmonic Minor", "Dark minor cadence for drill." },
        { "Drill", "NY Drill", "i VI iv V", "Harmonic Minor", "Orchestral NY drill feel." },

        // ---------------------------------------------------------------- Lo-Fi
        { "Lo-Fi / Chillhop", "Lo-Fi Descent", "IVmaj7 iii7 ii7 Imaj7", "Major", "Classic lo-fi descending 7ths." },
        { "Lo-Fi / Chillhop", "Lo-Fi ii-V-I", "ii9 V13 Imaj9 Imaj9", "Major", "Jazzy lo-fi cadence." },
        { "Lo-Fi / Chillhop", "Lo-Fi Minor", "i7 iv7 VImaj7 V7", "Harmonic Minor", "Rainy-day minor loop." },
        { "Lo-Fi / Chillhop", "Lo-Fi Royal Road", "IVmaj7 V7 iii7 vi7", "Major", "The royal road, lo-fi style." },
        { "Lo-Fi / Chillhop", "Bedroom Lo-Fi", "Imaj9 vi9 ii9 V9", "Major", "Warm extended turnaround." },
        { "Lo-Fi / Chillhop", "Study Beats", "vi9 IVmaj7 Imaj7 V7sus4", "Major", "Calm, focused loop." },
        { "Lo-Fi / Chillhop", "Dorian Lo-Fi", "i9 IV13", "Dorian", "Mellow Dorian vamp." },

        // ---------------------------------------------------------------- EDM
        { "EDM / Progressive House", "Progressive Anthem", "vi IV I V", "Major", "Festival progressive house." },
        { "EDM / Progressive House", "Euphoric", "IV V vi I", "Major", "Euphoric build and drop." },
        { "EDM / Progressive House", "Big Room Minor", "i VI III VII", "Natural Minor", "Big room minor anthem." },
        { "EDM / Progressive House", "Festival Lift", "IV I V vi", "Major", "Bright festival progression." },
        { "EDM / Progressive House", "Progressive Minor", "i VI VII i", "Natural Minor", "Driving minor progressive." },
        { "EDM / Progressive House", "Melodic Techno Minor", "i VII VI V", "Harmonic Minor", "Dark melodic techno." },
        { "House / Deep House", "Deep House Dorian", "i7 IV9", "Dorian", "Classic deep house minor 7 to IV9." },
        { "House / Deep House", "Deep House Min9", "i9 VImaj7 iv9 v7", "Natural Minor", "Lush minor 9 deep house." },
        { "House / Deep House", "Soulful House", "IVmaj9 V9 iii7 vi9", "Major", "Soulful house piano." },
        { "House / Deep House", "Piano House", "I V vi IV", "Major", "90s piano house." },
        { "House / Deep House", "Organ House", "i7 iv7 VII7 IIImaj7", "Natural Minor", "Organ stabs in a minor cycle." },
        { "Future Bass", "Future Bass Royal Road", "IVmaj7 V7 iii7 vi7", "Major", "Future bass favourite." },
        { "Future Bass", "Future Bass Emotional", "vi9 IVmaj9 Imaj9 V9", "Major", "Supersaw emotional loop." },
        { "Future Bass", "Kawaii Future Bass", "Imaj7 iii7 IVmaj7 V7", "Major", "Cute and bright." },
        { "Dubstep / Bass", "Dubstep Minor", "i VI III VII", "Natural Minor", "Melodic dubstep chords." },
        { "Dubstep / Bass", "Riddim Phrygian", "i bII", "Phrygian", "Menacing two-chord riddim." },
        { "Dubstep / Bass", "Melodic Dubstep", "VI VII i v", "Natural Minor", "Emotional melodic bass." },
        { "Trance", "Uplifting Trance", "vi IV I V", "Major", "Uplifting trance anthem." },
        { "Trance", "Classic Trance Minor", "i VII VI VII", "Natural Minor", "Classic trance arp progression." },
        { "Trance", "Epic Trance", "i VI III VII", "Natural Minor", "Epic breakdown chords." },
        { "Trance", "Psytrance Phrygian", "i bII", "Phrygian Dominant", "Psychedelic half-step tension." },
        { "Techno", "Dub Techno Chord", "i9:4", "Dorian", "One hypnotic minor 9 chord stab." },
        { "Techno", "Techno Minor Stab", "i7 i7 VII7 i7", "Natural Minor", "Minimal techno stab pattern." },
        { "Drum & Bass", "Liquid DnB", "Imaj7 iii7 vi9 IVmaj9", "Major", "Liquid drum & bass pads." },
        { "Drum & Bass", "Liquid Minor", "i9 VImaj7 III VII", "Natural Minor", "Rolling minor liquid." },
        { "Drum & Bass", "Jungle Minor", "i7 iv7", "Dorian", "Jungle pads." },
        { "UK Garage", "2-Step Garage", "i7 iv7 VII7 IIImaj7", "Natural Minor", "Shuffled garage chords." },
        { "UK Garage", "Garage ii-V", "ii7 V7 Imaj7 vi7", "Major", "Bright garage organ." },
        { "Synthwave / Retrowave", "Night Drive", "i VI III VII", "Natural Minor", "80s neon minor progression." },
        { "Synthwave / Retrowave", "Outrun", "VI VII i i", "Natural Minor", "Rising into the minor tonic." },
        { "Synthwave / Retrowave", "Lydian Sunset", "I II", "Lydian", "Dreamy Lydian major II." },
        { "Synthwave / Retrowave", "Retro Ballad", "vi IV I V", "Major", "80s power ballad." },
        { "Phonk", "Phonk Cowbell", "i bII", "Phrygian", "Memphis-style dark half step." },
        { "Phonk", "Drift Phonk", "i iv v i", "Natural Minor", "Aggressive drift phonk loop." },
        { "Hyperpop", "Hyperpop Rush", "IV V vi I", "Major", "Maximal, sugary rush." },
        { "Hyperpop", "Glitch Minor", "vi IV I V", "Major", "Distorted emotional pop." },

        // ---------------------------------------------------------------- Country / Folk / Celtic
        { "Country", "Country Classic", "I IV V I", "Major", "Honest three-chord country." },
        { "Country", "Country Ballad", "I V vi IV", "Major", "Modern country ballad." },
        { "Country", "Two-Step", "I I IV IV V V I I", "Major", "Dancehall two-step." },
        { "Country", "Nashville Walk", "I iii IV V", "Major", "Nashville number walkup." },
        { "Country", "Honky-Tonk", "I I7 IV IV I V7 I I", "Major", "Bar-room honky-tonk." },
        { "Country", "Bluegrass", "I IV I V I", "Major Pentatonic", "Fast bluegrass changes." },
        { "Folk / Acoustic", "Folk Simple", "I IV V I", "Major", "Campfire folk." },
        { "Folk / Acoustic", "Folk Minor", "i VII VI VII", "Natural Minor", "Minor folk ballad." },
        { "Folk / Acoustic", "Singer-Songwriter", "I iii vi IV", "Major", "Intimate singer-songwriter loop." },
        { "Folk / Acoustic", "Dorian Folk", "i VII i IV", "Dorian", "Old-world Dorian folk." },
        { "Celtic", "Celtic Dorian", "i VII i VII", "Dorian", "Irish/Scottish Dorian tune." },
        { "Celtic", "Celtic Mixolydian", "I bVII I bVII", "Mixolydian", "Pipe-tune Mixolydian." },
        { "Celtic", "Irish Air", "I IV vi V", "Major", "Gentle Irish air." },
        { "Celtic", "Celtic Aeolian", "i VII VI VII", "Natural Minor", "Haunting minor reel." },

        // ---------------------------------------------------------------- Reggae / Dancehall / Afro
        { "Reggae", "Roots Reggae", "I IV", "Major", "One-drop roots reggae." },
        { "Reggae", "Reggae Minor", "i iv", "Natural Minor", "Minor skank." },
        { "Reggae", "Reggae Four", "I IV V IV", "Major", "Classic reggae four-chord." },
        { "Reggae", "Dorian Reggae", "i IV", "Dorian", "Dub-style Dorian groove." },
        { "Dancehall", "Dancehall Minor", "i VI III VII", "Natural Minor", "Modern dancehall loop." },
        { "Dancehall", "Dancehall Vamp", "i iv", "Natural Minor", "Bouncy minor vamp." },
        { "Afrobeats / Amapiano", "Afrobeats Loop", "I IV vi V", "Major", "Sunny Afrobeats loop." },
        { "Afrobeats / Amapiano", "Afro Highlife", "I IV V IV", "Major", "Highlife guitar changes." },
        { "Afrobeats / Amapiano", "Afro Minor", "vi IV I V", "Major", "Afro-pop emotional loop." },
        { "Afrobeats / Amapiano", "Amapiano Jazz", "ii7 iii7 IVmaj7 V7", "Major", "Log-drum jazzy amapiano." },
        { "Afrobeats / Amapiano", "Amapiano Minor", "i9 iv9 VII9 IIImaj9", "Natural Minor", "Deep minor amapiano." },

        // ---------------------------------------------------------------- Latin
        { "Latin", "Reggaeton Minor", "i VI III VII", "Natural Minor", "Dembow reggaeton staple." },
        { "Latin", "Reggaeton Dark", "i iv VI V", "Harmonic Minor", "Darker perreo changes." },
        { "Latin", "Bachata", "i iv V i", "Harmonic Minor", "Romantic bachata." },
        { "Latin", "Salsa Montuno", "I IV V IV", "Major", "Piano montuno vamp." },
        { "Latin", "Salsa Minor", "i iv V7 i", "Harmonic Minor", "Minor salsa." },
        { "Latin", "Bossa Nova", "Imaj7 II7 ii7 bII7 Imaj7", "Major", "Bossa nova changes." },
        { "Latin", "Bossa ii-V", "ii7 V7b9 Imaj7 vi7", "Major", "Smooth bossa cadence." },
        { "Latin", "Tango", "i iv V7 i", "Harmonic Minor", "Dramatic tango." },
        { "Latin", "Cumbia", "I V7 I V7", "Major", "Cumbia two-chord." },
        { "Latin", "Mariachi", "I V7 I V7 IV I V7 I", "Major", "Ranchera changes." },

        // ---------------------------------------------------------------- Flamenco / Spanish
        { "Flamenco / Spanish", "Andalusian Cadence", "i VII VI V", "Natural Minor", "The descending flamenco cadence." },
        { "Flamenco / Spanish", "Flamenco Phrygian", "I bII bIII bII I", "Phrygian Dominant", "Phrygian-dominant flamenco on the major tonic." },
        { "Flamenco / Spanish", "Spanish Minor", "i bII VII i", "Phrygian", "Spanish Phrygian colour." },
        { "Flamenco / Spanish", "Malaguena-style", "i VII VI V V", "Harmonic Minor", "Andalusian descent landing on V." },

        // ---------------------------------------------------------------- Classical
        { "Classical / Baroque", "Authentic Cadence", "I IV V I", "Major", "Tonic-subdominant-dominant-tonic." },
        { "Classical / Baroque", "Plagal Cadence", "I IV I", "Major", "The 'Amen' cadence." },
        { "Classical / Baroque", "Deceptive Cadence", "I IV V vi", "Major", "V resolves to vi instead of I." },
        { "Classical / Baroque", "Diatonic Circle of Fifths", "I IV vii° iii vi ii V I", "Major", "Every diatonic chord by fifths." },
        { "Classical / Baroque", "Pachelbel Canon", "I V vi iii IV I IV V", "Major", "The canon sequence." },
        { "Classical / Baroque", "Romanesca", "III VII i V", "Natural Minor", "Renaissance descending-fourths pattern." },
        { "Classical / Baroque", "La Folia", "i V i VII III VII i V", "Harmonic Minor", "Baroque dance theme." },
        { "Classical / Baroque", "Lament Bass", "i v/7 iv/6 V", "Harmonic Minor", "Descending tetrachord lament." },
        { "Classical / Baroque", "Neapolitan Cadence", "i iv bII V i", "Harmonic Minor", "Flat II on the way to V." },
        { "Classical / Baroque", "Picardy Third", "i iv V I", "Harmonic Minor", "Minor piece ending on a major tonic." },
        { "Classical / Baroque", "Monte", "I7 IV II7 V", "Major", "Galant rising sequence." },
        { "Classical / Baroque", "Fonte", "VI7 ii V7 I", "Major", "Galant falling sequence." },
        { "Classical / Baroque", "German Sixth (enharmonic)", "i iv VI7 V", "Harmonic Minor", "Augmented-sixth sound resolving to V." },
        { "Classical / Baroque", "Chromatic Mediants", "I bVI I III", "Major", "Romantic-era colour shifts." },

        // ---------------------------------------------------------------- Cinematic
        { "Cinematic / Film", "Epic Minor", "i VI III VII", "Natural Minor", "Blockbuster trailer chords." },
        { "Cinematic / Film", "Heroic Rise", "bVI bVII I", "Major", "Triumphant victory moment." },
        { "Cinematic / Film", "Chromatic Mediant", "I bVI", "Major", "Magical, awe-filled shift." },
        { "Cinematic / Film", "Major Mediant", "I III", "Major", "Bright wonder." },
        { "Cinematic / Film", "Lydian Fantasy", "I II I II", "Lydian", "Fantasy and flight." },
        { "Cinematic / Film", "Villain", "i bII i bVI", "Phrygian", "Ominous antagonist theme." },
        { "Cinematic / Film", "Tragic", "i iv i V", "Harmonic Minor", "Tragic, operatic minor." },
        { "Cinematic / Film", "Wonder", "I III IV iv", "Major", "Childlike wonder turning bittersweet." },
        { "Cinematic / Film", "Interstellar Pulse", "i VI i VI", "Natural Minor", "Minimal organ pulse." },
        { "Cinematic / Film", "Hope Rising", "vi IV I V", "Major", "Emotional uplift." },
        { "Cinematic / Film", "Minor Hero", "i VI VII i", "Natural Minor", "Determined hero theme." },
        { "Cinematic / Film", "Suspense", "i bVI i bVI", "Natural Minor", "Tense alternating minor and flat VI." },

        // ---------------------------------------------------------------- Video Game
        { "Video Game", "Victory Fanfare", "bVI bVII I", "Major", "Level-clear fanfare." },
        { "Video Game", "Overworld", "I IV V I", "Major", "Cheerful overworld." },
        { "Video Game", "Dungeon", "i bVI bVII i", "Natural Minor", "Dark dungeon crawl." },
        { "Video Game", "Boss Battle", "i bII i bII", "Phrygian", "Intense boss fight." },
        { "Video Game", "Town Theme", "I vi IV V", "Major", "Cozy town music." },
        { "Video Game", "JRPG Emotional", "IVmaj7 V7 iii7 vi7", "Major", "Emotional JRPG cutscene." },

        // ---------------------------------------------------------------- Ambient
        { "Ambient", "Floating Major 7", "Imaj7 IVmaj7", "Major", "Weightless and still." },
        { "Ambient", "Lydian Drift", "Imaj7 II", "Lydian", "Bright Lydian float." },
        { "Ambient", "Minor Haze", "i9 VImaj7", "Natural Minor", "Hazy minor pads." },
        { "Ambient", "Suspended Bliss", "Isus2 IVsus2 vi7 Vsus4", "Major", "Open suspended chords." },

        // ---------------------------------------------------------------- K-Pop / J-Pop / Anime
        { "K-Pop / J-Pop / Anime", "Royal Road", "IVmaj7 V7 iii7 vi", "Major", "The J-pop 'royal road' progression." },
        { "K-Pop / J-Pop / Anime", "Komuro Progression", "vi IV V I", "Major", "90s J-pop staple." },
        { "K-Pop / J-Pop / Anime", "Anime Climax", "IV V iii vi ii V I", "Major", "Big anime opening chorus." },
        { "K-Pop / J-Pop / Anime", "K-Pop Hook", "vi IV I V", "Major", "Catchy K-pop loop." },
        { "K-Pop / J-Pop / Anime", "Just The Two Of Us (J-Pop)", "| IVmaj7 | III7 | vi7 | v7 I7 |", "Major", "City-pop / J-pop loop." },
        { "K-Pop / J-Pop / Anime", "City Pop", "IVmaj7 iii7 vi7 ii7 V7sus4", "Major", "Breezy 80s city pop." },

        // ---------------------------------------------------------------- Worship / Musical Theatre
        { "Worship / CCM", "Worship Anthem", "IV I V vi", "Major", "Contemporary worship standard." },
        { "Worship / CCM", "Worship Build", "I V vi IV", "Major", "Congregational anthem." },
        { "Worship / CCM", "Bridge Climb", "IV V vi I", "Major", "Rising bridge." },
        { "Musical Theatre", "Broadway Line Cliché", "I I+ I6 I7", "Major", "Show-tune chromatic climb." },
        { "Musical Theatre", "Broadway Turnaround", "Imaj7 VI7 ii7 V7", "Major", "Show-tune turnaround with a secondary dominant." },
        { "Musical Theatre", "Heartfelt Ballad", "I iii IV iv", "Major", "Big ballad with a borrowed minor iv." },

        // ---------------------------------------------------------------- World
        { "Middle Eastern", "Hijaz Vamp", "I bII", "Phrygian Dominant", "Middle Eastern Hijaz colour." },
        { "Middle Eastern", "Maqam Minor", "I bII bvii I", "Phrygian Dominant", "Phrygian-dominant cadence." },
        { "Middle Eastern", "Arabic Pop", "i VII VI V", "Harmonic Minor", "Andalusian descent with Arabic flavour." },
        { "Middle Eastern", "Double Harmonic Drone", "I bII I", "Double Harmonic Major", "Exotic double-harmonic drone." },
        { "Indian / Bollywood", "Filmi Minor", "i VI VII i", "Natural Minor", "Bollywood minor ballad." },
        { "Indian / Bollywood", "Bhairav Drone", "I bII I bII", "Double Harmonic Major", "Raga Bhairav colour over a drone." },
        { "Indian / Bollywood", "Bollywood Dance", "i iv VII III", "Natural Minor", "Upbeat filmi dance." },

        // ---------------------------------------------------------------- Modal
        { "Modal", "Ionian Cadence", "I IV V I", "Major", "Plain major." },
        { "Modal", "Dorian Vamp", "i IV", "Dorian", "The Dorian major IV." },
        { "Modal", "Phrygian Vamp", "i bII", "Phrygian", "The Phrygian flat II." },
        { "Modal", "Lydian Vamp", "I II", "Lydian", "The Lydian major II." },
        { "Modal", "Mixolydian Vamp", "I bVII", "Mixolydian", "The Mixolydian flat VII." },
        { "Modal", "Aeolian Vamp", "i VII", "Natural Minor", "Natural minor flat VII." },
        { "Modal", "Locrian", "i° bII", "Locrian", "Unstable Locrian tonic." },
        { "Modal", "Harmonic Minor", "i V", "Harmonic Minor", "Minor with a major V." },
        { "Modal", "Melodic Minor", "imaj7 IV7", "Melodic Minor", "Jazz-minor colour." },
    };

    struct Library
    {
        std::vector<ProgressionDef> items;
        std::vector<std::string> genres;

        Library()
        {
            for (const auto& e : entries)
            {
                items.push_back ({ e.genre, e.name, e.numerals, e.scale, e.description });

                bool known = false;
                for (const auto& g : genres)
                    known = known || g == e.genre;
                if (! known)
                    genres.push_back (e.genre);
            }
        }
    };

    const Library& library()
    {
        static const Library lib;
        return lib;
    }
} // namespace

const std::vector<ProgressionDef>& progressionLibrary() { return library().items; }

const std::vector<std::string>& progressionGenres() { return library().genres; }

std::vector<int> progressionsInGenre (std::string_view genre)
{
    std::vector<int> out;
    const auto& items = progressionLibrary();
    for (size_t i = 0; i < items.size(); ++i)
        if (equalsIgnoreCase (items[i].genre, genre))
            out.push_back (static_cast<int> (i));
    return out;
}

std::vector<int> searchProgressions (std::string_view query)
{
    std::vector<std::string> words;
    {
        std::istringstream in { std::string (query) };
        for (std::string w; in >> w;)
            words.push_back (toLower (w));
    }

    std::vector<int> out;
    const auto& items = progressionLibrary();

    for (size_t i = 0; i < items.size(); ++i)
    {
        const auto& p = items[i];
        const auto haystack = toLower (p.genre + " " + p.name + " " + p.numerals + " " + p.scale + " " + p.description);

        bool all = true;
        for (const auto& w : words)
            all = all && haystack.find (w) != std::string::npos;

        if (all)
            out.push_back (static_cast<int> (i));
    }

    return out;
}

} // namespace melodee
