// Unit tests for ChordLogic (chordLogic.hpp / chordLogic.cpp).
//
// How to read these tests:
//  - Each TEST(ChordLogic, Name) { ... } is one independent test. GoogleTest runs
//    them all and reports which passed or failed.
//  - EXPECT_EQ(actual, expected) checks that two values are equal. If not, the test
//    is marked failed but keeps running, so you see every problem at once.
//  - ASSERT_EQ is the same but stops the test immediately. We use it before indexing
//    into a vector, so a wrong size gives a clear failure instead of a crash.
//  - Keys are MIDI note numbers, the same numbers PianoKeyboard::getPressedKeys()
//    returns. Handy ones: 60 = middle C (C4), 64 = E4, 67 = G4, 72 = C5.
//    Pitch classes are 0-11 with C = 0, so E = 4 and G = 7.
//    That is why a C major triad (C, E, G) is the semitone pattern 0, 4, 7.

#include <gtest/gtest.h>

#include "chordLogic.hpp"

#include <unordered_set>

namespace {

// Shorthand so tests read as "Keys{60, 64, 67}" instead of the full type name.
using Keys = std::unordered_set<int>;
using Status = ChordAnalysis::Status;

// ---------------------------------------------------------------------------
// Edge cases: nothing, or one note, held.
// ---------------------------------------------------------------------------

TEST(ChordLogic, NoKeysHeldIsEmpty) {
    ChordLogic logic;

    // With no keys down there is nothing to analyze: status Empty, empty label.
    EXPECT_EQ(logic.analyze(Keys{}).status, Status::Empty);
    EXPECT_EQ(logic.identifyChord(Keys{}), "");
}

TEST(ChordLogic, SingleNoteIgnoresOctave) {
    ChordLogic logic;

    // One key is not a chord, but the app still shows its note name.
    EXPECT_EQ(logic.analyze(Keys{60}).status, Status::SingleNote);
    EXPECT_EQ(logic.identifyChord(Keys{60}), "C");

    // 48 (C3) and 60 (C4) are the same note an octave apart. Detection works on
    // pitch classes, so this is still a single note rather than a two-note interval.
    EXPECT_EQ(logic.analyze(Keys{48, 60}).status, Status::SingleNote);
}

// ---------------------------------------------------------------------------
// The one chord currently in the table: the major triad (0, 4, 7).
// ---------------------------------------------------------------------------

TEST(ChordLogic, MajorTriadIsRecognizedOnAnyRootAndOctave) {
    ChordLogic logic;

    // C4, E4, G4 -> C major.
    EXPECT_EQ(logic.identifyChord(Keys{60, 64, 67}), "C");

    // Same shape starting on D (D4, F#4, A4) -> D major. The chord table stores the
    // shape once and it is matched against every possible root.
    EXPECT_EQ(logic.identifyChord(Keys{62, 66, 69}), "D");

    // Wide voicing: C3, E4, G5 are far apart but are still the notes C, E, G.
    EXPECT_EQ(logic.identifyChord(Keys{48, 64, 79}), "C");

    // Doubling a note in another octave (an extra low C) does not change the chord.
    EXPECT_EQ(logic.identifyChord(Keys{48, 60, 64, 67}), "C");

    // The full analysis also says what kind of chord it is.
    const ChordAnalysis analysis = logic.analyze(Keys{60, 64, 67});
    EXPECT_EQ(analysis.status, Status::Recognized);
    ASSERT_EQ(analysis.matches.size(), 1u); // exactly one reading for this chord
    EXPECT_EQ(analysis.matches.front().typeName, "Major triad");
    EXPECT_FALSE(analysis.matches.front().inversion); // C is the lowest note
}

TEST(ChordLogic, InversionsNameTheLowestNoteAsBass) {
    ChordLogic logic;

    // C major with E at the bottom (E4, G4, C5) is "first inversion". It is still
    // a C chord, written C/E ("C over E").
    EXPECT_EQ(logic.identifyChord(Keys{64, 67, 72}), "C/E");

    // With G at the bottom (G4, C5, E5) it is second inversion: C/G.
    EXPECT_EQ(logic.identifyChord(Keys{67, 72, 76}), "C/G");

    // The detailed result records which note is the root and which is the bass.
    const ChordAnalysis analysis = logic.analyze(Keys{64, 67, 72});
    ASSERT_EQ(analysis.matches.size(), 1u);
    EXPECT_TRUE(analysis.matches.front().inversion);
    EXPECT_EQ(analysis.matches.front().root, 0); // C
    EXPECT_EQ(analysis.matches.front().bass, 4); // E
}

// ---------------------------------------------------------------------------
// Things that should NOT be called a major triad.
// ---------------------------------------------------------------------------

TEST(ChordLogic, UnknownCombinationIsUnnamedNotWrong) {
    ChordLogic logic;

    // C minor (C, Eb, G) is a real chord but it is not in the table yet. The app
    // should say it does not know it rather than guess a wrong name.
    const ChordAnalysis analysis = logic.analyze(Keys{60, 63, 67});
    EXPECT_EQ(analysis.status, Status::Unnamed);
    EXPECT_TRUE(analysis.matches.empty());

    // The fallback label lists the notes as semitones above the lowest note:
    // Eb is 3 above C and G is 7 above C.
    EXPECT_EQ(analysis.label, "C (unnamed: +3 +7)");
}

TEST(ChordLogic, MoreThanFourDistinctNotesIsTooMany) {
    ChordLogic logic;

    // C, D, E, G, A = five different notes, over the 4-note limit.
    const ChordAnalysis analysis = logic.analyze(Keys{60, 62, 64, 67, 69});
    EXPECT_EQ(analysis.status, Status::TooManyNotes);
    EXPECT_EQ(analysis.distinctNotes, 5);
}

// ---------------------------------------------------------------------------
// Helper functions.
// ---------------------------------------------------------------------------

TEST(ChordLogic, NoteNamesIncludeOctaveWhereRequested) {
    // noteName takes any number and gives just the letter (pitch class).
    EXPECT_EQ(ChordLogic::noteName(0), "C");
    EXPECT_EQ(ChordLogic::noteName(61), "Db"); // 61 % 12 = 1, a black key spelled with a flat

    // midiNoteName adds the octave. Middle C (60) is C4; the lowest piano key (21) is A0.
    EXPECT_EQ(ChordLogic::midiNoteName(60), "C4");
    EXPECT_EQ(ChordLogic::midiNoteName(21), "A0");
}

TEST(ChordLogic, ChordPitchClassesListNotesFromTheRoot) {
    ChordLogic logic;

    // "" is the suffix for a major triad. Starting on C: C, E, G = 0, 4, 7.
    EXPECT_EQ(logic.chordPitchClasses(0, ""), (std::vector<int>{0, 4, 7}));

    // Starting on G (7): G, B, D = 7, 11, 2 (the numbers wrap past 11 back to 0).
    EXPECT_EQ(logic.chordPitchClasses(7, ""), (std::vector<int>{7, 11, 2}));

    // A suffix that is not in the table gives an empty list.
    EXPECT_TRUE(logic.chordPitchClasses(0, "nonsense").empty());
}

} // namespace