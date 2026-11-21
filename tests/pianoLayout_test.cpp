// Tests for the fixed two-octave layout used by UI, without a window.
#include <gtest/gtest.h>

#include "pianoLayout.hpp"

#include <vector>

namespace {
namespace PL = PianoLayout;

TEST(PianoLayout, BlackKeysAreTheFiveSharpsOfEachOctave) {
    for (int pc : {1, 3, 6, 8, 10}) {
        EXPECT_TRUE(PL::isBlackKey(pc));
        EXPECT_TRUE(PL::isBlackKey(pc + 60));
    }
    for (int pc : {0, 2, 4, 5, 7, 9, 11}) {
        EXPECT_FALSE(PL::isBlackKey(pc));
    }
}

TEST(PianoLayout, BlackKeyClassificationWrapsNegativePitchClasses) {
    EXPECT_FALSE(PL::isBlackKey(-1)); // B
    EXPECT_TRUE(PL::isBlackKey(-2));  // A#
}

TEST(PianoLayout, VisibleRangeIncludesC4ThroughB5Only) {
    EXPECT_FALSE(PL::isInRange(59));
    EXPECT_TRUE(PL::isInRange(60));
    EXPECT_TRUE(PL::isInRange(83));
    EXPECT_FALSE(PL::isInRange(84));
}

TEST(PianoLayout, WhiteKeysOccupyConsecutiveColumns) {
    const std::vector<int> whiteNotes{60, 62, 64, 65, 67, 69, 71,
                                      72, 74, 76, 77, 79, 81, 83};
    for (int column = 0; column < 14; ++column) {
        EXPECT_EQ(PL::whiteKeyIndex(whiteNotes[column]), column);
        EXPECT_FLOAT_EQ(PL::keyX(whiteNotes[column]), column * 40.0f);
    }
}

TEST(PianoLayout, BlackKeysStraddleTheirWhiteKeyBoundaries) {
    // C#4 and A#5 are centered on the boundaries at x=40 and x=520.
    EXPECT_FLOAT_EQ(PL::keyX(61), 27.0f);
    EXPECT_FLOAT_EQ(PL::keyX(82), 507.0f);
    EXPECT_EQ(PL::whiteKeyIndex(61), -1);
}

TEST(PianoLayout, InvalidLayoutNotesHaveNoPosition) {
    for (int note : {-1, 59, 84, 128}) {
        EXPECT_EQ(PL::whiteKeyIndex(note), -1);
        EXPECT_FLOAT_EQ(PL::keyX(note), -1.0f);
    }
}

TEST(PianoLayout, HighlightsAreSortedAndLimitedToVisibleNotes) {
    EXPECT_EQ(PL::keysToHighlight({83, 59, 67, 60, 84}),
              (std::vector<int>{60, 67, 83}));
    EXPECT_TRUE(PL::keysToHighlight({}).empty());
    EXPECT_TRUE(PL::keysToHighlight({0, 127}).empty());
}

TEST(PianoLayout, NoteNamesIncludeOctavesAndRejectInvalidMidiNotes) {
    EXPECT_EQ(PL::noteName(60), "C4");
    EXPECT_EQ(PL::noteName(61), "C#4");
    EXPECT_EQ(PL::noteName(0), "C-1");
    EXPECT_EQ(PL::noteName(127), "G9");
    EXPECT_TRUE(PL::noteName(-1).empty());
    EXPECT_TRUE(PL::noteName(128).empty());
}

TEST(PianoLayout, PressedNoteDescriptionHandlesChordsAndEmptyInput) {
    EXPECT_EQ(PL::describeKeys({60, 64, 67}), "C4 E4 G4");
    EXPECT_EQ(PL::describeKeys({}), "None");
}

} // namespace
