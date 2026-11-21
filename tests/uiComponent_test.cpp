#include <gtest/gtest.h>

#include "chordTheory.hpp"
#include "learningSession.hpp"
#include "pianoInput.hpp"
#include "pianoLayout.hpp"

#include <unordered_set>
#include <vector>

// Component tests for the UI logic. A component is several parts working
// together. Everything external is replaced by a small fake in this file, so
// no test needs a window, a MIDI device, the real mouse or the real clock.
//
//   LearningSession  Learning::Session + ChordTheory + Learning::glowFor
//                    chords played -> key guess -> ideas after a pause
//                    -> "outside the key" note -> practice chord
//   PianoInput       PianoLayout + PianoInput::State + ChordTheory::identifyChord
//                    mouse, computer keys and MIDI keyboard -> one chord

namespace {

namespace CT = ChordTheory;
using Learning::Session;

using Notes = std::unordered_set<int>;

const Notes C_MAJOR = {60, 64, 67};
const Notes F_MAJOR = {65, 69, 72};
const Notes G_MAJOR = {67, 71, 74};
const Notes A_MINOR = {69, 72, 76};
const Notes D_MAJOR = {62, 66, 69};
const Notes A_MAJOR = {69, 73, 76};

// Stands in for the window loop: presses and releases keys, lets (fake) time
// pass, and calls Session::update the way UI::update does every frame. The
// clock is a plain number, so waiting 15 seconds takes no real time.
class FakePlayer {
public:
    Session session;
    double now = 0.0;
    bool learner = true;
    bool autoSuggest = true;
    double idleSeconds = 3.0;

    void playChord(const Notes& notes) {
        now += 0.5;
        pressed = notes;
        frame();
        now += 0.3;
        pressed.clear();
        frame();
    }

    void wait(double seconds) {
        now += seconds;
        frame();
    }

    // C, F, G: the start of a song in C major.
    void playCFG() {
        playChord(C_MAJOR);
        playChord(F_MAJOR);
        playChord(G_MAJOR);
    }

private:
    void frame() { session.update(pressed, now, learner, autoSuggest, idleSeconds); }

