#include <gtest/gtest.h>

#include "chordTheory.hpp"
#include "pianoLayout.hpp"

#include <vector>

// Unit tests for the UI logic. Each unit is a plain function from a header
// with no ImGui, GLFW or MIDI inside, so nothing needs to be faked.
//
//   NoteAtPoint      PianoLayout::noteAtPoint      mouse click -> piano key
//   IdentifyChord    ChordTheory::identifyChord    pressed keys -> chord + inversion
//   DetectKey        ChordTheory::detectKey        chords played -> guessed key

namespace {

namespace CT = ChordTheory;

constexpr int C = 0;
constexpr int D = 2;
constexpr int F = 5;
constexpr int G = 7;
constexpr int A = 9;

// ---------------------------------------------------------------- noteAtPoint

TEST(NoteAtPoint, ClickOnWhiteKeyFindsThatKey) {
    // x=20, y=150 is the low part of the first key (C4 = MIDI 60).
    EXPECT_EQ(PianoLayout::noteAtPoint(20.0f, 150.0f), 60);
}

TEST(NoteAtPoint, ClickOnBlackKeyFindsBlackKeyNotWhiteKeyUnderIt) {
    // x=35, y=50 is the top of the C# key, where it overlaps the C key.
    EXPECT_EQ(PianoLayout::noteAtPoint(35.0f, 50.0f), 61);
}

TEST(NoteAtPoint, FirstRowBelowBlackKeysFindsWhiteKeyAgain) {
    // Boundary: black keys are 100 pixels high, so y=100 is white key C4 again.
    EXPECT_EQ(PianoLayout::noteAtPoint(35.0f, 99.0f), 61);
    EXPECT_EQ(PianoLayout::noteAtPoint(35.0f, 100.0f), 60);
}

TEST(NoteAtPoint, ClicksOutsidePianoFindNoKey) {
    EXPECT_EQ(PianoLayout::noteAtPoint(-1.0f, 10.0f), -1);   // left of the piano
    EXPECT_EQ(PianoLayout::noteAtPoint(560.0f, 10.0f), -1);  // right of 14 white keys
    EXPECT_EQ(PianoLayout::noteAtPoint(20.0f, 160.0f), -1);  // below the keys
}

// ---------------------------------------------------------------- identifyChord

TEST(IdentifyChord, TriadInRootPositionIsRecognized) {
    CT::IdentifiedChord chord = CT::identifyChord({60, 64, 67});  // C4 E4 G4

    EXPECT_TRUE(chord.found);
    EXPECT_EQ(chord.rootPc, C);
    EXPECT_EQ(chord.typeIndex, CT::MAJOR);
    EXPECT_EQ(chord.inversion, 0);
}

TEST(IdentifyChord, InversionIsRecognizedFromTheBottomNote) {
    CT::IdentifiedChord chord = CT::identifyChord({64, 67, 72});  // E4 G4 C5

    // Same chord as above, but E is at the bottom: C Major, 1st inversion.
    EXPECT_TRUE(chord.found);
    EXPECT_EQ(chord.rootPc, C);
    EXPECT_EQ(chord.typeIndex, CT::MAJOR);
    EXPECT_EQ(chord.inversion, 1);
}

TEST(IdentifyChord, TwoNotesAreNotAFullChordYet) {
    EXPECT_FALSE(CT::identifyChord({60, 64}).found);
    EXPECT_FALSE(CT::identifyChord({}).found);
}

TEST(IdentifyChord, ThreeNotesSideBySideAreNotAChord) {
    // C4 C#4 D4 is not a major, minor, diminished or augmented triad.
    EXPECT_FALSE(CT::identifyChord({60, 61, 62}).found);
}

// ---------------------------------------------------------------- detectKey

TEST(DetectKey, KeyIsFoundFromTheChordsThemselves) {
    // F, G, Am, G never starts or ends on C, but all four chords are in C major.
    std::vector<CT::PlayedChord> history = {
        {F, CT::MAJOR}, {G, CT::MAJOR}, {A, CT::MINOR}, {G, CT::MAJOR}};

    EXPECT_EQ(CT::detectKey(history), C);
}

TEST(DetectKey, NothingPlayedYetGivesNoKey) {
    EXPECT_EQ(CT::detectKey({}), -1);
}

TEST(DetectKey, MinorSongIsPlacedInItsRelativeMajor) {
    // Am, Dm, Em, Am ends on A minor, whose relative major is C major.
    std::vector<CT::PlayedChord> history = {
        {A, CT::MINOR}, {D, CT::MINOR}, {4, CT::MINOR}, {A, CT::MINOR}};

    EXPECT_EQ(CT::detectKey(history), C);
}

TEST(DetectKey, OneChordOutsideTheKeyDoesNotChangeTheKey) {
    // D Major is not in C major, but one odd chord among four does not win.
    std::vector<CT::PlayedChord> history = {
        {F, CT::MAJOR}, {G, CT::MAJOR}, {A, CT::MINOR}, {G, CT::MAJOR}, {D, CT::MAJOR}};

    EXPECT_EQ(CT::detectKey(history), C);
}

}  // namespace
