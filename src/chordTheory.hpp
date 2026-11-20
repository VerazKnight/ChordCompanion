#pragma once

// chordTheory.hpp
//
// The small amount of music theory the learning features of the UI need:
// chord types, which chords a pressed note could start, which chord is being
// played (including inversions), which key a song is in, and which chords
// could come next.
//
// Like pianoLayout.hpp, this file does not use ImGui or GLFW, so it can be
// unit tested without opening a window.
//
// NOTE: this is not the official chord detector. The official one is
// ChordLogic (the model). This file only exists so the UI can teach and
// suggest. The "Detected chord" label still comes from setChordLabel().

#include <algorithm>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace ChordTheory {

// ---------------------------------------------------------------- basics

struct ChordType {
    std::string name;            // "Major"
    std::string suffix;          // "" for major, "m" for minor, ...
    std::vector<int> intervals;  // semitones above the root
};

constexpr int MAJOR = 0;
constexpr int MINOR = 1;
constexpr int DIMINISHED = 2;
constexpr int AUGMENTED = 3;

inline const std::vector<ChordType>& chordTypes() {
    static const std::vector<ChordType> types = {
        { "Major", "", { 0, 4, 7 } },
        { "Minor", "m", { 0, 3, 7 } },
        { "Diminished", "dim", { 0, 3, 6 } },
        { "Augmented", "aug", { 0, 4, 8 } },
    };
    return types;
}

// Pitch class: the note without its octave. C = 0, C# = 1, ... B = 11.
inline int pitchClass(int note) {
    return ((note % 12) + 12) % 12;
}

inline std::string pitchClassName(int pc) {
    static const char* names[12] = { "C", "C#", "D", "D#", "E", "F",
                                     "F#", "G", "G#", "A", "A#", "B" };
    return names[pitchClass(pc)];
}

// "C Major", "A Minor", ...
inline std::string chordName(int rootPc, int typeIndex) {
    return pitchClassName(rootPc) + " " + chordTypes()[typeIndex].name;
}

// "C", "Am", "Bdim", "Caug": the short form used in the "You played" row.
inline std::string chordShortName(int rootPc, int typeIndex) {
    return pitchClassName(rootPc) + chordTypes()[typeIndex].suffix;
}

// The pitch classes of a chord in order: root, third, fifth.
inline std::vector<int> chordPitchClasses(int rootPc, int typeIndex) {
    std::vector<int> result;
    for (int interval : chordTypes()[typeIndex].intervals) {
        result.push_back(pitchClass(rootPc + interval));
    }
    return result;
}

inline std::string inversionName(int inversion) {
    if (inversion == 1) return "1st inversion";
    if (inversion == 2) return "2nd inversion";
    return "root position";
}

// The MIDI notes of a chord on the on-screen piano (C4 = 60 to B5 = 83).
// inversion 0 = root position, 1 = first inversion, 2 = second inversion.
// In an inversion the lowest note moves to the top, one octave higher.
inline std::vector<int> chordNotes(int rootPc, int typeIndex, int inversion) {
    const std::vector<int>& intervals = chordTypes()[typeIndex].intervals;
    std::vector<int> notes;
    for (int interval : intervals) {
        notes.push_back(60 + pitchClass(rootPc) + interval);
    }

    inversion = std::max(0, std::min(inversion, 2));
    for (int i = 0; i < inversion; i++) {
        int lowest = notes.front();
        notes.erase(notes.begin());
        notes.push_back(lowest + 12);
    }

    // Keep the chord on the piano: move it down an octave if it goes past B5.
    if (*std::max_element(notes.begin(), notes.end()) > 83) {
        for (int& note : notes) {
            note -= 12;
        }
    }
    return notes;
}

// ------------------------------------------------ chords a note could start

struct Candidate {
    int typeIndex = 0;
    bool possible = false;          // false = ruled out by the pressed notes
    std::vector<int> missingPcs;    // notes still needed to finish the chord
};

// The lowest pressed note, or -1 if nothing is pressed.
inline int lowestNote(const std::unordered_set<int>& pressed) {
    if (pressed.empty()) {
        return -1;
    }
    return *std::min_element(pressed.begin(), pressed.end());
}

