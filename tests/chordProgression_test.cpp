#include <gtest/gtest.h>

#include "chordTheory.hpp"

#include <unordered_set>
#include <vector>

namespace {
namespace CT = ChordTheory;
using Notes = std::unordered_set<int>;

// Each snapshot represents a separate played chord, not a rendered frame.
// Exercise the real identifier and key detector together; UI timing and
// history deduplication are outside this component's scope.
std::vector<CT::PlayedChord> identifyProgression(const std::vector<Notes>& snapshots) {
    std::vector<CT::PlayedChord> history;
    for (const auto& notes : snapshots) {
        const auto chord = CT::identifyChord(notes);
        if (chord.found) {
            history.push_back({chord.rootPc, chord.typeIndex});
        }
    }
    return history;
}

TEST(ChordProgression, RecognizedProgressionProducesCMajorKey) {
    // F, G, Am, G: the key must come from the progression, not its first root.
    const auto history = identifyProgression({
        {65, 69, 72}, {67, 71, 74}, {69, 72, 76}, {67, 71, 74}});

    ASSERT_EQ(history.size(), 4u);
    const std::vector<int> roots{5, 7, 9, 7};
    const std::vector<int> types{CT::MAJOR, CT::MAJOR, CT::MINOR, CT::MAJOR};
    for (std::size_t i = 0; i < history.size(); ++i) {
        EXPECT_EQ(history[i].rootPc, roots[i]);
        EXPECT_EQ(history[i].typeIndex, types[i]);
    }
    EXPECT_EQ(CT::detectKey(history), 0);
}

TEST(ChordProgression, EmptyIncompleteAndUnrecognizedInputProducesNoKey) {
    // Silence, a single note, two notes, and a cluster are not full chords.
    const auto history = identifyProgression({{}, {60}, {60, 64}, {60, 61, 62}});

    EXPECT_TRUE(history.empty());
    EXPECT_EQ(CT::detectKey(history), -1);
}

TEST(ChordProgression, OutsideKeyChordDoesNotOverrideEstablishedProgression) {
    auto history = identifyProgression({
        {65, 69, 72}, {67, 71, 74}, {69, 72, 76}, {67, 71, 74}});
    ASSERT_EQ(history.size(), 4u);
    ASSERT_EQ(CT::detectKey(history), 0);

    // D major contains F#, outside C major, but remains a valid played chord.
    const auto outside = CT::identifyChord({62, 66, 69});
    ASSERT_TRUE(outside.found);
    EXPECT_EQ(outside.rootPc, 2);
    EXPECT_EQ(outside.typeIndex, CT::MAJOR);
    EXPECT_FALSE(CT::isDiatonic(0, outside.rootPc, outside.typeIndex));
    history.push_back({outside.rootPc, outside.typeIndex});

    EXPECT_EQ(history.size(), 5u);
    EXPECT_EQ(CT::detectKey(history), 0);
}

TEST(ChordProgression, InvertedChordsContributeTheirRootsRatherThanBassNotes) {
    // First inversions of C, F and G have bass notes E, A and B.
    const auto history = identifyProgression({
        {64, 67, 72}, {69, 72, 77}, {71, 74, 79}});

    ASSERT_EQ(history.size(), 3u);
    EXPECT_EQ(history[0].rootPc, 0);
    EXPECT_EQ(history[1].rootPc, 5);
    EXPECT_EQ(history[2].rootPc, 7);
    for (const auto& chord : history) {
        EXPECT_EQ(chord.typeIndex, CT::MAJOR);
    }
    EXPECT_EQ(CT::detectKey(history), 0);
}

} // namespace
