#include <gtest/gtest.h>

#include "pianoKeyboard.hpp"

#include <set>
#include <tuple>

namespace {

// Compare event contents without depending on their emission order.
using EventValues = std::multiset<std::tuple<int, int, int>>;

EventValues eventValues(const std::vector<NoteState::Event>& events) {
    EventValues values;
    for (const auto& event : events) {
        values.emplace(event.channel, event.note, event.velocity);
    }
    return values;
}

TEST(NoteState, MidiPressAndReleaseUpdateKeysAndEmitEvents) {
    NoteState notes;

    // A MIDI press preserves its velocity in the note-on event.
    EXPECT_EQ(eventValues(notes.setMidi(0, 60, 90)),
              (EventValues{{0, 60, 90}}));
    EXPECT_EQ(notes.pressedKeys(), (std::unordered_set<int>{60}));

    EXPECT_EQ(eventValues(notes.setMidi(0, 60, 0)),
              (EventValues{{0, 60, 0}}));
    EXPECT_TRUE(notes.pressedKeys().empty());
}

TEST(NoteState, UiInputEmitsEventsOnlyForChangedNotes) {
    NoteState notes;

    // UI presses use channel 0 and the default velocity of 100.
    EXPECT_EQ(eventValues(notes.setUi({60, 64})),
              (EventValues{{0, 60, 100}, {0, 64, 100}}));
    EXPECT_EQ(notes.pressedKeys(), (std::unordered_set<int>{60, 64}));

    // Repeating the same held keys must not retrigger their notes.
    EXPECT_TRUE(notes.setUi({60, 64}).empty());
    EXPECT_EQ(notes.pressedKeys(), (std::unordered_set<int>{60, 64}));

    // Removing one key releases only that note; the other stays held.
    EXPECT_EQ(eventValues(notes.setUi({64})), (EventValues{{0, 60, 0}}));
    EXPECT_EQ(notes.pressedKeys(), (std::unordered_set<int>{64}));
}

TEST(NoteState, SharedNoteStaysHeldUntilBothSourcesRelease) {
    NoteState notes;

    EXPECT_EQ(eventValues(notes.setMidi(0, 60, 90)),
              (EventValues{{0, 60, 90}}));
    // UI takes ownership of the same note without a second note-on.
    EXPECT_TRUE(notes.setUi({60}).empty());
    EXPECT_EQ(notes.pressedKeys(), (std::unordered_set<int>{60}));

    // Releasing MIDI cannot silence a note still held by the UI.
    EXPECT_TRUE(notes.setMidi(0, 60, 0).empty());
    EXPECT_EQ(notes.pressedKeys(), (std::unordered_set<int>{60}));

    // The final owner releasing the note produces its single note-off.
    EXPECT_EQ(eventValues(notes.setUi({})), (EventValues{{0, 60, 0}}));
    EXPECT_TRUE(notes.pressedKeys().empty());
}

TEST(NoteState, MidiChannelsMaintainIndependentOwnership) {
    NoteState notes;

    // Both channels emit note-ons, but the pressed-key set lists the pitch once.
    EXPECT_EQ(eventValues(notes.setMidi(0, 60, 90)),
              (EventValues{{0, 60, 90}}));
    EXPECT_EQ(eventValues(notes.setMidi(1, 60, 80)),
              (EventValues{{1, 60, 80}}));
    EXPECT_EQ(notes.pressedKeys(), (std::unordered_set<int>{60}));

    // Channel 0 can stop its voice while channel 1 keeps the pitch held.
    EXPECT_EQ(eventValues(notes.setMidi(0, 60, 0)),
              (EventValues{{0, 60, 0}}));
    EXPECT_EQ(notes.pressedKeys(), (std::unordered_set<int>{60}));

    EXPECT_EQ(eventValues(notes.setMidi(1, 60, 0)),
              (EventValues{{1, 60, 0}}));
    EXPECT_TRUE(notes.pressedKeys().empty());
}

} // namespace
