// Unit tests for keyboardLayout.hpp, the math behind the on-screen piano that
// MainApp draws. This is the testable part of mainApp: the window and ImGui
// drawing need a real OpenGL window, but the layout is plain numbers.
//
// The layout covers a range of MIDI keys, e.g. 36 (C2) to 96 (C7), and says where
// each key's rectangle goes. Positions are relative to the keyboard's left edge.

#include <gtest/gtest.h>

#include "keyboardLayout.hpp"

#include <algorithm>
#include <cmath>

namespace {

using keyboardLayout::Key;

// The range MainApp draws: C2 (36) to C7 (96), 5 octaves plus the top C.
constexpr int kFirst = 36;
constexpr int kLast = 96;
constexpr float kWidth = 720.0f;  // total pixel width given to the keyboard
constexpr float kHeight = 160.0f; // height of a white key

// Looks up one key in a layout by its MIDI number (nullptr if it is not there).
const Key* find(const std::vector<Key>& keys, int midiKey) {
    auto it = std::find_if(keys.begin(), keys.end(),
                           [midiKey](const Key& k) { return k.midiKey == midiKey; });
    return it == keys.end() ? nullptr : &*it;
}

// Horizontal center of a key.
float centerX(const Key& k) { return k.x + k.width / 2; }

// ---------------------------------------------------------------------------
// isBlackKey: which keys of an octave are black.
// ---------------------------------------------------------------------------

TEST(KeyboardLayout, BlackKeysAreTheFiveSharpsOfEachOctave) {
    // Pitch classes 1, 3, 6, 8, 10 are C#, D#, F#, G#, A#.
    for (int pc : {1, 3, 6, 8, 10}) {
        EXPECT_TRUE(keyboardLayout::isBlackKey(pc)) << "pitch class " << pc;
        EXPECT_TRUE(keyboardLayout::isBlackKey(pc + 60)) << "pitch class " << pc << " in a higher octave";
    }
    // The seven white keys: C D E F G A B.
    for (int pc : {0, 2, 4, 5, 7, 9, 11}) {
        EXPECT_FALSE(keyboardLayout::isBlackKey(pc)) << "pitch class " << pc;
    }
}

TEST(KeyboardLayout, IsBlackKeyHandlesNegativeNumbers) {
    // -1 is one semitone below C0, i.e. B, a white key. A naive "n % 12" would give
    // -1 in C++ and wrongly fall through; the function must wrap it to 11.
    EXPECT_FALSE(keyboardLayout::isBlackKey(-1));
    EXPECT_TRUE(keyboardLayout::isBlackKey(-2)); // A#
}

// ---------------------------------------------------------------------------
// compute: the full layout.
// ---------------------------------------------------------------------------

TEST(KeyboardLayout, FiveOctavesPlusTopCHasExpectedKeyCounts) {
    const auto keys = keyboardLayout::compute(kFirst, kLast, kWidth, kHeight);

    // C2..C7 is 61 keys: 36 white (7 x 5 octaves + the final C) and 25 black (5 x 5).
    EXPECT_EQ(keys.size(), 61u);
    EXPECT_EQ(std::count_if(keys.begin(), keys.end(), [](const Key& k) { return !k.black; }), 36);
    EXPECT_EQ(std::count_if(keys.begin(), keys.end(), [](const Key& k) { return k.black; }), 25);
}

TEST(KeyboardLayout, WhiteKeysFillTheWidthEvenly) {
    const auto keys = keyboardLayout::compute(kFirst, kLast, kWidth, kHeight);

    const Key* firstWhite = find(keys, 36); // C2, the leftmost key
    const Key* lastWhite = find(keys, 96);  // C7, the rightmost key
    ASSERT_NE(firstWhite, nullptr);
    ASSERT_NE(lastWhite, nullptr);

    // The leftmost key starts at 0 and the rightmost ends exactly at the total width.
    EXPECT_NEAR(firstWhite->x, 0.0f, 0.001f);
    EXPECT_NEAR(lastWhite->x + lastWhite->width, kWidth, 0.001f);

    // 36 white keys share 720 px, so each is 20 px wide and as tall as requested.
    EXPECT_NEAR(firstWhite->width, kWidth / 36, 0.001f);
    EXPECT_NEAR(firstWhite->height, kHeight, 0.001f);

    // Neighbouring white keys touch: D2 starts exactly where C2 ends.
    const Key* d2 = find(keys, 38);
    ASSERT_NE(d2, nullptr);
    EXPECT_NEAR(d2->x, firstWhite->x + firstWhite->width, 0.001f);
}

TEST(KeyboardLayout, BlackKeysAreNarrowerAndShorterThanWhiteKeys) {
    const auto keys = keyboardLayout::compute(kFirst, kLast, kWidth, kHeight);

    const Key* white = find(keys, 36); // C2
    const Key* black = find(keys, 37); // C#2
    ASSERT_NE(white, nullptr);
    ASSERT_NE(black, nullptr);

    EXPECT_NEAR(black->width, white->width * 0.6f, 0.001f);
    EXPECT_NEAR(black->height, white->height * 0.6f, 0.001f);
}

TEST(KeyboardLayout, BlackKeySitsOnTheEdgeBetweenItsTwoWhiteNeighbours) {
    const auto keys = keyboardLayout::compute(kFirst, kLast, kWidth, kHeight);

    // C#2 (37) sits between C2 (36) and D2 (38): its center is the shared edge,
    // which is the right edge of C2.
    const Key* c = find(keys, 36);
    const Key* cSharp = find(keys, 37);
    ASSERT_NE(c, nullptr);
    ASSERT_NE(cSharp, nullptr);
    EXPECT_NEAR(centerX(*cSharp), c->x + c->width, 0.001f);

    // Same for A#3 (58), between A3 (57) and B3 (59), higher up the keyboard.
    const Key* a = find(keys, 57);
    const Key* aSharp = find(keys, 58);
    ASSERT_NE(a, nullptr);
    ASSERT_NE(aSharp, nullptr);
    EXPECT_NEAR(centerX(*aSharp), a->x + a->width, 0.001f);
}

TEST(KeyboardLayout, NoBlackKeyBetweenEAndFOrBAndC) {
    const auto keys = keyboardLayout::compute(kFirst, kLast, kWidth, kHeight);

    // On a real piano there is no black key between E-F or B-C. So no black key's
    // center may sit on the shared edge of E2 (40) / F2 (41) or B2 (47) / C3 (48).
    for (int whiteBeforeGap : {40, 47}) {
        const Key* w = find(keys, whiteBeforeGap);
        ASSERT_NE(w, nullptr);
        const float edge = w->x + w->width;
        for (const Key& k : keys) {
            if (k.black) {
                EXPECT_GT(std::abs(centerX(k) - edge), 0.01f) << "black key " << k.midiKey;
            }
        }
    }
}

TEST(KeyboardLayout, WhiteKeysAreListedBeforeBlackKeysSoBlackDrawsOnTop) {
    const auto keys = keyboardLayout::compute(kFirst, kLast, kWidth, kHeight);

    // MainApp draws keys in list order, so once the first black key appears, no
    // white key may follow it (it would be painted over the black key).
    bool seenBlack = false;
    for (const Key& k : keys) {
        if (k.black) seenBlack = true;
        if (seenBlack) {
            EXPECT_TRUE(k.black) << "white key " << k.midiKey << " after a black key";
        }
    }
}

TEST(KeyboardLayout, EmptyRangeProducesNoKeys) {
    // first > last means no keys at all, and must not divide by zero.
    EXPECT_TRUE(keyboardLayout::compute(60, 50, kWidth, kHeight).empty());
}

} // namespace