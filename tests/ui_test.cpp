#include <gtest/gtest.h>

#include "chordTheory.hpp"
#include "learningSession.hpp"
#include "pianoInput.hpp"
#include "pianoLayout.hpp"

#include <string>
#include <unordered_set>
#include <vector>

// Unit tests for the UI logic. Each unit is a plain function from a header
// with no ImGui, GLFW or MIDI inside, so nothing needs to be faked.
//
//   NoteAtPoint      PianoLayout::noteAtPoint      mouse click -> piano key
//   IdentifyChord    ChordTheory::identifyChord    pressed keys -> chord + inversion
//   DetectKey        ChordTheory::detectKey        chords played -> guessed key
//   JudgePractice    Learning::judgePractice       friendly reply to a practice chord
//   ComputerKeyNote  PianoInput::computerKeyNote   computer key + octave -> piano key

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

// ---------------------------------------------------------------- judgePractice

TEST(JudgePractice, PlayingTheChosenChordAndInversionIsCheered) {
    CT::IdentifiedChord played{true, C, CT::MAJOR, 1};

    Learning::PracticeOutcome out = Learning::judgePractice(C, CT::MAJOR, 1, played);

    EXPECT_EQ(out.result, Learning::PracticeResult::Correct);
    EXPECT_EQ(out.message, "Yes! C Major, 1st inversion. Nailed it! Pick another one to keep going.");
}

TEST(JudgePractice, RightChordWrongInversionGetsAHelpfulTip) {
    CT::IdentifiedChord played{true, C, CT::MAJOR, 0};

    Learning::PracticeOutcome out = Learning::judgePractice(C, CT::MAJOR, 1, played);

    EXPECT_EQ(out.result, Learning::PracticeResult::WrongInversion);
    EXPECT_EQ(out.message,
              "So close! That is C Major, but in root position. For 1st inversion put E at the bottom.");
}

TEST(JudgePractice, DifferentChordIsAnsweredKindly) {
    // The player may always wander off: no error, just a friendly note.
    CT::IdentifiedChord played{true, F, CT::MAJOR, 0};

    Learning::PracticeOutcome out = Learning::judgePractice(C, CT::MAJOR, 1, played);

    EXPECT_EQ(out.result, Learning::PracticeResult::DifferentChord);
    EXPECT_EQ(out.message,
              "Oh, you went with F Major instead of C Major. No problem! Press Show again whenever you want to try it.");
}

TEST(JudgePractice, SameRootButDifferentTypeCountsAsDifferentChord) {
    CT::IdentifiedChord played{true, A, CT::MAJOR, 0};

    Learning::PracticeOutcome out = Learning::judgePractice(A, CT::MINOR, 0, played);

    EXPECT_EQ(out.result, Learning::PracticeResult::DifferentChord);
    EXPECT_EQ(out.message,
              "Oh, you went with A Major instead of A Minor. No problem! Press Show again whenever you want to try it.");
}

// ---------------------------------------------------------------- computerKeyNote

TEST(ComputerKeyNote, AKeyPlaysC4InTheLowOctave) {
    EXPECT_EQ(PianoInput::computerKeyNote('A', 0), 60);
}

TEST(ComputerKeyNote, SemicolonPlaysE5TheLastKeyOfTheLowOctave) {
    EXPECT_EQ(PianoInput::computerKeyNote(';', 0), 76);
}

TEST(ComputerKeyNote, OctaveShiftReachesTheUpperKeysAndIgnoresKeysPastB5) {
    // With X (shift 12) the same keys play one octave higher: A becomes C5.
    EXPECT_EQ(PianoInput::computerKeyNote('A', 12), 72);

    // P would be D#6 (87), which is not on the 2 octave piano.
    EXPECT_EQ(PianoInput::computerKeyNote('P', 12), -1);
}

TEST(ComputerKeyNote, LetterThatIsNotAPianoKeyPlaysNothing) {
    EXPECT_EQ(PianoInput::computerKeyNote('Q', 0), -1);
}

}  // namespace