    Notes pressed;
};

// Stands in for PianoKeyboard + RtMidi (a real MIDI device).
struct FakeMidiKeyboard {
    Notes keys;
    Notes getPressedKeys() const { return keys; }
};

// Stands in for the real mouse. x and y are pixels from the piano's corner.
struct FakeMouse {
    float x = 0.0f;
    float y = 0.0f;
    bool down = false;
    bool clicked = false;
    bool shift = false;
};

// One mouse step: find the key under the mouse, then update the input.
void mouseStep(PianoInput::State& input, const FakeMouse& mouse) {
    int note = PianoLayout::noteAtPoint(mouse.x, mouse.y);
    input.updateMouse(note, mouse.down, mouse.clicked, mouse.shift);
}

// A click in the middle of a white key.
FakeMouse clickOn(int note, bool shift) {
    FakeMouse mouse;
    mouse.x = PianoLayout::keyX(note) + PianoLayout::WHITE_KEY_WIDTH / 2.0f;
    mouse.y = 140.0f;
    mouse.down = true;
    mouse.clicked = true;
    mouse.shift = shift;
    return mouse;
}

// The mouse button is up and the mouse is still over C4.
FakeMouse mouseReleasedOverC4() {
    return FakeMouse{20.0f, 140.0f, false, false, false};
}

// ---------------------------------------------------------------- LearningSession

TEST(LearningSession, SongInCMajorIsFollowedThenIdeasAppearAfterAPause) {
    FakePlayer player;
    player.playChord(F_MAJOR);
    player.playChord(G_MAJOR);
    player.playChord(A_MINOR);
    player.playChord(G_MAJOR);
    player.wait(3.5);  // longer than the 3 second pause

    const Session& s = player.session;
    EXPECT_EQ(s.keyPc, 0);                    // C major
    EXPECT_EQ(s.history.size(), 4u);
    EXPECT_FALSE(s.noticeVisible);

    // The last chord was G (the V chord), so the first idea is to go home to C.
    ASSERT_TRUE(s.suggestionsVisible);
    EXPECT_EQ(s.ideasKeyPc, 0);
    ASSERT_FALSE(s.suggestions.empty());
    EXPECT_EQ(s.suggestions[0].rootPc, 0);
    EXPECT_EQ(s.suggestions[0].typeIndex, CT::MAJOR);
}

TEST(LearningSession, ChordOutsideTheKeyShowsANoteAndIdeasStartFromThatChord) {
    FakePlayer player;
    player.playCFG();
    player.playChord(D_MAJOR);  // D Major is not in C major
    player.wait(3.5);

    const Session& s = player.session;

    // The note appears, but playing was never blocked: the chord was added.
    EXPECT_TRUE(s.noticeVisible);
    EXPECT_EQ(s.noticeChord.rootPc, 2);
    EXPECT_EQ(s.noticeChord.typeIndex, CT::MAJOR);
    EXPECT_EQ(s.noticeKeyPc, 0);
    EXPECT_EQ(s.history.size(), 4u);

    // The ideas start fresh from D: first idea is G Major (D's IV chord).
    ASSERT_TRUE(s.suggestionsVisible);
    EXPECT_EQ(s.ideasKeyPc, 2);
    ASSERT_FALSE(s.suggestions.empty());
    EXPECT_EQ(s.suggestions[0].rootPc, 7);
    EXPECT_EQ(s.suggestions[0].typeIndex, CT::MAJOR);
}

TEST(LearningSession, NoteFadesAfterFourteenSecondsAndStartFreshRestartsFromThatChord) {
    FakePlayer player;
    player.playCFG();
    player.playChord(D_MAJOR);
    EXPECT_TRUE(player.session.noticeVisible);

    // Nobody has to press anything: the note goes away by itself.
    player.wait(15.0);
    EXPECT_FALSE(player.session.noticeVisible);

    // Another odd chord brings the note back; "Start fresh here" forgets the
    // old chords and works out the key from this chord on.
    player.playChord(A_MAJOR);
    player.playChord(D_MAJOR);
    player.session.startFreshHere();

    const Session& s = player.session;
    ASSERT_EQ(s.history.size(), 1u);
    EXPECT_EQ(s.history[0].rootPc, 2);
    EXPECT_EQ(s.history[0].typeIndex, CT::MAJOR);
    EXPECT_EQ(s.keyPc, 2);  // D major
    EXPECT_FALSE(s.noticeVisible);
}

TEST(LearningSession, ShowChordKeepsGlowingEvenWhileIdeasAreShowing) {
    FakePlayer player;
    player.playCFG();
    player.playChord(C_MAJOR);
    player.wait(3.5);

    // Ideas are showing, and the first idea is F Major.
    ASSERT_TRUE(player.session.suggestionsVisible);
    EXPECT_EQ(player.session.suggestions[0].rootPc, 5);

    // The player presses Show for C Major, 1st inversion (E4 G4 C5).
    player.session.startPractice(0, CT::MAJOR, 1, player.now);
    Learning::Glow glow = Learning::glowFor(player.session, {}, true);

    // The player's own pick wins over the idea.
    EXPECT_EQ(glow.notes, (std::vector<int>{64, 67, 72}));
    EXPECT_EQ(glow.caption, "Glowing = C Major, 1st inversion (your pick)");
}

TEST(LearningSession, PlayingADifferentChordClearsTheGlowAndAnswersKindly) {
    FakePlayer player;
    player.session.startPractice(0, CT::MAJOR, 1, player.now);

    player.playChord(F_MAJOR);

    Learning::Glow glow = Learning::glowFor(player.session, {}, true);
    EXPECT_TRUE(glow.notes.empty());
    EXPECT_TRUE(player.session.movedOn);  // the inversion box follows the player again
    EXPECT_EQ(player.session.practiceMsg,
              "Oh, you went with F Major instead of C Major. No problem! Press Show again whenever you want to try it.");
}

TEST(LearningSession, PlayerModeStaysQuiet) {
    FakePlayer player;
    player.learner = false;

    player.playCFG();
    player.playChord(D_MAJOR);
    player.wait(10.0);
    player.session.startPractice(0, CT::MAJOR, 1, player.now);

    // No ideas, no note, no glow. The chords are still tracked.
    EXPECT_FALSE(player.session.suggestionsVisible);
    EXPECT_FALSE(player.session.noticeVisible);
    EXPECT_TRUE(Learning::glowFor(player.session, {}, player.learner).notes.empty());
    EXPECT_EQ(player.session.history.size(), 4u);
}

// ---------------------------------------------------------------- PianoInput

TEST(PianoInput, ShiftClickingThreeKeysWithTheMouseMakesAChord) {
    PianoInput::State input;

    for (int note : {60, 64, 67}) {
        mouseStep(input, clickOn(note, true));
    }
    mouseStep(input, FakeMouse{-50.0f, -50.0f, false, false, false});  // mouse leaves the piano

    // Shift + click latches the keys, so they stay down.
    Notes pressed = input.allPressed({});
    EXPECT_EQ(pressed, C_MAJOR);

    CT::IdentifiedChord chord = CT::identifyChord(pressed);
    EXPECT_TRUE(chord.found);
    EXPECT_EQ(chord.rootPc, 0);
    EXPECT_EQ(chord.typeIndex, CT::MAJOR);
    EXPECT_EQ(PianoLayout::describeKeys(PianoLayout::keysToHighlight(pressed)), "C4 E4 G4");
}

TEST(PianoInput, NormalClickHoldsAKeyOnlyWhileTheButtonIsDown) {
    PianoInput::State input;

    mouseStep(input, clickOn(60, false));
    EXPECT_EQ(input.allPressed({}), (Notes{60}));

    mouseStep(input, mouseReleasedOverC4());  // button released
    EXPECT_TRUE(input.allPressed({}).empty());
}

TEST(PianoInput, ShiftClickingALatchedKeyAgainLetsItGo) {
    PianoInput::State input;

    mouseStep(input, clickOn(64, true));
    EXPECT_EQ(input.allPressed({}), (Notes{64}));

    mouseStep(input, clickOn(64, true));
    EXPECT_TRUE(input.allPressed({}).empty());
}

TEST(PianoInput, ComputerKeysReachBothOctavesAndIgnoreKeysPastB5) {
    PianoInput::State input;

    // Low octave (Z): A D G is C4 E4 G4.
    input.setHighOctave(false);
    input.updateComputer({'A', 'D', 'G'});
    EXPECT_EQ(input.allPressed({}), C_MAJOR);

    // High octave (X): the same keys are C5 E5 G5, still a C Major chord.
    input.setHighOctave(true);
    input.updateComputer({'A', 'D', 'G'});
    EXPECT_EQ(input.allPressed({}), (Notes{72, 76, 79}));
    EXPECT_TRUE(CT::identifyChord(input.allPressed({})).found);

    // ';' would be E6 (88), off the piano, so only A is pressed.
    input.updateComputer({'A', ';'});
    EXPECT_EQ(input.allPressed({}), (Notes{72}));
}

TEST(PianoInput, MidiKeyboardMouseAndComputerKeyboardCombineIntoOneChord) {
    FakeMidiKeyboard midi;
    midi.keys = {60};

    PianoInput::State input;
    mouseStep(input, clickOn(64, true));  // E4 with the mouse
    input.updateComputer({'G'});          // G4 with the computer keyboard

    CT::IdentifiedChord chord = CT::identifyChord(input.allPressed(midi.getPressedKeys()));

    EXPECT_TRUE(chord.found);
    EXPECT_EQ(chord.rootPc, 0);
    EXPECT_EQ(chord.typeIndex, CT::MAJOR);
    EXPECT_EQ(chord.inversion, 0);
}

TEST(PianoInput, TwoNotesAreNotAChordAndAClickOutsideThePianoAddsNothing) {
    FakeMidiKeyboard midi;
    midi.keys = {60, 64};

    PianoInput::State input;
    mouseStep(input, FakeMouse{900.0f, 900.0f, true, true, false});  // far outside the piano

    Notes pressed = input.allPressed(midi.getPressedKeys());
    EXPECT_EQ(pressed, (Notes{60, 64}));
    EXPECT_FALSE(CT::identifyChord(pressed).found);
}

TEST(PianoInput, ReleaseAllLetsGoOfMouseKeysButNotTheMidiKeyboard) {
    FakeMidiKeyboard midi;
    midi.keys = {83};  // B5 held on the MIDI keyboard

    PianoInput::State input;
    for (int note : {60, 64, 67}) {
        mouseStep(input, clickOn(note, true));
    }
    EXPECT_EQ(input.allPressed(midi.getPressedKeys()).size(), 4u);

    input.releaseMouseKeys();

    EXPECT_EQ(input.allPressed(midi.getPressedKeys()), (Notes{83}));
}

}  // namespace