// Treat the lowest pressed note as the root and check every chord type.
// A chord is "possible" while all pressed notes belong to it, and "ruled out"
// as soon as one pressed note does not. Example: with C pressed all four
// C chords are possible; after E is added, C Minor and C Diminished are
// ruled out.
inline std::vector<Candidate> possibleChords(const std::unordered_set<int>& pressed) {
    std::vector<Candidate> result;
    if (pressed.empty()) {
        return result;
    }

    int root = pitchClass(lowestNote(pressed));
    std::set<int> pressedPcs;
    for (int note : pressed) {
        pressedPcs.insert(pitchClass(note));
    }

    for (int t = 0; t < static_cast<int>(chordTypes().size()); t++) {
        std::vector<int> chordPcs = chordPitchClasses(root, t);
        std::set<int> chordSet(chordPcs.begin(), chordPcs.end());

        Candidate candidate;
        candidate.typeIndex = t;
        candidate.possible = std::includes(chordSet.begin(), chordSet.end(),
                                           pressedPcs.begin(), pressedPcs.end());
        for (int pc : chordPcs) {
            if (pressedPcs.count(pc) == 0) {
                candidate.missingPcs.push_back(pc);
            }
        }
        result.push_back(candidate);
    }
    return result;
}

// ------------------------------------------------ which chord is played

struct IdentifiedChord {
    bool found = false;
    int rootPc = 0;
    int typeIndex = 0;
    int inversion = 0;  // 0 = root position, 1 = first, 2 = second
};

// Recognizes a full triad (three different notes) in any order, so
// inversions are recognized too: E G C is C Major in first inversion.
inline IdentifiedChord identifyChord(const std::unordered_set<int>& pressed) {
    std::set<int> pcs;
    for (int note : pressed) {
        pcs.insert(pitchClass(note));
    }
    if (pcs.size() != 3) {
        return IdentifiedChord{};
    }

    int bassPc = pitchClass(lowestNote(pressed));

    // Try the lowest note as the root first, so a symmetric chord such as
    // an augmented triad is reported in root position.
    std::vector<int> roots = { bassPc };
    for (int pc : pcs) {
        if (pc != bassPc) {
            roots.push_back(pc);
        }
    }

    for (int root : roots) {
        for (int t = 0; t < static_cast<int>(chordTypes().size()); t++) {
            std::vector<int> chordPcs = chordPitchClasses(root, t);
            std::set<int> chordSet(chordPcs.begin(), chordPcs.end());
            if (chordSet == pcs) {
                IdentifiedChord found;
                found.found = true;
                found.rootPc = root;
                found.typeIndex = t;
                for (int i = 0; i < 3; i++) {
                    if (chordPcs[i] == bassPc) {
                        found.inversion = i;
                    }
                }
                return found;
            }
        }
    }
    return IdentifiedChord{};
}

// ------------------------------------------------------ keys and next chords

struct PlayedChord {
    int rootPc = 0;
    int typeIndex = 0;
};

// The seven chords of a major key, as semitones above the key's note.
struct Degree {
    int offset;
    int typeIndex;
    const char* numeral;
};

inline const std::vector<Degree>& majorKeyChords() {
    static const std::vector<Degree> degrees = {
        { 0, MAJOR, "I" },   { 2, MINOR, "ii" },  { 4, MINOR, "iii" },
        { 5, MAJOR, "IV" },  { 7, MAJOR, "V" },   { 9, MINOR, "vi" },
        { 11, DIMINISHED, "vii" },
    };
    return degrees;
}

// Position (0 to 6) of a chord in a major key, or -1 if it is not in the key.
inline int degreeIndex(int keyPc, int rootPc, int typeIndex) {
    const std::vector<Degree>& degrees = majorKeyChords();
    for (int i = 0; i < static_cast<int>(degrees.size()); i++) {
        if (pitchClass(keyPc + degrees[i].offset) == pitchClass(rootPc) &&
            degrees[i].typeIndex == typeIndex) {
            return i;
        }
    }
    return -1;
}

inline bool isDiatonic(int keyPc, int rootPc, int typeIndex) {
    return degreeIndex(keyPc, rootPc, typeIndex) >= 0;
}

// Best guess for the major key of the chords played so far (0 = C ... 11 = B),
// or -1 if nothing has been played. The key with the most chords inside it
// wins. Starting or ending on the key's home chord breaks ties.
inline int detectKey(const std::vector<PlayedChord>& history) {
    if (history.empty()) {
        return -1;
    }

    int bestKey = -1;
    int bestScore = -1;
    for (int key = 0; key < 12; key++) {
        int score = 0;
        for (const PlayedChord& chord : history) {
            if (isDiatonic(key, chord.rootPc, chord.typeIndex)) {
                score += 10;
            }
        }
        const PlayedChord& first = history.front();
        const PlayedChord& last = history.back();
        if (degreeIndex(key, first.rootPc, first.typeIndex) == 0) score += 3;
        if (degreeIndex(key, last.rootPc, last.typeIndex) == 0) score += 2;
        // A song that ends on a minor chord is often in its relative major.
        if (last.typeIndex == MINOR && pitchClass(last.rootPc + 3) == key) score += 2;

        if (score > bestScore) {
            bestScore = score;
            bestKey = key;
        }
    }
    return bestKey;
}

