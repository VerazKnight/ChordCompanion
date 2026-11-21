#include <gtest/gtest.h>

#include "chordTheory.hpp"
#include "pianoKeyboard.hpp"
#include "pianoLayout.hpp"

#include <unordered_set>

// Component tests combine PianoLayout, NoteState and ChordTheory using
// resolved UI notes and MIDI messages, without opening a window or device.

namespace {

namespace CT = ChordTheory;

using Notes = std::unordered_set<int>;

const Notes C_MAJOR = {60, 64, 67};
// ---------------------------------------------------------------- NoteInput
// UI owns QWERTY mapping and mouse latching. These tests supply the resulting
// UI note set to the real backend instead of recreating those UI behaviors.

TEST(NoteInput, UiChordIsRecognizedAndDescribed) {
    NoteState input;
    input.setUi(C_MAJOR);

    const Notes pressed = input.pressedKeys();
    EXPECT_EQ(pressed, C_MAJOR);
    const auto chord = CT::identifyChord(pressed);
    ASSERT_TRUE(chord.found);
    EXPECT_EQ(chord.rootPc, 0);
    EXPECT_EQ(chord.typeIndex, CT::MAJOR);
    EXPECT_EQ(PianoLayout::describeKeys(PianoLayout::keysToHighlight(pressed)), "C4 E4 G4");
}

TEST(NoteInput, MouseHitTestSuppliesANoteThatUiCanRelease) {
    NoteState input;
    const int note = PianoLayout::noteAtPoint(20.0f, 140.0f);
    ASSERT_EQ(note, 60);
    input.setUi({note});
    EXPECT_EQ(input.pressedKeys(), (Notes{60}));

    // An empty UI snapshot means neither mouse nor QWERTY holds any notes.
    input.setUi({});
    EXPECT_TRUE(input.pressedKeys().empty());
}

TEST(NoteInput, MidiAndUiNotesCombineIntoOneChord) {
    NoteState input;
    input.setMidi(0, 60, 90);
    // UI has already combined the mouse's E4 and QWERTY keyboard's G4.
    input.setUi({64, 67});
    EXPECT_EQ(input.pressedKeys(), C_MAJOR);

    const auto chord = CT::identifyChord(input.pressedKeys());
    ASSERT_TRUE(chord.found);
    EXPECT_EQ(chord.rootPc, 0);
    EXPECT_EQ(chord.typeIndex, CT::MAJOR);
    EXPECT_EQ(chord.inversion, 0);
}

TEST(NoteInput, OutsideMouseHitAddsNothingToMidiNotes) {
    NoteState input;
    input.setMidi(0, 60, 90);
    input.setMidi(0, 64, 90);
    const int note = PianoLayout::noteAtPoint(900.0f, 900.0f);
    ASSERT_EQ(note, -1);
    input.setUi({});  // No valid mouse hit and no held QWERTY keys.

    EXPECT_EQ(input.pressedKeys(), (Notes{60, 64}));
    EXPECT_FALSE(CT::identifyChord(input.pressedKeys()).found);
}

TEST(NoteInput, ReleasingUiNotesPreservesMidiChordUntilMidiReleases) {
    NoteState input;
    for (int note : C_MAJOR) {
        input.setMidi(0, note, 90);
    }
    input.setUi(C_MAJOR);

    // UI and MIDI share the chord; releasing UI must not send note-offs.
    EXPECT_TRUE(input.setUi({}).empty());
    EXPECT_EQ(input.pressedKeys(), C_MAJOR);
    EXPECT_TRUE(CT::identifyChord(input.pressedKeys()).found);

    for (int note : C_MAJOR) {
        input.setMidi(0, note, 0);
    }
    EXPECT_TRUE(input.pressedKeys().empty());
    EXPECT_FALSE(CT::identifyChord(input.pressedKeys()).found);
}

}  // namespace