struct Suggestion {
    int rootPc = 0;
    int typeIndex = 0;
    std::string reason;
};

inline Suggestion suggestionFor(int keyPc, int degree, const std::string& reason) {
    const Degree& d = majorKeyChords()[degree];
    return Suggestion{ pitchClass(keyPc + d.offset), d.typeIndex, reason };
}

// If the chord is a major chord outside the key whose "target" (a fourth
// above) is inside the key, it works as a bridge to that target.
// Example: in C major, D major is a bridge to G. Returns the target's
// position in the key, or -1.
inline int bridgeTarget(int keyPc, int rootPc, int typeIndex) {
    if (typeIndex != MAJOR || isDiatonic(keyPc, rootPc, typeIndex)) {
        return -1;
    }
    return degreeIndex(keyPc, pitchClass(rootPc + 5), MAJOR) >= 0
               ? degreeIndex(keyPc, pitchClass(rootPc + 5), MAJOR)
               : degreeIndex(keyPc, pitchClass(rootPc + 5), MINOR);
}

// Up to four chords that could come after the last chord played.
inline std::vector<Suggestion> suggestNext(int keyPc, int rootPc, int typeIndex) {
    std::vector<Suggestion> result;
    int degree = degreeIndex(keyPc, rootPc, typeIndex);

    if (degree >= 0) {
        // Each row: the chord to suggest (position in the key) and why.
        struct Next { int degree; const char* reason; };
        static const std::vector<std::vector<Next>> table = {
            /* I    */ { { 3, "Classic next step in this key" },
                         { 1, "Softer mood, often leads to the V chord" },
                         { 2, "Gentle, still in the key" },
                         { 4, "Go to the tense chord" } },
            /* ii   */ { { 4, "The classic move: ii leads to V" },
                         { 3, "Smooth and open" },
                         { 5, "Soft and a little sad" },
                         { 0, "Go back home" } },
            /* iii  */ { { 5, "A natural flow downward" },
                         { 3, "Lift to a brighter chord" },
                         { 1, "A gentle step down" },
                         { 0, "Go back home" } },
            /* IV   */ { { 4, "Build tension" },
                         { 0, "Relax back home" },
                         { 1, "Soft, leads to the V chord" },
                         { 5, "A softer landing" } },
            /* V    */ { { 0, "Resolve home: the strongest ending" },
                         { 5, "A surprise, softer landing" },
                         { 3, "Step back, keep it open" },
                         { 2, "Gentle, still in the key" } },
            /* vi   */ { { 3, "Very common: vi then IV" },
                         { 1, "Soft, leads to the V chord" },
                         { 4, "Build tension" },
                         { 0, "Brighten and go home" } },
            /* vii  */ { { 0, "Resolves strongly home" },
                         { 2, "Gentle, still in the key" },
                         { 5, "Soft and a little sad" },
                         { 4, "Build tension" } },
        };
        for (const Next& next : table[degree]) {
            result.push_back(suggestionFor(keyPc, next.degree, next.reason));
        }
        return result;
    }

    // The last chord is outside the key.
    int target = bridgeTarget(keyPc, rootPc, typeIndex);
    if (target >= 0) {
        result.push_back(suggestionFor(keyPc, target, "It leads naturally here"));
    }
    const int fallback[3] = { 0, 4, 3 };
    const char* reasons[3] = { "Go back home", "Build tension", "Classic next step in this key" };
    for (int i = 0; i < 3 && result.size() < 4; i++) {
        if (fallback[i] != target) {
            result.push_back(suggestionFor(keyPc, fallback[i], reasons[i]));
        }
    }
    return result;
}

// A short sentence about a chord outside the key, or "" if the chord is in
// the key.
inline std::string bridgeMessage(int keyPc, int rootPc, int typeIndex) {
    if (isDiatonic(keyPc, rootPc, typeIndex)) {
        return "";
    }
    int target = bridgeTarget(keyPc, rootPc, typeIndex);
    if (target >= 0) {
        const Degree& d = majorKeyChords()[target];
        return pitchClassName(rootPc) +
               " isn't in this key, but it's a common bridge move. It leads naturally to " +
               chordShortName(keyPc + d.offset, d.typeIndex) + ".";
    }
    return chordShortName(rootPc, typeIndex) +
           " isn't in this key. That is fine, it adds color. Going back to " +
           chordShortName(keyPc, MAJOR) + " will feel like home.";
}

}  // namespace ChordTheory